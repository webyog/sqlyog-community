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
#include <ocidl.h>
#include <unknwn.h>
#include <olectl.h>
#include <scintilla.h>

#include "ConnectionBase.h"
#include "CommonHelper.h"
#include "ExportMultiFormat.h"
#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "GUIHelper.h"
#include "Verify.h"
#include <CommCtrl.h>
#include "CCustomComboBox.h"

#include <iomanip>
#include "modes.h"
#include "aes.h"
#include "filters.h"


#define COMUNITY_HTTP	_(L"HTTP Tunneling (Professional/Enterprise/Ultimate only)")
#define COMUNITY_SSH	_(L"SSH Tunneling (Professional/Enterprise/Ultimate only)")
#define COMUNITY_SSL	_(L"SSL Encryption (Professional/Enterprise/Ultimate only)")

#define GETENT_DLG_TEXT _(L"Get SQLyog PROFESSIONAL/ENTERPRISE/ULTIMATE today to enable this and dozens of other great features.")

ConnectionBase::ConnectionBase()
{
    m_conndetaildirty = wyFalse;
    m_haboutfont = m_haboutfont2 = m_hreginfofont = NULL;
	pGlobals->m_isrefreshkeychange	= IsRefreshOptionChange();//setting flag if refresh option switched F5 to F9 
	m_isent = wyFalse;
	m_isconv = wyFalse;
    m_hstatusfont = NULL;
	m_isbuiltactagfile = wyFalse;
	m_enttype = ENT_INVALID;
	m_powertoolsID = 0;
	
	m_rgbconnection = RGB(255,255,255);
	m_rgbconnectionfg = RGB(0,0,0);
	m_setcolor      = wyFalse;
	m_changeobcolor = wyFalse;
	m_rgbobbkcolor  =  RGB(255,255,255);
	m_rgbobfgcolor  =  RGB(0,0,0);

	m_arrayofcolor = NULL;

	m_isnewconnection = wyFalse;
	m_newconnid = -1;

	m_isencrypted_clone = wyFalse;
    
}

ConnectionBase::~ConnectionBase()
{
	if(m_arrayofcolor)
    {
        free(m_arrayofcolor);
        m_arrayofcolor = NULL;
    }
}

wyBool 
ConnectionBase::IsEnt()
{
    return m_isent;
}

void 
ConnectionBase::OnInitDialog(HWND hwnd, LPARAM lparam)
{
    // initializes the dialog box with the default value.
	// also sets the maximum text which can be entered into the value fields.
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_HOST), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_USER), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_PASSWORD), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_PORT), EM_LIMITTEXT, 5, 0);
	SendMessage(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), EM_LIMITTEXT, 6, 0);
	SendMessage(GetDlgItem(hwnd, IDC_PINGINTERVAL), EM_LIMITTEXT, 6, 0);
	
	//Limit database name to 512 characters
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_DATABASE), EM_LIMITTEXT, SIZE_512 - 2, 0);

	//HTTP Tab options
	SendMessage(GetDlgItem(hwnd, IDC_HTTPTIME), EM_LIMITTEXT, 5, 0);

	//SSH Tab options
	SendMessage(GetDlgItem(hwnd, IDC_SSHHOST), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SSHUSER), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SSHPWD), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SSHPRIVATEKEY), EM_LIMITTEXT, 255, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SSHPORT), EM_LIMITTEXT, 5, 0);
    SendMessage(GetDlgItem(hwnd, IDC_DESC), CB_LIMITTEXT, 64, 0);

    SendMessage(GetDlgItem(hwnd, IDC_PINGINTERVAL), EM_LIMITTEXT, 6, 0);

	ShowWindow(GetDlgItem(hwnd, IDC_LINK), SW_SHOW);

#ifndef COMMUNITY 
		//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
		//	{	
				ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_SHOW);
				ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_SHOW);
		//	}
		/*	else
			{
			ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
			ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
			}
		*/
#else
				ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
				ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
#endif

	PostMessage(hwnd, WM_INITCONNDIALOG, 0, 0);
    return;
}

// Callback function to handle the about dialog box.
INT_PTR CALLBACK
ConnectionBase::AboutDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        LocalizeWindow(hwnd);
		return pGlobals->m_pcmainwin->m_connection->OnAboutInitDialog(hwnd);

	case WM_CTLCOLORSTATIC:
        return pGlobals->m_pcmainwin->m_connection->OnWmCtlColorStatic(hwnd, wParam, lParam);

	case WM_COMMAND:
        pGlobals->m_pcmainwin->m_connection->OnAboutWmCommand(hwnd, wParam);
        break;

	case WM_NCDESTROY:
		VERIFY(DeleteObject(pGlobals->m_pcmainwin->m_connection->m_haboutfont));
		pGlobals->m_pcmainwin->m_connection->m_haboutfont = NULL;
		VERIFY(DeleteObject(pGlobals->m_pcmainwin->m_connection->m_haboutfont2));
		pGlobals->m_pcmainwin->m_connection->m_haboutfont2 = NULL;
		break;
	}

	return 0;
}

wyInt32 
ConnectionBase::OnAboutInitDialog(HWND hwnd)
{
	// Initializes the dialog box with the details in the about dialog box.
	wyString    warning(_("Warning : This computer program is protected by copyright laws. Unauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law."));
	wyString    msg, appname;
	HWND        hwndwarning = GetDlgItem(hwnd, IDC_WARNING);
	HWND        hwndstatic = GetDlgItem(hwnd, IDC_LINK);
	FrameWindow    *pcmainwin;
	WNDPROC		origWndProc;

#ifdef ENTERPRISE
	SendMessage(hwnd, WM_SETTEXT, 0,(LPARAM)_(L"About SQLyog"));

	if(pGlobals->m_entlicense.GetLength())
		appname.Sprintf(_("SQLyog %s - MySQL GUI"), pGlobals->m_entlicense.GetString());//Set app name.
	
#elif COMMUNITY 
	SendMessage(hwnd, WM_SETTEXT, 0,(LPARAM)_(L"About SQLyog Community"));
    appname.SetAs(_("SQLyog Community - MySQL GUI"));
#else
	SendMessage(hwnd, WM_SETTEXT, 0,(LPARAM)L"About SQLyog Trial");
    appname.SetAs(_("SQLyog Trial - MySQL GUI"));
#endif
	
	VERIFY(pcmainwin = (FrameWindow*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA));

	SendMessage(hwndwarning, WM_SETTEXT, 0,(LPARAM)warning.GetAsWideChar());

	origWndProc = (WNDPROC)SetWindowLongPtr(hwndstatic, GWLP_WNDPROC,(LONG_PTR)StaticDlgProc);

	SetWindowLongPtr(hwndstatic, GWLP_USERDATA,(LONG_PTR)origWndProc);

	msg.Sprintf("%s %s\n%s", appname.GetString(), APPVERSION, COMPANY_COPYRIGHT);
	SetWindowText(GetDlgItem(hwnd, IDC_APPINFO), msg.GetAsWideChar());

	AboutRegInfo(hwnd);

	return 0;
}

LRESULT CALLBACK
	ConnectionBase::StaticDlgProcLinkCursor (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ConnectionBase * pdb = (ConnectionBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		return 0;
	}

	return CallWindowProc(pdb->m_wpstaticorigproc, hwnd, message, wParam, lParam);
}

wyInt32
ConnectionBase::OnWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyInt32   identifier;
	wyUInt32  fontheight;	
    HDC	hdc = (HDC)wparam;
	
	identifier = GetDlgCtrlID((HWND)lparam);
	if(identifier == IDC_LINK)
	{
		return SetAsLink(hdc);
	} 
	else if(identifier == IDC_REGINFO)
	{
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		if(!m_hreginfofont)
			m_hreginfofont = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, 0, 
            0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_hreginfofont);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));

		return (wyInt32)GetStockObject(HOLLOW_BRUSH);
	} 
	else if(identifier == IDC_APPINFO || identifier == IDC_WARNING)
	{
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		if(!m_haboutfont2)
			m_haboutfont2 = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_haboutfont2);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));

		return (wyInt32)GetStockObject(HOLLOW_BRUSH);
	}

    return 0;
}

wyBool
ConnectionBase::ConfirmAndSaveConnection(HWND hwnd, wyBool onconnect)
{
	wyInt32		ret;
	wyInt32     index;
    
    if((onconnect == wyTrue) && (m_isnewconnection == wyTrue))
	{
		/*ret = yog_message(hwnd, 
			  _(L"You are trying to connect to a new connection.\nConnection will be saved."), 
			  pGlobals->m_appname.GetAsWideChar(), MB_OKCANCEL | MB_ICONINFORMATION);*/
		ret = yog_message(hwnd, 
			  _(L"You have changed your connection details.\nDo you want to save changes?"), 
			  pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONINFORMATION);
	
		if(ret == IDYES)
		{
			SaveConnection(hwnd);
			return wyTrue;
		}
		else
		{
			return wyTrue;
			//return wyFalse;
		}
	}
	else
	{
		ret = yog_message(hwnd, 
			  _(L"You have changed your connection details.\nDo you want to save changes?"), 
			  pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONINFORMATION);
	
		if(ret == IDYES)
		{
			SaveConnection(hwnd);
		}
		else
		{
			//int index;
			index = SendMessage(GetDlgItem(hwnd,IDC_COLORCOMBO),CB_GETCURSEL,0,0);
			
			if(m_isnewconnection == wyTrue)
			{
				DeleteNewConnectionDetails(hwnd);
			}
			else
			{
				//HandlerToGetInitialDetails resets the selected colors, so the following code is to save the colors
				m_rgbobbkcolor = m_rgbconnection;
				m_rgbobfgcolor = m_rgbconnectionfg;
				HandlerToGetInitialDetails(hwnd);
				if(m_rgbconnection != m_rgbobbkcolor || m_rgbconnectionfg != m_rgbobfgcolor)
				{
					EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
					m_rgbconnection = m_rgbobbkcolor;
					m_rgbconnectionfg = m_rgbobfgcolor;
					m_conndetaildirty = wyTrue;
					return wyTrue;
				}
			}
		}
		
	}
	//EnableWindow(GetDlgItem(hwnd, IDC_DELETE), TRUE);	
	//EnableWindow(GetDlgItem(hwnd, IDC_DBNAME), TRUE);	
    m_conndetaildirty = wyFalse;
	return wyTrue;
}



/*	starting from SQLyog 4.0 we have options for SSH and HTTP tunneling
	since dialog editor doesn't support creation of windows in tab etc that well
	so we make it from hand. painful process but gives more control :)
*/
wyBool 
ConnectionBase::CreateConnectDialogWindows(HWND hwnd, wyBool ispowertools)
{
	HWND		geninfo;
	HWND		hwndtab;
	RECT		rc;
	wyInt32    topx = 22, sidex =3;

	//if connection dialog is of powertools, then no advanced tab
    VERIFY(pGlobals->m_pcmainwin->m_connection->CreateTabs(hwnd, IDC_CONNTAB, ispowertools));

	VERIFY(hwndtab = GetDlgItem(hwnd, IDC_CONNTAB));
	VERIFY(GetClientRect(hwndtab, &rc));
	
	pGlobals->m_pcmainwin->m_geninfo = geninfo = CreateWindow(L"STATIC", L"", WS_CHILD | WS_TABSTOP, 
                                                    rc.left + sidex, rc.top + topx, rc.right -(sidex*2), 
                                                    rc.bottom -(topx+sidex), hwndtab,(HMENU)GENERALWND, 
                                                    pGlobals->m_hinstance, NULL);
	VERIFY(geninfo);
	ShowWindow(geninfo, SW_SHOW);
	ShowWindow(GetDlgItem(hwndtab, IDC_LINK), SW_SHOW);

/*#ifndef COMMUNITY 
	if(pGlobals->m_entlicense.CompareI("Professional") == 0)
	{
		ShowWindow(GetDlgItem(hwndtab, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hwndtab, IDC_READONLY), SW_HIDE);
	}
#else
	ShowWindow(GetDlgItem(hwndtab, IDC_COMPRESSHELP), SW_HIDE);	
	ShowWindow(GetDlgItem(hwndtab, IDC_READONLY), SW_HIDE);
#endif
*/
	#ifdef COMMUNITY
		ShowWindow(GetDlgItem(hwndtab, IDC_COMPRESSHELP), SW_HIDE);	
		ShowWindow(GetDlgItem(hwndtab, IDC_READONLY), SW_HIDE);
	#endif

	return wyTrue;
}


// Function reads the connection.ini file and reads from it and fills the dialog box with the details
// obtained from the file. It reads data sizeof CONNINFO struture which contains host, username,
// password and port info.
void
ConnectionBase::InitConnDialog(HWND hdlg, HWND combo, wyBool toadd, wyBool selectfirst)
{
	wyInt32	ret, connindex;
	wyInt32     index;
	wyWChar     directory[MAX_PATH + 1], *lpfileport=0;
    wyChar      *tempnum = NULL, *allsectionnames, *tempconsecname;
	wyString    conn, dirstr, connnamestr, connselnamestr, tempnumstr;
	wyString	selconnnamestr, codepage, allsecnames, tempdir;
   	HWND		hwndcombo;
	
    wyChar	    seps[] = ";";
    RECT        rct;
	wyInt32		width;//number of characters in connection name 
    
    rct.left    = 28;
    rct.bottom  = 2;
    rct.top     = 2;
    rct.right   = 0;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return;
	
	dirstr.SetAs(directory);
	tempdir.SetAs(directory);

	

	if(!combo)
		VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));
	else
		hwndcombo = combo;

	SendMessage(hwndcombo, CCBM_SETMARGIN, NULL, (LPARAM)&rct); 
    
    ret = wyIni::IniGetString(SECTION_NAME, "Host", "root", &selconnnamestr, dirstr.GetString());

    //wyIni::IniWriteString(SECTION_NAME, "Host", selconnnamestr.GetString(), dirstr.GetString());

	//wyIni::IniWriteString(SECTION_NAME, "Encoding", "utf8", dirstr.GetString());

	wyIni::IniGetSection(&allsecnames, &tempdir);

    allsectionnames = (wyChar*)allsecnames.GetString();

    tempconsecname = strtok(allsectionnames, seps);
	while(tempconsecname)
	{
        conn.SetAs(tempconsecname);
		
        tempnum  = strstr(tempconsecname, " ");
		
		if(tempnum != NULL)
		{
			tempnum = tempnum + 1;
			tempnumstr.SetAs(tempnum);
		}

        wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());

        //to avoid duplicate connection name, we need to search if the connection name is already there in combo.
		connindex = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)connnamestr.GetAsWideChar());

        if((connnamestr.GetLength() == 0) || (connindex != CB_ERR))
        {
            tempconsecname = strtok(NULL, seps);
			continue;
        }

		//wyIni::IniWriteString(conn.GetString(), "Name", connnamestr.GetString(), dirstr.GetString());
		
		index = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)connnamestr.GetAsWideChar());
		VERIFY(SendMessage(hwndcombo, CB_SETITEMDATA, index, (LPARAM)tempnumstr.GetAsInt32()));

        tempconsecname = strtok(NULL, seps);
    }

    if(toadd)
    {
        SendMessage(hwndcombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)(DEFAULT_CONN));
    }
   
	if(!selectfirst)
	{
        index = SendMessage(hwndcombo, CB_FINDSTRING, -1,(LPARAM)selconnnamestr.GetAsWideChar());
		
		if(index == -1)
			index = 0;
	}
	else
	{
		index = 0;
	}

    pGlobals->m_pcmainwin->m_connection->PopulateColorArray(hwndcombo, &dirstr);

	SendMessage(hwndcombo, CB_SETCURSEL, index, 0);

	/*width = SetComboWidth(hwndcombo);
	SendMessage(hwndcombo, CB_SETDROPPEDWIDTH, width+50, 0); //width + 50 means width of the text + width of the scroll bar*/

	HWND actualcombo = (HWND)SendMessage(hwndcombo, CCBM_GETCOMBOHWND, 0, 0);
	width = SetComboWidth(actualcombo);
	SendMessage(actualcombo, CB_SETDROPPEDWIDTH, width + 50, 0);
	return;
}

void 
ConnectionBase::ExamineData(wyString &codepage, wyString &buffer)
{
	IMultiLanguage2		*mlang;
    DetectEncodingInfo	info;
    MIMECPINFO          codepageinfo;
	wyInt32				length = 0, cnt = 1;
	wyString			codepagestr;
	
	if(buffer.GetString())
		length = buffer.GetLength();

	HRESULT hr		=		S_OK;
	SUCCEEDED(CoInitialize(NULL));							// init COM
  
	hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2,(void **)&mlang);
	hr = mlang->DetectInputCodepage(0,0, (wyChar *)buffer.GetString(), &length, &info, &cnt);

    if(SUCCEEDED(hr))
    {
        hr = mlang->GetCodePageInfo(info.nCodePage, info.nLangID, &codepageinfo);
        if(SUCCEEDED(hr))
            codepage.SetAs(codepageinfo.wszDescription);
		else
		{
			CoUninitialize();
			goto cleanup;
		}
    }
	else
	{
		CoUninitialize();
		goto cleanup;
	}

cleanup:
	mlang->Release();
	CoUninitialize();
}

void 
ConnectionBase::InitCodePageCombo(HWND hdlg, wyInt32 ctrl_id)
{
	HWND hcpcombo  = GetDlgItem(hdlg, ctrl_id);
	wyInt32 count = SendMessage(hcpcombo , CB_GETCOUNT, 0, 0), itemcount;

	// Removes the contents in the combo 
	for(itemcount = 0; itemcount < count; itemcount++)
		SendMessage(hcpcombo , CB_DELETESTRING, 0, 0);
	
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"latin1");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"latin2");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"latin5");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"latin7");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"ascii");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp850");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp852");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp866");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp1250");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp1251");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp1256");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp1257");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"greek");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"hebrew");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"tis620");//thai
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"koi8r");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"koi8u");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"binary");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"geostd8");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"macroman");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"macce");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"keybcs2");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"dec8");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"hp8");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"swe7");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"armscii8");

	// those using more than one byte......
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"big5");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"ujis");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"sjis");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"euckr");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"gb2312");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"gbk");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"utf8");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"cp932");
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"eucjpms");
	
	SendMessage(hcpcombo , CB_ADDSTRING, 0,(LPARAM)L"[default]");

	SendMessage(hcpcombo , CB_SELECTSTRING, -1,(LPARAM)L"[default]");
}

void    
ConnectionBase::OnValidConNameChngState(HWND hdlg, BOOL state)
{
    wyInt32     count, size;
	wyInt32     id_arr[] = { IDC_CONNTAB, IDC_CLONECONN, IDC_DELETE,
							 IDC_EDITCONN, IDC_DLGCONNECT_HOST, IDC_DLGCONNECT_USER, 
							 IDC_DLGCONNECT_PASSWORD, IDC_DLGCONNECT_STOREPASSWORD, 
							 IDC_DLGCONNECT_DATABASE, IDC_DLGCONNECT_PORT, IDOK, IDC_TESTCONN,
							 IDC_COMPRESS, IDC_COMPRESSHELP, IDC_TIMEOUT, IDC_TIMEOUTDEF, 
							 IDC_TIMEOUTOPT, IDC_TIMEOUTHELP, IDC_TIMESEC , 
							 IDC_KEEPALIVE, IDC_KEEPALIVESEC, IDC_PINGINTERVAL
							};
	HWND		hctrl;

	size = sizeof(id_arr)/ sizeof(id_arr[0]);
	for(count = 0; count < size; count++)
    {
		VERIFY(hctrl = GetDlgItem(hdlg, id_arr[count])); 
		EnableWindow(hctrl, state);
	}
VERIFY(hctrl = GetDlgItem(hdlg, IDC_READONLY));
/*
#ifndef COMMUNITY 
	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
	{	
		ShowWindow(GetDlgItem(hdlg, IDC_COMPRESSHELP), SW_SHOW);
		ShowWindow(GetDlgItem(hdlg, IDC_READONLY), SW_SHOW);
	}	//	EnableWindow(hctrl, state);
	else
	{
		ShowWindow(GetDlgItem(hdlg, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hdlg, IDC_READONLY), SW_HIDE);
	}	//EnableWindow(hctrl, FALSE);
#else
		EnableWindow(hctrl, FALSE);
#endif
*/
/*
#ifndef COMMUNITY
		ShowWindow(GetDlgItem(hdlg, IDC_COMPRESSHELP), SW_SHOW);
		ShowWindow(GetDlgItem(hdlg, IDC_READONLY), SW_SHOW);
#else
		ShowWindow(GetDlgItem(hdlg, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hdlg, IDC_READONLY), SW_HIDE);
#endif
		*/
    if(m_conndetaildirty == wyTrue)
    {
       EnableWindow(GetDlgItem(hdlg, IDC_SAVE), state);
    }
    else
    {
        EnableWindow(GetDlgItem(hdlg, IDC_SAVE), FALSE);
    }

	if(state == FALSE)
	{
		EnableWindow(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), FALSE); 
	}
}

// Function changes the state of various buttons in the conection dialog box.
wyInt32 
ConnectionBase::ChangeConnStates(HWND hdlg, wyBool state)
{
	wyInt32     count, size;
	wyInt32     id_arr[] = { IDC_CONNTAB, IDC_CLONECONN, IDC_DELETE, IDC_DESCSTATIC, IDC_DESC, 
							 IDC_EDITCONN, IDC_MYSQLHOSTST, IDC_DLGCONNECT_HOST, 
							 IDC_MYSQLUSERST, IDC_DLGCONNECT_USER, 
							 IDC_MYSQLPWDST, IDC_DLGCONNECT_PASSWORD, IDC_DLGCONNECT_STOREPASSWORD, 
							 IDC_MYSQLDBST, IDC_DLGCONNECT_DATABASE, IDC_COLONST, IDC_MYSQLPORTST, 
							 IDC_DLGCONNECT_PORT, IDC_MYSQLDEFSRVST, IDOK, IDC_TESTCONN,
							 IDC_COMPRESS, IDC_COMPRESSHELP, IDC_TIMEOUT, IDC_TIMEOUTDEF, 
							 IDC_TIMEOUTOPT, IDC_TIMEOUTHELP, IDC_TIMESEC , 
							 IDC_KEEPALIVE, IDC_KEEPALIVESEC, IDC_PINGINTERVAL, IDC_READONLY
							};
	HWND		hctrl;

	size = sizeof(id_arr)/ sizeof(id_arr[0]);
	for(count = 0; count < size; count++)
    {
		VERIFY(hctrl = GetDlgItem(hdlg, id_arr[count])); 
		EnableWindow(hctrl, state);
	}
VERIFY(hctrl = GetDlgItem(hdlg, IDC_READONLY));
#ifndef COMMUNITY 
//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
		EnableWindow(hctrl, state);
//	else
//		EnableWindow(hctrl, FALSE);
	//ShowWindow(GetDlgItem(hwnd, IDC_READONLY), state);
#else
		//EnableWindow(hctrl, FALSE);
	ShowWindow(GetDlgItem(hdlg, IDC_READONLY), SW_HIDE);
	ShowWindow(GetDlgItem(hdlg, IDC_COMPRESSHELP), SW_HIDE);

#endif
	if(state == wyFalse)
	{
		EnableWindow(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), FALSE); 
	}

	return 1;
}

wyInt32
ConnectionBase::NewConnection(HWND hdlg)
{
	wyInt64         ret, index;
	wyUInt32        concount = 1;
    wyWChar         directory[MAX_PATH + 1] = {0}, *lpfileport=0;
	wyString        conn, dirstr, connnamestr, allsecnames;
    CONNDLGPARAM	pconn = {0};
	HWND		    hwndcombo;

	
	wcscpy(pconn.m_connname, _(L"New Connection"));
	pconn.m_isnew = wyTrue;

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CONNECTIONNAME), 
                         hdlg, ConnectionBase::ConnDetailDialogProc, (LPARAM)&pconn); 

	if(!ret)
		return 0;

	m_isnewconnection = wyTrue;
	

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return 0;
	
    dirstr.SetAs(directory);

	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));

    wyIni::IniGetSection(&allsecnames, &dirstr);
  
	for(concount = 0; concount < MAX_CONN; concount++)
    {
		wyChar *temp = NULL;
		if(allsecnames.GetLength() == 0)
			break;

		conn.Sprintf("Connection %u", concount+1);
		if((temp  = strstr((wyChar*)allsecnames.GetString(), conn.GetString())) == NULL)
			break;


		wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());

        if(connnamestr.GetLength() == 0)
			break;
	}

	if(concount == MAX_CONN)
		concount = 0;

	conn.Sprintf("Connection %u", concount + 1);

    WriteDefValues(hdlg);
	
	index = SendMessage(hwndcombo, CB_ADDSTRING, -1,(LPARAM)pconn.m_connname);
	SendMessage(hwndcombo, CB_SETITEMDATA, index, concount+1);

	//populate color array for connection combo
	PopulateColorArray(hwndcombo, &dirstr);

    if(index == CB_ERR)
		SendMessage(hwndcombo, CB_SETCURSEL, 0, 0);
	else
	{
		SendMessage(hwndcombo, CB_SETCURSEL, index, 0);
		m_newconnid = index;
	}
	

	HandlerToGetInitialDetails(hdlg);
	ChangeConnStates(hdlg, wyTrue);

	/* we have to change the tab selection to server also */
	TabCtrl_SetCurSel(GetDlgItem(hdlg, IDC_CONNTAB), 0);
    ShowServerOptions(hdlg);
   	m_conndetaildirty = wyTrue;

	EnableWindow(GetDlgItem(hdlg, IDC_SAVE), TRUE);
	EnableWindow(GetDlgItem(hdlg, IDC_CLONECONN), FALSE);	
	EnableWindow(GetDlgItem(hdlg, IDC_DELETE), FALSE);	
	EnableWindow(GetDlgItem(hdlg, IDC_EDITCONN), FALSE);	

	return 1;
}

wyInt32
ConnectionBase::CloneConnection(HWND hdlg)
{
	wyInt64         ret, index;
	wyUInt32        concount = 1;
    wyWChar         directory[MAX_PATH + 1] = {0}, *lpfileport=0;
	wyString        conn, dirstr, connnamestr, allsecnames;
    CONNDLGPARAM	pconn = {0};
	HWND		    hwndcombo;

	
	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return 0;
	
    dirstr.SetAs(directory);

	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));

    wyIni::IniGetSection(&allsecnames, &dirstr);
	

	this;
	conn.Clear();
	wyIni::IniGetString(m_consectionname.GetString(), "Name", "", &connnamestr, dirstr.GetString());
	conn.SetAs(connnamestr.GetAsWideChar());
	conn.Add("_New");

	wyString isencrypted("");
	wyIni::IniGetString(m_consectionname.GetString(), "Isencrypted", "0", &isencrypted, dirstr.GetString());

	if (isencrypted.Compare("1") == 0)
	{
		m_isencrypted_clone = wyTrue;
	}

	// THE FOLLOWING FIXES THE Connection Size when clonning the name.
	// so that the size will not exceeds the 64 char limit 
	// if for 13.1.6 version is decided to be kept unchanged switch to the comment line below
	wcsncpy(pconn.m_connname, _(conn.GetAsWideChar()),64); 
	// wcscpy(pconn.m_connname, _(conn.GetAsWideChar()));
	pconn.m_isnew = wyTrue;
	conn.Clear();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CONNECTIONNAME), 
                         hdlg, ConnectionBase::ConnDetailDialogProc, (LPARAM)&pconn); 

	if(!ret)
		return 0;

	m_isnewconnection = wyTrue;

	for(concount = 0; concount < MAX_CONN; concount++)
    {
		wyChar *temp = NULL;
		if(allsecnames.GetLength() == 0)
			break;

		conn.Sprintf("Connection %u", concount+1);
		if((temp  = strstr((wyChar*)allsecnames.GetString(), conn.GetString())) == NULL)
			break;


		wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());

        if(connnamestr.GetLength() == 0)
			break;
	}

	if(concount == MAX_CONN)
		concount = 0;

	conn.Sprintf("Connection %u", concount + 1);

 	index = SendMessage(hwndcombo, CB_ADDSTRING, -1,(LPARAM)pconn.m_connname);
	SendMessage(hwndcombo, CB_SETITEMDATA, index, concount+1);

	//populate color array for connection combo
	PopulateColorArray(hwndcombo, &dirstr, concount+1, m_rgbconnection);

    if(index == CB_ERR)
		SendMessage(hwndcombo, CB_SETCURSEL, 0, 0);
	else
	{
		SendMessage(hwndcombo, CB_SETCURSEL, index, 0);
		m_newconnid = index;
	}
		
	/*HandlerToGetInitialDetails(hdlg);
	ChangeConnStates(hdlg, wyTrue);*/

	/* we have to change the tab selection to server also */
	TabCtrl_SetCurSel(GetDlgItem(hdlg, IDC_CONNTAB), 0);
    ShowServerOptions(hdlg);
   	m_conndetaildirty = wyTrue;

	EnableWindow(GetDlgItem(hdlg, IDC_SAVE), TRUE);
	EnableWindow(GetDlgItem(hdlg, IDC_DELETE), FALSE);	
	EnableWindow(GetDlgItem(hdlg, IDC_CLONECONN), FALSE);	
	EnableWindow(GetDlgItem(hdlg, IDC_EDITCONN), FALSE);	

	return 1;
}



wyBool  
ConnectionBase::ChangeConnName(HWND hdlg)
{
	wyInt64         count, cursel, ret;
	HWND			hwndcombo;
	wyWChar         directory[MAX_PATH + 1]={0}, *lpfileport = 0;
	wyString        connname, connnamestr, dirstr;
	CONNDLGPARAM	pconn={0};
    HWND            hwndc = NULL;

	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));
    hwndc = (HWND)SendMessage(hwndcombo, CCBM_GETCOMBOHWND, NULL, NULL); 

	VERIFY ((cursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
	VERIFY ((SendMessage(hwndc, CB_GETLBTEXT, cursel,(LPARAM)pconn.m_connname))); 
	VERIFY((count = SendMessage(hwndcombo, CB_GETITEMDATA, cursel, 0)));

	pconn.m_isnew = wyFalse;

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CONNECTIONNAME), hdlg,ConnectionBase::ConnDetailDialogProc,(LPARAM)&pconn); 

	if(!ret)
		return wyFalse;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;

	connname.Sprintf("Connection %u", count);
	
	connnamestr.SetAs(pconn.m_connname);
	dirstr.SetAs(directory);
	ret = wyIni::IniWriteString(connname.GetString(), "Name", connnamestr.GetString(), dirstr.GetString());
	VERIFY(SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0));

	InitConnDialog(hdlg, NULL, wyFalse);

	// here we can initialize the default server code page combo also..........
	//InitCodePageCombo(hdlg, IDC_DEFSRV);

    ret = SendMessage(hwndcombo, CB_SELECTSTRING, -1, (LPARAM)pconn.m_connname);

    if(ret == CB_ERR)
		SendMessage(hwndcombo, CB_SETCURSEL, 0,(LPARAM)0);
    
    GetInitialDetails(hdlg);

	SendMessage(hwndcombo, CCBM_SETMARGIN, NULL, NULL); 
    m_conndetaildirty = wyFalse;
	
	return wyTrue;
}

INT_PTR CALLBACK
ConnectionBase::ConnDetailDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PCONNDLGPARAM	pconn =(PCONNDLGPARAM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyString		connnamestr, currname;
	wyWChar			connnamechar[SIZE_512]={0};
	

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return 1;

	case WM_INITDLGVALUES:

		if(pconn->m_isnew ==  wyTrue)
			SetWindowText(hwnd, _(L" New Connection"));
		else
			SetWindowText(hwnd, _(L" Rename Connection"));

		SendMessage(GetDlgItem(hwnd, IDC_DBNAME), EM_SETLIMITTEXT, 64, 0);
		SetWindowText(GetDlgItem(hwnd, IDC_DBNAME), pconn->m_connname);
		SendMessage(GetDlgItem(hwnd, IDC_DBNAME), EM_SETSEL, 0, -1);
		break;

	case WM_COMMAND:
		if(HIWORD(wParam)== 2)
        {
			if(SendMessage(GetDlgItem(hwnd, IDC_DBNAME), WM_GETTEXTLENGTH, 0, 0))
				EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
			else
				EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		}

		switch(LOWORD(wParam))
		{
		case IDOK:
			//Get current name of connection
			currname.SetAs(pconn->m_connname);

			SendMessage(GetDlgItem(hwnd, IDC_DBNAME), WM_GETTEXT, SIZE_512-1, (LPARAM)connnamechar);

			connnamestr.SetAs(connnamechar);

			if(IsQueryEmpty(connnamestr.GetAsWideChar()))
            {
				yog_message(hwnd, _(L"Please enter a valid name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_HELP | MB_ICONINFORMATION);
				return 0;
			}
            else if(pGlobals->m_pcmainwin->m_connection->CheckConnectionName(connnamestr.GetAsWideChar(), &currname, pconn->m_isnew) == wyFalse)
            {
				yog_message(hwnd, _(L"Connection already exists, Please enter a unique name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_HELP | MB_ICONINFORMATION);
				return 0;
			}
			
			wcscpy(pconn->m_connname, connnamestr.GetAsWideChar());
			yog_enddialog(hwnd, 1);
			//free(connnamechar);
			break;

		case IDCANCEL:
			yog_enddialog(hwnd, 0);
			break;
		}
		break;

	default:
		break;
	}
	return 0;
}

wyInt32 
ConnectionBase::CheckConnectionName(wyWChar *connectionname, wyString * currname, wyBool newcon)
{
    wyString	conn, connnamestr, connectionnamestr, dirstr, allsecnames;
	wyWChar		*lpfileport;
	wyChar      *tempconsecname, *allsectionnames, seps[] = ";";
	wyWChar     directory[MAX_PATH+1] = {0};
    wyChar		connames[MAX_CONN][512];
    wyInt32     bufconcount = 0, concount, ret;
	
	connectionnamestr.SetAs(connectionname);

	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	if(ret == 0)
		return 0;
	
	dirstr.SetAs(directory);

    wyIni::IniGetSection(&allsecnames, &dirstr);

    allsectionnames = (wyChar *)allsecnames.GetString();

    tempconsecname = strtok(allsectionnames, seps);
	while(tempconsecname)
    {        
        conn.SetAs(tempconsecname);

        wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());

        if(connnamestr.GetLength() == 0)
        {
            tempconsecname = strtok(NULL, seps);
			continue;
        }

        strcpy(connames[bufconcount++], connnamestr.GetString());

        tempconsecname = strtok(NULL, seps);
    }				
	
	//if it is rename connection and new name is same as old then do not throw any error
	if(newcon == wyFalse)
	{
		if(stricmp(connectionnamestr.GetString(), currname->GetString()) ==  0)
			return 1;
	}

	/* Checks the obtained connection is existing or not, 
								if it is existing change its name */
	for(concount = 0; concount < bufconcount; concount++)
	{
		if(stricmp(connectionnamestr.GetString(), connames[concount]) ==  0)
			return 0;				
	}

	return 1;
}

void
ConnectionBase::OnWmConnectionNotify(HWND hwnd, LPARAM lParam)
{
	LPNMHDR		lpnmhdr	=(LPNMHDR)lParam;


	switch(lpnmhdr->code)
	{
	case TCN_SELCHANGE:
		OnConnDialogTabSelChange(hwnd, lpnmhdr->hwndFrom);
        break;
	}
    return;
}

wyBool 
ConnectionBase::OnConnDialogTabSelChange(HWND hwnd, HWND hwndtab)
{
	wyInt32 index;
		
    index = TabCtrl_GetCurSel(hwndtab);
	VERIFY(index != -1);

	// whenever the tab changes we loop and hide all the corresponding windows for the other tab and show all of this tab 
	// since all the controls are child of the tab control we have to do this extra coding 
	
    switch(index)
    {
        case SERVER_TAB:
            ShowSeverTabOptions(hwnd, wyTrue);
            ShowHttpTabOptions(hwnd);
            ShowSshTabOptions(hwnd);
            ShowSslTabOptions(hwnd);
			ShowAdvanceTabOptions(hwnd);
            break;
	
        case HTTP_TAB:
            ShowHttpTabOptions(hwnd, wyTrue);
            ShowSeverTabOptions(hwnd);
            ShowSshTabOptions(hwnd);
            ShowSslTabOptions(hwnd);
			ShowAdvanceTabOptions(hwnd);
            break;

        case SSH_TAB:
            ShowSshTabOptions(hwnd, wyTrue);
            ShowSeverTabOptions(hwnd);
            ShowHttpTabOptions(hwnd);
            ShowSslTabOptions(hwnd);
			ShowAdvanceTabOptions(hwnd);
            break;

        case SSL_TAB:
            ShowSslTabOptions(hwnd, wyTrue);
            ShowSshTabOptions(hwnd);
            ShowSeverTabOptions(hwnd);
            ShowHttpTabOptions(hwnd);
			ShowAdvanceTabOptions(hwnd);
            break;

		case ADVANCE_TAB:
			ShowAdvanceTabOptions(hwnd, wyTrue);
			ShowSeverTabOptions(hwnd);
            ShowHttpTabOptions(hwnd);
            ShowSshTabOptions(hwnd);
            ShowSslTabOptions(hwnd);
            break;

        default:
            return wyFalse;
    }
	
	return wyTrue;
}

void 
ConnectionBase::ShowSeverTabOptions(HWND hwnd, wyBool enable)
{
    wyInt32	serverids[] = {	IDC_MYSQLHOSTST, IDC_DLGCONNECT_HOST, IDC_MYSQLUSERST, IDC_DLGCONNECT_USER, IDC_MYSQLPWDST,
							IDC_DLGCONNECT_PASSWORD, IDC_DLGCONNECT_STOREPASSWORD, IDC_MYSQLDBST, 
                            IDC_DLGCONNECT_DATABASE, IDC_COLONST, IDC_MYSQLPORTST, IDC_DLGCONNECT_PORT, 
							IDC_MYSQLDEFSRVST, IDC_COMPRESS, IDC_COMPRESSHELP, IDC_TIMEOUT, 
							IDC_TIMEOUTDEF, IDC_TIMEOUTOPT, IDC_TIMESEC, IDC_TIMEOUTEDIT, IDC_TIMEOUTHELP, 
							IDC_KEEPALIVE, IDC_KEEPALIVESEC, IDC_PINGINTERVAL, IDC_READONLY, IDC_LINK}; 

    wyInt32 count = sizeof(serverids)/ sizeof(serverids[0]);

    ShowOrHide(hwnd, serverids, count, enable);
#ifdef COMMUNITY
	ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);

#endif
}
			
void 
ConnectionBase::ShowHttpTabOptions(HWND hwnd, wyBool enable)
{	
	wyInt32 tunnelids[] = { IDC_TUNNELGRP, IDC_TUNNEL, IDC_TUNNELHELP, IDC_STATICURL,
							IDC_HTTPTIME, IDC_HTTPTIMEST, IDC_HTTPTIMEST2, IDC_URL, 
							IDC_TUNNELST, IDC_HTTPAUTH, IDC_HTTPSOCKET, IDC_HTTPSOCKETPATH, IDC_HTTPSOCKETSTATIC, IDC_HTTPSOCKETGRP
							};


    wyInt32 count = sizeof(tunnelids)/ sizeof(tunnelids[0]);

    ShowOrHide(hwnd, tunnelids, count, enable);

#ifdef COMMUNITY	

	HWND hwndhttp = NULL, hwndhttpbutton = NULL;
		
	//Http grupbox with checkbox
	hwndhttpbutton = GetDlgItem(hwnd, IDC_TUNNEL);
	//Hiding the checkbox
	ShowWindow(hwndhttpbutton, SW_HIDE);

	//Groupbox handle
	hwndhttp = GetDlgItem(hwnd, IDC_TUNNELGRP);

	//HTTP froupbox caption
	SendMessage(hwndhttp, WM_SETTEXT, 0, (LPARAM)COMUNITY_HTTP);

	EnableOrDisable(hwnd, tunnelids, count, wyFalse);

    //enabling the help button
    EnableWindow(GetDlgItem(hwnd, IDC_TUNNELHELP), TRUE);
#else
	if(enable == wyTrue)
	{
		ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
	}
#endif

}

void 
ConnectionBase::ShowSshTabOptions(HWND hwnd, wyBool enable)
{
    wyInt32 sshids[] = {IDC_SSHGRP, IDC_SSHPASSGRP, IDC_SSH, IDC_SSHHELP, IDC_SSHUSER, IDC_SSHUSERST, IDC_SSHPWDST, 
                             IDC_SSHPWD, IDC_SSHHOST, IDC_SSHHOSTST, 
                             IDC_SSHPORTST, IDC_SSHPORT, IDC_FORHOSTST, IDC_FORHOST, 
                             IDC_SSHPASSWORDRADIO, IDC_SSHPRIVATERADIO,
                             IDC_SSHPRIVATEKEYTITLE, IDC_SSHPRIVATEKEY, IDC_SSHSAVEPHRASE,
                             IDC_SSHPRIVATEKEYFILE };

    wyInt32 count = sizeof(sshids)/ sizeof(sshids[0]);

    ShowOrHide(hwnd, sshids, count, enable);

#ifdef COMMUNITY   

	HWND	hwndssh = NULL, hwndsshbutton = NULL;

	//Checkbox for SSH groupbox
	hwndsshbutton = GetDlgItem(hwnd, IDC_SSH);

	//Hide the checkbox
	ShowWindow(hwndsshbutton, SW_HIDE);

	//SSH Groupbox
	hwndssh = GetDlgItem(hwnd, IDC_SSHGRP);

	//Sets the caption for groupbox
	SendMessage(hwndssh, WM_SETTEXT, 0, (LPARAM)COMUNITY_SSH);

	EnableOrDisable(hwnd, sshids, count, wyFalse);

    //enabling the help button
    EnableWindow(GetDlgItem(hwnd, IDC_SSHHELP), TRUE);
	#else
	if(enable == wyTrue)
	{
		ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
	}
#endif
} 

void 
ConnectionBase::ShowSslTabOptions(HWND hwnd, wyBool enable)
{
	wyInt32  sslids[]  = {IDC_SSLGRP, IDC_SSLAUTHGRP, IDC_SSL, IDC_SSLHELP, IDC_SSLAUTH, IDC_SSLCLIENTKEY, 
                         IDC_CLIENTKEY, IDC_SSLCLIENTCER, IDC_CLIENTCER,
                         IDC_SSLCACER, IDC_CACER, IDC_SSLCIPHER, IDC_CIPHER,
                         IDC_SSLKEYFILE, IDC_SSLCERFILE, IDC_SSLCAFILE//, IDC_SSLCIPHERFILE
                         };

    wyInt32 count = sizeof(sslids)/ sizeof(sslids[0]);

    ShowOrHide(hwnd, sslids, count, enable);

#ifdef COMMUNITY	

	HWND		hwndssl = NULL, hwndsslbutton = NULL;

	//Chekbox for SSL groupbox
	hwndsslbutton = GetDlgItem(hwnd, IDC_SSL);

	//Hiding the Chekbox
	ShowWindow(hwndsslbutton, SW_HIDE);

	//Groupbox handle
	hwndssl = GetDlgItem(hwnd, IDC_SSLGRP);

	//Set Groupbox caption for community
	SendMessage(hwndssl, WM_SETTEXT, 0, (LPARAM)COMUNITY_SSL);
	
	EnableOrDisable(hwnd, sslids, count, wyFalse);

    //enabling the help button
    EnableWindow(GetDlgItem(hwnd, IDC_SSLHELP), TRUE);
	#else
	if(enable == wyTrue)
	{
		ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
	}
#endif
}

void 
ConnectionBase::ShowOrHide(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable)
{
    wyInt32 count;
    HWND	hwndc;
			
    for(count = 0; count < arraycount; ++count)
	{
		hwndc = GetDlgItem(hwnd, array[count]);
		if(hwndc)
        {
            if(enable == wyTrue)
                ShowWindow(hwndc, SW_SHOW);
            else
                ShowWindow(hwndc, SW_HIDE);
		}
	}
}

void 
ConnectionBase::EnableOrDisable(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable)
{
    wyInt32 count;
    HWND	hwndc;

    for(count = 0; count < arraycount; ++count)
	{
		hwndc = GetDlgItem(hwnd, array[count]);
		if(hwndc)
        {
            if(enable == wyTrue)
                EnableWindow(hwndc, TRUE);
            else
                EnableWindow(hwndc, FALSE);
        }
	}
}

void
ConnectionBase::HandleCustomComboNotifiction(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyWChar     str[70] = {0};
    HWND        hcb = GetDlgItem(hwnd, IDC_DESC);
    HWND        hwndCombo = NULL;
    wyInt32     id = -1;
    wyInt32     len = -1;

    if(LOWORD(wparam) == IDC_DESC)
    {
        switch(HIWORD(wparam))
        {
        case CBN_EDITUPDATE:
            id = SendMessage(hcb, CB_GETCURSEL, NULL, NULL);
            if(m_conndetaildirty == wyTrue && id != CB_ERR)
			{
                //SendMessage(hcb, CB_GETLBTEXT, id, (LPARAM)str);
                //SetWindowText(hcb, str);
                SendMessage(hcb, CB_SETCURSEL, id, NULL);
                SendMessage(hcb, CCBM_NOSHOWDROPDOWN, NULL, NULL);
                PostMessage(hwnd, SHOW_CONFIRM, 0, 0);
                //m_conndetaildirty = wyFalse;
			}
            break;
        case CBN_EDITCHANGE:
            GetWindowText(hcb, str, 65);
            id = SendMessage(hcb, CB_FINDSTRINGEXACT, -1, (LPARAM)str);
            if(id == CB_ERR)
            {
                OnValidConNameChngState(hwnd, FALSE);
            }
            else
            {
                OnValidConNameChngState(hwnd, TRUE);
            }

            id = TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_CONNTAB));
            
            if(id != 0)
            {
                TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);
                OnConnDialogTabSelChange(hwnd, GetDlgItem(hwnd, IDC_CONNTAB));
            }
            break;
        case CBN_SELENDOK:
            {
                id = SendMessage(hcb, CB_GETCURSEL, NULL, NULL);
                if( id != CB_ERR && m_conndetaildirty == wyFalse)
                {
                    OnValidConNameChngState(hwnd, TRUE);
                    hwndCombo = (HWND) SendMessage(hcb, CCBM_GETCOMBOHWND, NULL, NULL);
                    SendMessage(hwndCombo, CB_GETLBTEXT, (WPARAM)id, (LPARAM)str);
                    SetWindowText(hcb, str);
                    if((m_conndetaildirty == wyTrue) && (m_isnewconnection == wyTrue))
				    {
				        SendMessage(hcb, CB_DELETESTRING, m_newconnid, 0);
				    }
                    HandlerToGetInitialDetails(hwnd);
				    TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);
				    OnConnDialogTabSelChange(hwnd, GetDlgItem(hwnd, IDC_CONNTAB));
				    m_conndetaildirty = wyFalse;
				    m_isnewconnection = wyFalse;
				    m_newconnid = -1;
                }
                
            }
            break;
        case CBN_SELENDCANCEL:
            {
                len = GetWindowText(hcb, str, 64);
                if(len > 0)
                {
                    id = SendMessage(hcb, CB_FINDSTRING, -1, (LPARAM)str);
                    if(id == CB_ERR)
                    {
                        OnValidConNameChngState(hwnd, FALSE);
                    }
                    else
                    {
                        HWND hfocus = GetFocus();
                        if(hfocus != GetDlgItem(hwnd, IDC_CLONECONN))
                        {
                            id = SendMessage(hcb, CB_GETCURSEL, NULL, NULL);
                            if(id != CB_ERR && m_conndetaildirty == wyFalse)
                            {
                                hwndCombo = (HWND) SendMessage(hcb, CCBM_GETCOMBOHWND, NULL, NULL);
                                SendMessage(hwndCombo, CB_GETLBTEXT, id, (LPARAM)str);
                                OnValidConNameChngState(hwnd, TRUE);
                                SetWindowText(hcb, str);
                                if((m_conndetaildirty == wyTrue) && (m_isnewconnection == wyTrue))
				                {
				                    SendMessage(hcb, CB_DELETESTRING, m_newconnid, 0);
				                }
                                HandlerToGetInitialDetails(hwnd);
                                m_conndetaildirty = wyFalse;
				                m_newconnid = -1;
                            }
                        }
                    }
                }
                else
                {
                    OnValidConNameChngState(hwnd, FALSE);
                }
            }
            break;
        case CBN_DROPDOWN:
			if(m_conndetaildirty == wyTrue)
			{
                SendMessage(hwnd, SHOW_CONFIRM, 0, 0);
				m_conndetaildirty = wyFalse;
			}
			break;
		case CBN_CLOSEUP:
			TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);
			OnConnDialogTabSelChange(hwnd, GetDlgItem(hwnd, IDC_CONNTAB));
            GetWindowText(hcb, str, 65);
            id = SendMessage(hcb, CB_FINDSTRINGEXACT, -1, (LPARAM)str); 
            if(id != CB_ERR)
            {
                SendMessage(hcb, CB_SETCURSEL, id, NULL);
                SetFocus(GetDlgItem(hwnd, IDOK));
            }
            break;
        }
    }
}


wyInt32
ConnectionBase::OnComboEraseBkGnd(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    HDC         dc = (HDC)wparam;
    HWND        hwndCombo = (HWND)SendMessage(GetDlgItem(hwnd, lparam), CCBM_GETCOMBOHWND, NULL, NULL);
    RECT        rc;
    HBRUSH      hbr = NULL;
    wyInt32     id = -1;
    COLORREF    clrref;
    wyWChar     str[70] = {0};
    wyInt32     count = 0;


    GetClientRect(hwndCombo, &rc);
    rc.right -= 19;
    rc.left += 2;
    rc.top += 2;
    rc.bottom -= 1;
    hbr = GetSysColorBrush(COLOR_WINDOW);
    FillRect(dc, &rc, hbr);
    //DeleteObject(hbr);

    count = SendMessage(hwndCombo, CB_GETCOUNT, NULL, NULL);

    if(count > 0)
    {
        rc.left += 1;
        rc.right = rc.left + 23;
        rc.top += 4;
        rc.bottom -= 4;

        GetWindowText(hwndCombo, str, 64);
        id = SendMessage(hwndCombo, CB_FINDSTRING, -1, (LPARAM)str);

        if(id != CB_ERR)
        {
            clrref = m_arrayofcolor[id];
        }
        else
        {
            clrref = RGB(255,255,255);
        }

        hbr = CreateSolidBrush(clrref);
        FillRect(dc, &rc, hbr);
        DeleteObject(hbr);
        hbr = CreateSolidBrush(RGB(0,0,0));
        FrameRect(dc, &rc, hbr);
        DeleteObject(hbr);
    }

    return 1;
}

void 
ConnectionBase::OnInitConnDialog(HWND hwnd)
{
	pGlobals->m_pcmainwin->m_connection->CreateConnectDialogWindows(hwnd);
	pGlobals->m_pcmainwin->m_connection->InitConnDialog(hwnd, NULL, wyFalse, wyFalse);
	pGlobals->m_pcmainwin->m_connection->HandlerToGetInitialDetails(hwnd);
	pGlobals->m_pcmainwin->m_connection->m_conndetaildirty = wyFalse;

	//If save password checkbox is checked then focus is on connection combobox else on password field
	if(Button_GetCheck(GetDlgItem(hwnd, IDC_DLGCONNECT_STOREPASSWORD)) == BST_CHECKED)
	{
		SetFocus(GetDlgItem(hwnd, IDC_DESC));
	}
	else
	{
		SetFocus(GetDlgItem(hwnd, IDC_DLGCONNECT_PASSWORD));
	}

	EnableWindow(GetDlgItem(hwnd, IDC_SAVE), FALSE);

    return;
}

wyBool 
ConnectionBase::GetInitialDetails(HWND hdlg)
{
	wyInt32     count, storepwd;
	HWND		hwndcombo;
	wyString    timeout, conn, dirstr, pwdstr, tempstr, isencrypted;
	wyString	codepage, userstr, hoststr, portstr, dbstr;
	wyWChar     directory[MAX_PATH+1]={0}, *lpfileport=0;
	wyChar	 	pwd[SIZE_512]={0};
	wyBool      decodepwd = wyTrue;
	wyUInt32    ret, usecompress = 1, isdefwaittimeout = 1, readonly = 0/*, usecleartext = 0*/;
	ConnectionInfo  conninfo;
	wyInt32 version = 0, versionum = MAJOR_VERSION_INT * 10000 + MINOR_VERSION_INT * 100 + UPDATE_VERSION_INT;
	wyString value("");
	wyString inipath(""), backupfilepath("");
	
	value.AddSprintf("%d", versionum);

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

    if(ret == 0)
		return wyFalse;

	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));

	count  = SendMessage(hwndcombo, CB_GETCOUNT, 0, 0);

	if(!count)
		return wyFalse;

	dirstr.SetAs(directory);

    count = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);
	VERIFY(count != CB_ERR);
	count = SendMessage(hwndcombo, CB_GETITEMDATA, count, 0);
	conn.Sprintf("Connection %u", count);
	
	//version = wyIni::IniGetInt(SECTION_NAME, "version", 0, dirstr.GetString());
	//if (version == 0)
	//{
	//	inipath.SetAs(dirstr.GetString());
	//	backupfilepath.SetAs(dirstr.GetString());

	//	backupfilepath.Strip(strlen("sqlyog.ini") + 1);
	//	backupfilepath.AddSprintf("\\sqlyog_backup.ini");
	//	CopyAndRename(dirstr, inipath, backupfilepath);
	//	
	//}

	////we will create the backup only once, perticulary for v13.1.3 release
	//wyIni::IniWriteString(SECTION_NAME, "version", value.GetString(), dirstr.GetString());

	ret = wyIni::IniGetString(conn.GetString(), "Isencrypted", "0", &isencrypted, dirstr.GetString());
	if (isencrypted.Compare("0") == 0)
	{
		//MigrateAllPassword(conn.GetString(),dirstr.GetString());
		ret = wyIni::IniGetString(conn.GetString(), "Password", "", &pwdstr, dirstr.GetString());
		if (!ret)
		{
			wyIni::IniWriteString(conn.GetString(), "Password", "", dirstr.GetString());
		}
		else
		{
			wyString pwdstr1("");
			pwdstr1.SetAs(pwdstr.GetString());
			MigratePassword(conn.GetString(), dirstr.GetString(), pwdstr1);
			wyIni::IniWriteString(conn.GetString(), "Password", pwdstr1.GetString(), dirstr.GetString());
		}
		////Encrypt the proxy pwd password
		ret = wyIni::IniGetString(conn.GetString(), "ProxyPwd", "", &pwdstr, dirstr.GetString());
		if (!ret)
		{
			wyIni::IniWriteString(conn.GetString(), "ProxyPwd", "", dirstr.GetString());
		}
		else
		{
			wyString pwdstr1("");
			pwdstr1.SetAs(pwdstr.GetString());
			MigratePassword(conn.GetString(), dirstr.GetString(), pwdstr1);
			wyIni::IniWriteString(conn.GetString(), "ProxyPwd", pwdstr1.GetString(), dirstr.GetString());
		}
		//Encrypt the ssh pwd password
		ret = wyIni::IniGetString(conn.GetString(), "SshPwd", "", &pwdstr, dirstr.GetString());
		if (!ret)
		{
			wyIni::IniWriteString(conn.GetString(), "SshPwd", "", dirstr.GetString());
		}
		else
		{
			wyString pwdstr1("");
			pwdstr1.SetAs(pwdstr.GetString());
			MigratePassword(conn.GetString(), dirstr.GetString(), pwdstr1);
			wyIni::IniWriteString(conn.GetString(), "SshPwd", pwdstr1.GetString(), dirstr.GetString());
		}

		//Encrypt the 401 pwd password
		ret = wyIni::IniGetString(conn.GetString(), "401Pwd", "", &pwdstr, dirstr.GetString());
		if (!ret)
		{
			wyIni::IniWriteString(conn.GetString(), "401Pwd", "", dirstr.GetString());
		}
		else
		{
			wyString pwdstr1("");
			pwdstr1.SetAs(pwdstr.GetString());
			MigratePassword(conn.GetString(), dirstr.GetString(), pwdstr1);
			wyIni::IniWriteString(conn.GetString(), "401Pwd", pwdstr1.GetString(), dirstr.GetString());
		}

		wyIni::IniWriteString(conn.GetString(), "Isencrypted", "1", dirstr.GetString());
		conninfo.m_isencrypted = 1; 
		
	}

	/* starting from 4.0 beta 6 we keep the passwords in encoded form so we have delete the earlier password */
	/* first time the password breaks and you get an empty string but after that its OK */
	/* Update: From RC 1 we upgrade the old password if found and then remove the section */
	ret = wyIni::IniGetString(conn.GetString(), "Password", "", &pwdstr, dirstr.GetString());
    if(!ret)
        ret = wyIni::IniGetString(conn.GetString(), "Password01", "", &pwdstr, dirstr.GetString());
    
    strcpy(pwd, pwdstr.GetString());

	/*wyChar *decodedstr = NULL;
	decodedstr = AllocateBuff(512);
	wyInt32 len = DecodeBase64(pwdstr.GetString(), decodedstr);
	pwdstr.SetAsDirect(decodedstr, len);*/
	wyString::DecodeBase64Password(pwdstr);
	DecodePassword(pwdstr);
	
	/*if (decodedstr)
		free(decodedstr);*/
	
	ret = wyIni::IniGetString(conn.GetString(), "User", "root", &userstr, dirstr.GetString());
	ret = wyIni::IniGetString(conn.GetString(), "Host", "localhost", &hoststr, dirstr.GetString());
	ret = wyIni::IniGetString(conn.GetString(), "Port", "3306", &portstr, dirstr.GetString());
	
	ret = wyIni::IniGetString(conn.GetString(), "Database", "", &dbstr, dirstr.GetString());

    /* starting from 5.0 we allow a user to store password */
	storepwd = wyIni::IniGetInt(conn.GetString(), "StorePassword", 1, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_DLGCONNECT_STOREPASSWORD), storepwd);
	
	//Compressed prtocol
	usecompress = wyIni::IniGetInt(conn.GetString(), "compressedprotocol", 1, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_COMPRESS), usecompress);
#ifndef COMMUNITY
//if(pGlobals->m_entlicense.CompareI("Professional") != 0)
//{
	readonly = wyIni::IniGetInt(conn.GetString(), "readonly", 0, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_READONLY), readonly);
//}
//else
//{
//	readonly = 0;
//	Button_SetCheck(GetDlgItem(hdlg, IDC_READONLY), readonly);
//}
#else
{
	readonly = 0;
	Button_SetCheck(GetDlgItem(hdlg, IDC_READONLY), readonly);
}
#endif
	/*usecleartext = wyIni::IniGetInt(conn.GetString(), "cleartextpwd", 0, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_ISCLEARTEXT), usecleartext);*/

	//session wait_timeout(default option)
	isdefwaittimeout = wyIni::IniGetInt(conn.GetString(), "defaulttimeout", 1, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_TIMEOUTDEF), isdefwaittimeout);

	//isdsetwaittimeout = wyIni::IniGetInt(conn.GetString(), "timeoutset", 0, dirstr.GetString());
	Button_SetCheck(GetDlgItem(hdlg, IDC_TIMEOUTOPT), !isdefwaittimeout);

	//Specified value
	ret = wyIni::IniGetString(conn.GetString(), "waittimeoutvalue", "28800", &tempstr, dirstr.GetString());
	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), WM_SETTEXT, 0, (LPARAM)tempstr.GetAsWideChar()));
	
	(isdefwaittimeout == 1) ? EnableWindow(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), FALSE) :
							  EnableWindow(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), TRUE);


	wyIni::IniGetString(conn.GetString(), "keep_alive", "", &tempstr, dirstr.GetString());
	SendMessage(GetDlgItem(hdlg, IDC_PINGINTERVAL), WM_SETTEXT, 0, (LPARAM)tempstr.GetAsWideChar());

	/* now if somebody has modified the ini to set both of them to be true then we make both of them
		false */
	
	// having read it from the sqlyog.ini file we not put in the textbox.
    tempstr.SetAs(hoststr.GetString());
	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_HOST), WM_SETTEXT, 0, (LPARAM)tempstr.GetAsWideChar()));
	

	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_USER), WM_SETTEXT, 0, (LPARAM)userstr.GetAsWideChar()));

    tempstr.SetAs(portstr.GetString());
	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PORT), WM_SETTEXT, 0, (LPARAM)tempstr.GetAsWideChar()));

	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PASSWORD), WM_SETTEXT, 0, (LPARAM)pwdstr.GetAsWideChar()));
	
    tempstr.SetAs(dbstr.GetString());
	VERIFY(ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_DATABASE), WM_SETTEXT, 0, (LPARAM)tempstr.GetAsWideChar()));
	
	ChangeConnStates(hdlg, wyTrue);

	//Get advanced tab details
	GetAdvancedTabDetailsFromFile(&conninfo, (wyChar *)conn.GetString(), dirstr.GetString());
	FillAdvancedTab(hdlg, &conninfo);

	m_currentconn.SetAs(conn.GetString());

	EnableWindow(GetDlgItem(hdlg, IDC_SAVE), FALSE);
	EnableWindow(GetDlgItem(hdlg, IDC_CLONECONN), TRUE);

	/* now it may happen that without saving anything the user has connected 
	   so the field will be removed and the next time he wont get anything
	   to handle such cases we have to by default write the encodedpassword */
	wyChar* encodestr = NULL;
	if(!decodepwd)
	{
		pwdstr.SetAs(pwd);
		EncodePassword(pwdstr);
		encodestr=pwdstr.EncodeBase64Password();
		//strcpy(pwd, pwdstr.GetString());
		dirstr.SetAs(directory);
		wyIni::IniWriteString(conn.GetString(), "Password", encodestr, dirstr.GetString());
	}

	/*Gets the 'AutocompleteTagbuilded' info from .ini to decides the statusbar message to be 'Building tags' or 'Rebuilding tags'	
	'AutocompleteTagbuilded' sets to 1 when the 'building tags'was successful
	*/
	SetAutocompleteTagBuildFlag(&conn, &dirstr);
		    
	//SetFocus(GetDlgItem(hdlg, IDOK));
	if (encodestr)
		free(encodestr);

	return wyTrue;
}

void 
ConnectionBase::GetOtherValues(HWND hdlg, ConnectionInfo *coninfo)
{
    wyInt32 index;
    wyWChar  title[SIZE_1024]={0}, db[SIZE_512]={0};
    HWND    hcb = GetDlgItem(hdlg, IDC_DESC);
    HWND    hwndCombo = (HWND)SendMessage(hcb, CCBM_GETCOMBOHWND, NULL, NULL);
    
    // now get the title.
	index = SendMessage(hcb,CB_GETCURSEL,0,0);
	
	SendMessage(hwndCombo,CB_GETLBTEXT, index, (LPARAM)title);
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_DATABASE), WM_GETTEXT,(WPARAM)SIZE_512 - 1,(LPARAM)db);

	// save the code page details;, write the charset to .ini
	coninfo->m_title.SetAs(title);
	coninfo->m_db.SetAs(db);

	//to trim empty spaces from right, to avoid mysql errors
	coninfo->m_db.RTrim();
}

// Function initializes array of buttons and adds them to the second toolbar 
wyBool
ConnectionBase::CreateOtherToolButtons(HWND hwndsecondtool, HIMAGELIST hsecondiml)
{
	wyInt32     size;
	// array to store all the value so that we can move thru the loop.
	wyInt32 command[] = {
						 IDM_TOOL_ADDUSER, ID_IMPORTEXPORT_DBEXPORTTABLES, 
                         ID_IMEX_TEXTFILE, ID_OPEN_COPYDATABASE, 
                         ID_EXPORT_AS, ID_OBJECT_MAINMANINDEX, ACCEL_MANREL, IDM_SEPARATOR, 
						 ID_FORMATCURRENTQUERY, 
						 IDM_DATASYNC, IDC_DIFFTOOL,  
                         ID_IMPORT_EXTERNAL_DATA, IDC_TOOLS_NOTIFY, 
                         ID_POWERTOOLS_SCHEDULEEXPORT, ID_QUERYBUILDER,
						 ID_SCHEMADESIGNER , IDM_SEPARATOR, ID_STARTTRANSACTION_WITHNOMODIFIER,
						 ID_COMMIT_WITHNOMODIFIER, ID_ROLLBACK_TRANSACTION,
                         };

	wyUInt32 states[][2] = { 
		                    {TBSTATE_ENABLED, /*BTNS_WHOLEDROPDOWN*/TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
		                    {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, BTNS_SEP},
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
                            {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},  
                            {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},
							{TBSTATE_ENABLED, BTNS_SEP}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}}; 

	
	wyInt32 imgres[] = {   
						 IDI_USERMANAGER, IDI_EXPORTDATA, IDI_EXECBATCH, 
						 IDI_COPYDATABASE, IDI_XMLHTML, IDI_MANINDEX, IDI_MANREL, IDI_USERS,   
						 IDI_FORMATQUERY, IDI_DBSYNC, IDI_DIFFTOOL, IDI_ODBC,  
                         IDI_NOTIFICATION, IDI_SCHDEXPORT, IDI_QUERYBUILDER, IDI_SCHEMADESIGNER, 
						 IDI_USERS, IDI_START_TRANSACTION, IDI_COMMIT, IDI_ROLLBACK 
					 };   
   

	SendMessage(hwndsecondtool, TB_SETIMAGELIST, 0,(LPARAM)hsecondiml);
	SendMessage(hwndsecondtool, TB_SETEXTENDEDSTYLE, 0,(LPARAM)TBSTYLE_EX_DRAWDDARROWS);

#ifdef ENTERPRISE
	wyUInt32 statespro[][2] = { 
		                    {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
		                    {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
							{TBSTATE_ENABLED, TBSTYLE_BUTTON}}; 

	wyInt32 imgrespro[] = {   
						 IDI_USERMANAGER, IDI_EXPORTDATA, IDI_EXECBATCH, 
						 IDI_COPYDATABASE, IDI_XMLHTML, IDI_MANINDEX, IDI_MANREL 
					 };

	wyInt32 commandpro[] = {
						 IDM_TOOL_ADDUSER, ID_IMPORTEXPORT_DBEXPORTTABLES, 
                         ID_IMEX_TEXTFILE, ID_OPEN_COPYDATABASE, 
                         ID_EXPORT_AS, ID_OBJECT_MAINMANINDEX, ACCEL_MANREL
                         };
    
	if(m_enttype == ENT_PRO)
	{
		size = sizeof(commandpro)/ sizeof(commandpro[0]);
		AddToolBarButtons(pGlobals->m_hinstance, hwndsecondtool, hsecondiml, commandpro, size, statespro, imgrespro);
	}

	else
	{
		size = sizeof(command)/ sizeof(command[0]);
		AddToolBarButtons(pGlobals->m_hinstance, hwndsecondtool, hsecondiml, command, size, states, imgres);
	}

#else	
	size = sizeof(command)/ sizeof(command[0]);
	AddToolBarButtons(pGlobals->m_hinstance, hwndsecondtool, hsecondiml, command, size, states, imgres);
#endif

	/* Now set and show the toolbar */
	SendMessage(hwndsecondtool, TB_AUTOSIZE, 0, 0);
	ShowWindow(hwndsecondtool, TRUE);

	return wyTrue;
}

wyBool
ConnectionBase::EnableWindowItems(HMENU hmenu)
{
    wyInt32         size, count;
	wyInt32         nid[] = { IDM_WINDOW_TILE,
                              IDM_WINDOW_CASCADE,
                              IDM_WINDOWS_ICONARRANGE };

	wyUInt32        state;
	MDIWindow	*	pcquerywnd;

	VERIFY(pcquerywnd = GetActiveWin());

	// fisrt disable all the items in the database.
	state = MF_ENABLED;

	size = sizeof(nid) / sizeof(nid[0]);

	for(count=0; count < size; count++)
		EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

    return wyTrue;
}

wyBool
ConnectionBase::EnableHelpItems(HMENU hmenu)
{
    wyInt32         size, count;
	wyInt32         nid[] = { IDM_HELP_HELP, IDM_HELP_SHORT,IDM_HELP_ABOUT };

	wyUInt32        state;
	MDIWindow	*	pcquerywnd;

	VERIFY(pcquerywnd = GetActiveWin());

	// fisrt disable all the items in the database.
	state = MF_ENABLED;

	size = sizeof(nid)/sizeof(nid[0]);

	for(count=0; count < size; count++)
		EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

    return wyTrue;
}

 //Checks the index is of window menu
wyBool     
ConnectionBase::IsWindowMenu(wyInt32 index)
{
     if(index == MNUWINDOW_INDEX)
        return wyTrue;

     return wyFalse;
}

// Function to set various parts in the status bar of the main window.
// There are only two parts. One showing messages and one showing how many active connection.

wyBool
ConnectionBase::SetStatusParts(HWND hwndstatus)
{
	wyInt32		ret;
    RECT		rc, rcexetime = {0}, rctotaltime = {0}, rctotrows = {0}, rcline = {0}, rcconns = {0}, rctext = {0} ;
    HDC			hdc;
    HFONT		hfont = NULL;
	wyString	exestr("Exec: 00:00:00:000       ");
	wyString	totstr("Total: 00:00:00:000  ");
	wyString	totrowstr("100000000 row(s)  ");
    wyString    linestr("Ln 100000, Col 100000     ");
    wyString    connstr("Connections:99999      ");

	GetClientRect(hwndstatus, &rc);

    if(!m_hstatusfont)
    {
        hdc = GetDC(m_hwndlink);
        hfont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        OnStatusBarWmCtlColorStatic(hwndstatus, (WPARAM)hdc, (LPARAM)m_hwndlink);
        SelectObject(hdc, hfont);
        ReleaseDC(m_hwndlink, hdc);
    }

    hdc = GetDC(hwndstatus);
    hfont = (HFONT)SelectObject(hdc, (HGDIOBJ)m_hstatusfont);

    DrawText(hdc, m_statuslink.GetAsWideChar(), -1, &rctext, DT_CALCRECT);
    rctext.left = rctext.right;
    DrawText(hdc, L"WW", -1, &rctext, DT_CALCRECT);
    SelectObject(hdc, (HGDIOBJ)hfont);

	//size of Exec time , total time and total rows 
    hfont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    DrawText(hdc, exestr.GetAsWideChar(), -1, &rcexetime, DT_CALCRECT);
    DrawText(hdc, totstr.GetAsWideChar() , -1, &rctotaltime, DT_CALCRECT);
    DrawText(hdc, totrowstr.GetAsWideChar(), -1, &rctotrows, DT_CALCRECT);
    DrawText(hdc, linestr.GetAsWideChar(), -1, &rcline, DT_CALCRECT);
    DrawText(hdc, connstr.GetAsWideChar(), -1, &rcconns, DT_CALCRECT);
    SelectObject(hdc, (HGDIOBJ)hfont);   
    ReleaseDC(hwndstatus, hdc);

    m_sbparts[0] = (rc.right-rc.left) - (rcexetime.right + rctotaltime.right + rctotrows.right + rcline.right + rcconns.right + rctext.right)-250;
    m_sbparts[1] = m_sbparts[0] + rcexetime.right;
    m_sbparts[2] = m_sbparts[1] + rctotaltime.right;
    m_sbparts[3] = m_sbparts[2] + rctotrows.right;
    m_sbparts[4] = m_sbparts[3] + rcline.right;
    m_sbparts[5] = m_sbparts[4] + rcconns.right+250;
    m_sbparts[6] = -1;
	VERIFY(ret = SendMessage(hwndstatus, SB_SETPARTS,(WPARAM)pGlobals->m_statusbarsections, (LPARAM)m_sbparts));

	return wyTrue;
}

void
ConnectionBase::ResizeStatusWindow(HWND hwndmain, HWND hwndstatus)
{
	wyInt32 vpos, hpos, width, height;
	RECT    rcmain;

	// Get the client rectangle of the main window and then move the status bar 
	// at the bottom.
	VERIFY(GetClientRect(hwndmain, &rcmain));

	vpos	= rcmain.bottom - STATUS_BAR_HEIGHT;
	hpos	= rcmain.left;
	height	= STATUS_BAR_HEIGHT;
	width	= rcmain.right - rcmain.left;

	VERIFY(MoveWindow(hwndstatus, hpos, vpos, width, height, FALSE));

    VERIFY(GetClientRect(hwndstatus, &rcmain));

    SendMessage(m_hwndlink, WM_SETTEXT, 0, (LPARAM)m_statuslink.GetAsWideChar());
    ShowWindow(m_hwndlink, TRUE);
    SetStatusParts(hwndstatus);
    
    vpos	= rcmain.bottom - STATUS_BAR_HEIGHT;
	hpos	= rcmain.left+m_sbparts[5]+2;
	height	= STATUS_BAR_HEIGHT;
	width	= rcmain.right - rcmain.left - m_sbparts[5] - 20;

	VERIFY(MoveWindow(m_hwndlink, hpos+2, vpos+2, width, height, FALSE));

    SendMessage(m_hwndlink, WM_SETTEXT, 0, (LPARAM)m_statuslink.GetAsWideChar());
    ShowWindow(m_hwndlink, TRUE);
	
	return;
}

void 
ConnectionBase::ManageStatusBar(HWND hwndstatus)
{
     VERIFY(m_hwndlink = CreateWindow(L"STATIC", L"", WS_CHILD | SS_LEFTNOWORDWRAP | SS_NOTIFY | WS_VISIBLE, 
                                    0,0,0,0,
							       hwndstatus, (HMENU)IDC_LINK,
								   (HINSTANCE)pGlobals->m_hinstance, NULL));

    m_wpstatusbarorigproc = (WNDPROC)SetWindowLongPtr(hwndstatus, GWLP_WNDPROC, 
                            (LONG_PTR)StatusBarDialogProc);

    SetWindowLongPtr(hwndstatus, GWLP_USERDATA, (LONG_PTR)this);

    m_wpstatusbatstaticcorigproc = (WNDPROC)SetWindowLongPtr(m_hwndlink, GWLP_WNDPROC,
                                     (LONG_PTR)StatusBarStaticDlgProc);	

    SetWindowLongPtr(m_hwndlink, GWLP_USERDATA, (LONG_PTR)this);
}

// Callback function to handle the about dialog box.
wyInt32 CALLBACK
ConnectionBase::StatusBarDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    HBRUSH hbr;
    ConnectionBase *	conn = (ConnectionCommunity *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
    case WM_CTLCOLORSTATIC:
        return conn->OnStatusBarWmCtlColorStatic(hwnd, wParam, lParam);

    case WM_COMMAND:
        conn->OnStatusBarWmCommand(hwnd, wParam);
        return 0;
	case WM_DESTROY:
		conn->DestroyFonts();
		break;

    case WM_ERASEBKGND:
        if(wyTheme::GetBrush(BRUSH_FRAMEWINDOW, &hbr) ||
            wyTheme::GetBrush(BRUSH_BTNFACE, &hbr))
        {
            GetClientRect(hwnd, &rect);
            FillRect((HDC)wParam, &rect, hbr);
            return 1;
        }      
        break;
	}

	return CallWindowProc(conn->m_wpstatusbarorigproc, hwnd, message, wParam, lParam);
}

void 
ConnectionBase::DestroyFonts()
{
	if(m_haboutfont)
	{
		DeleteFont(m_haboutfont);
		m_haboutfont = NULL;
	}

	if(m_haboutfont2)
	{
		DeleteFont(m_haboutfont2);
		m_haboutfont2 = NULL;
	}

	if(m_hreginfofont)
	{
		DeleteFont(m_hreginfofont);
		m_hreginfofont = NULL;
	}

	if(m_hstatusfont)
	{
		DeleteFont(m_hstatusfont);
		m_hstatusfont = NULL;
	}
}

// Subclassed window procedure for the link static control so that we can change the 
// cursor to something better.
LRESULT CALLBACK
ConnectionBase::StatusBarStaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ConnectionBase * conn =(ConnectionCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
        {
		    SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
            pGlobals->m_pcmainwin->AddTextInStatusBar(L"http://www.webyog.com");
        }
		return 0;
	}

	return CallWindowProc(conn->m_wpstatusbatstaticcorigproc, hwnd, message, wParam, lParam);
}

void
ConnectionBase::OnToolTipInfo(LPNMTTDISPINFO lpnmtdi, wyWChar *string)
{
    switch(lpnmtdi->hdr.idFrom)
	{
    case ID_FORMATCURRENTQUERY:
		wcscpy(string, _(L"Format Query"));
		break; 

	case ID_POWERTOOLS_SCHEDULEEXPORT:
     	wcscpy(string, _(L"Scheduled Backup Wizard (Ctrl+Alt+S)"));
		break;

    case ID_IMPORT_EXTERNAL_DATA:
		wcscpy(string, _(L"Import External Data Wizard(Ctrl+Alt+O)"));
		break;
		
	case IDC_DIFFTOOL:
		wcscpy(string, _(L"Schema Synchronization Tool (Ctrl+Q)"));
		break;

	case IDM_DATASYNC:
		wcscpy(string, _(L"Database Synchronization Wizard (Ctrl+Alt+W)"));
		break;

	case IDC_TOOLS_NOTIFY:
		wcscpy(string, _(L"SQL Scheduler and Reporting Wizard (Ctrl+Alt+N)"));
		break;

	case ID_QUERYBUILDER:
		wcscpy(string, _(L"New Query Builder (Ctrl+K)"));
		break;

	case ID_SCHEMADESIGNER:
		wcscpy(string, _(L"New Schema Designer (Ctrl+Alt+D)"));
		break;
	case ID_STARTTRANSACTION_WITHNOMODIFIER:
		wcscpy(string, _(L"Start Transaction"));
		break;
	case ID_COMMIT_WITHNOMODIFIER:
		wcscpy(string, _(L"Commit Transaction"));
		break;
	case ID_ROLLBACK_TRANSACTION:
		wcscpy(string, _(L"Rollback Transaction"));
		break;
    }

	return;
}

wyBool
ConnectionBase::EnableToolButtonsAndCombo(HWND hwndtool, HWND hwndsecondtool, HWND hwndcombo, wyBool enable, wyBool isexec)
{
    wyInt32     itemcount;
	wyInt32     state = (enable == wyTrue)?(TBSTATE_ENABLED):(TBSTATE_INDETERMINATE);
    
	wyInt32		tbid[] =  {IDM_FILE_CONNECT, ID_NEW_EDITOR , IDM_EXECUTE, ACCEL_QUERYUPDATE,
                           ID_STOPQUERY, ACCEL_EXECUTEALL, IDM_REFRESHOBJECT, IDM_COLLAPSEOBJECT };

	wyInt32		tb2id[] = {	ID_FORMATCURRENTQUERY,			IDC_DIFFTOOL, 
							ID_QUERYBUILDER,				ID_SCHEMADESIGNER, 
							ID_IMPORTEXPORT_DBEXPORTTABLES, ID_IMEX_TEXTFILE, 
							ID_OBJECT_CREATESCHEMA,			ID_OBJECT_INSERTUPDATE,					 
							ID_OBJECT_TABLEEDITOR, 			ID_OPEN_COPYDATABASE,
							ID_EXPORT_AS,					ID_OBJECT_COPYTABLE,			
							ID_OBJECT_MAINMANINDEX, 		ACCEL_MANREL,
							ID_FORMATCURRENTQUERY,			IDC_DIFFTOOL,					 
							ID_QUERYBUILDER,				ID_SCHEMADESIGNER,
							IDM_TOOL_ADDUSER,				ID_STARTTRANSACTION_WITHNOMODIFIER,
							ID_COMMIT_WITHNOMODIFIER,		ID_ROLLBACK_TRANSACTION
						};
	
	/* Enable/Disable all the toolbuttons */
	for(itemcount = 0; itemcount < (sizeof(tbid)/sizeof(tbid[0])); itemcount++)
		SendMessage(hwndtool, TB_SETSTATE,(WPARAM)tbid[itemcount], state);

	for(itemcount = 0; itemcount < (sizeof(tb2id)/sizeof(tb2id[0])); itemcount++)
		SendMessage(hwndsecondtool, TB_SETSTATE,(WPARAM)tb2id[itemcount], state);
    
	return wyTrue;
}
void 
ConnectionBase::ShowServerOptions(HWND hdlg)
{
    ShowSeverTabOptions(hdlg, wyTrue);
    ShowHttpTabOptions(hdlg);
    ShowSshTabOptions(hdlg);
    ShowSslTabOptions(hdlg);
    
    //show advanced tab option
	ShowAdvanceTabOptions(hdlg);
}

VOID
ConnectionBase::SetAutocompleteTagBuildFlag(wyString *connsection, wyString *dirstr)
{
	wyInt32 ret = 0;

	ret = wyIni::IniGetInt(connsection->GetString(), "AutocompleteTagbuilded", 0, dirstr->GetString());	
	m_isbuiltactagfile = (ret) ? wyTrue : wyFalse;
	m_consectionname.SetAs(*connsection);
}

VOID
ConnectionBase::GetSQLyogUltimateDialog()
{	
	HINSTANCE hinstance;

	hinstance = pGlobals->m_hinstance;
    	
	DialogBox(hinstance, MAKEINTRESOURCE(IDD_GETSQLYOGULTIMATE),pGlobals->m_pcmainwin->m_hwndmain , GetUltimateDialogProc);
	return;
}

//Posp up "Get-Ent/Ult" dialog
VOID
ConnectionBase::GetSQLyogEntDialog()
{
	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_GETSQLYOGENTERPRISE),
				pGlobals->m_pcmainwin->m_hwndmain , 
				GetEntDialogProc,
				(LPARAM)this);
	
	return;
}

INT_PTR CALLBACK
ConnectionBase::GetUltimateDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
    case WM_INITDIALOG:
        LocalizeWindow(hwnd);
        break;
	
	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
			case IDCANCEL:
			case IDC_NOTNOW: 
				yog_enddialog(hwnd, 0);
				break;
			case IDC_BUYNOW:
				if(HIWORD(wparam) == STN_CLICKED)
				{
					ShellExecute(NULL, L"open", TEXT(BUYURL_POWERTOOLS), NULL, NULL, SW_SHOWNORMAL);
					yog_enddialog(hwnd, 1);
				}
				break;
		}
		break;
	}
		
	return 0;
}

INT_PTR CALLBACK
ConnectionBase::GetEntDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ConnectionBase *con = (ConnectionBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);			
		break;

	case WM_INITDLGVALUES:
		con->SetGetEntCaptionText(hwnd);	
		break;

	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
			case IDCANCEL:
			case IDC_NOTNOW: 
				yog_enddialog(hwnd, 0);
				break;
			case IDC_BUYNOW:
				if(HIWORD(wparam) == STN_CLICKED)
				{
					ShellExecute(NULL, L"open", TEXT(BUYURL_POWERTOOLS), NULL, NULL, SW_SHOWNORMAL);
					yog_enddialog(hwnd, 1);
				}
				break;
		}
		break;
	}
		
	return 0;
}

VOID
ConnectionBase::SetGetEntCaptionText(HWND hwnddlg)
{	
#ifdef ENTERPRISE	
		return;
#else

	HWND hwndtxt = NULL, hwndentft = NULL;
	
	if(!hwnddlg ||( m_powertoolsID != ID_REBUILDTAGS && 
		m_powertoolsID != ID_EDIT_LISTALLTAGS && 
		m_powertoolsID != ID_EDIT_LISTMATCHINGTAGS &&
        m_powertoolsID != ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS))

		return;

	SetWindowText(hwnddlg, _(L"Get SQLyog PROFESSIONAL/ENTERPRISE/ULTIMATE"));

	VERIFY(hwndtxt = GetDlgItem(hwnddlg, IDC_GETENTTEXT));
	VERIFY(hwndentft = GetDlgItem(hwnddlg, IDC_ENTFEATURE));

	if(hwndentft)
		SetWindowText(hwndentft, _(L"This feature is available in PROFESSIONAL/ENTERPRISE/ULTIMATE version of SQLyog."));

	if(hwndtxt)
		SetWindowText(hwndtxt, GETENT_DLG_TEXT);	
#endif
	return;
}

void 
ConnectionBase::ShowAdvanceTabOptions(HWND hwnd, wyBool enable)
{
	wyInt32  othersids[]  = {IDC_COLORTXT1, IDC_COLORTXT2, 
							IDC_COLORCOMBO, IDC_COLORTXT3,
							IDC_COLORCOMBO3, IDC_EDITMODE, 
							IDC_MSQLOPTIONSGRP, IDC_MODEGLOBAL, IDC_MODEVALUE,
                            IDC_INITCOMMAND, IDC_EDITINITCOMMAND, 
                            IDC_INITCOMMANDDETAIL, IDC_INITCOMMANDHELP

    };

    wyInt32 count = sizeof(othersids)/ sizeof(othersids[0]);

    wyInt32 httprelatedids[] = {IDC_INITCOMMAND, IDC_EDITINITCOMMAND, 
                            IDC_INITCOMMANDDETAIL
    };
    
    
    ShowOrHide(hwnd, othersids, count, enable);	

    enable = SendMessage(GetDlgItem(hwnd, IDC_TUNNEL), BM_GETCHECK, 0, 0) == BST_UNCHECKED && enable ? wyTrue : wyFalse;
    EnableOrDisable(hwnd, httprelatedids, sizeof(httprelatedids)/ sizeof(httprelatedids[0]), enable);

}


void
ConnectionBase::SaveAdvancedTabDetails(HWND hdlg, const wyChar *conn, const wyChar *directory)
{
     wyChar		color[32];
	 wyWChar	*wdata;
	 wyString	strdata;
	 wyInt32	textlen = 0;
     wyUInt32   keepalive;
	 
     //Write the connection color details to ini
	 _snprintf(color, 31, "%lu", m_rgbobbkcolor);
	wyIni::IniWriteString(conn, "ObjectbrowserBkcolor",	color, directory);
	_snprintf(color, 31, "%lu", m_rgbobfgcolor);
	wyIni::IniWriteString(conn, "ObjectbrowserFgcolor",	color, directory);

    if(!hdlg)
        return;

	//Writing the SQL_MODE to ini
	{
		//If global option is checked
		if(SendMessage(GetDlgItem(hdlg, IDC_MODEGLOBAL), BM_GETCHECK,(WPARAM)0, (LPARAM)0))
		{
			wyIni::IniWriteInt(conn, "sqlmode_global", 1, directory);
		}
		else
		{
			wyIni::IniWriteInt(conn, "sqlmode_global", 0, directory);
		}

		textlen = SendMessage(GetDlgItem(hdlg, IDC_EDITMODE), WM_GETTEXTLENGTH,(WPARAM)0, (LPARAM)0);
		wdata = AllocateBuffWChar(textlen + 1);

		if(!wdata)
			return;
		
        SendMessage(GetDlgItem(hdlg, IDC_EDITMODE), WM_GETTEXT,(WPARAM)textlen + 1, (LPARAM)wdata);
		
        if(wcslen(wdata))
		    strdata.SetAs(wdata);
        else
            strdata.SetAs("");

		free(wdata);
		wyIni::IniWriteString(conn, "sqlmode_value", strdata.GetString(), directory);
	}
    
    /// Writing Init Command to the .ini file
    {
        textlen = SendMessage(GetDlgItem(hdlg, IDC_EDITINITCOMMAND), WM_GETTEXTLENGTH,(WPARAM)0, (LPARAM)0);
		wdata = AllocateBuffWChar(textlen + 1);

		if(!wdata)
			return;

		SendMessage(GetDlgItem(hdlg, IDC_EDITINITCOMMAND), WM_GETTEXT,(WPARAM)textlen + 1, (LPARAM)wdata);

        if(wcslen(wdata))
		    strdata.SetAs(wdata);
        else
            strdata.SetAs("");

		free(wdata);
		wyIni::IniWriteString(conn, "init_command", strdata.GetString(), directory);
    }

    //save keepalive interval
    keepalive = GetDlgItemInt(hdlg, IDC_PINGINTERVAL, NULL, wyFalse);

    wyIni::IniWriteInt(conn, "keep_alive", keepalive, directory);

}

void
ConnectionBase::GetAdvancedTabDetailsFromFile(ConnectionInfo *conn, wyChar *connspe, const wyChar *path)
{
	wyString	sqlmodestr, strinitcommand;
	wyInt32		mode;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr;


	wyWChar     *lpfileport=0;

	if(!path)
	{
		if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
		{
			return;
		}

		dirstr.SetAs(directory);
	}
	else
	{
		dirstr.SetAs(path);
	}

	//read the color from ini,if no color present then make it white
	conn->m_rgbconn  = wyIni::IniGetInt(connspe, "ObjectbrowserBkcolor", COLOR_WHITE, dirstr.GetString());
	conn->m_rgbfgconn  = wyIni::IniGetInt(connspe, "ObjectbrowserFgcolor", conn->m_rgbconn ^ COLOR_WHITE, dirstr.GetString());
	m_rgbconnection = conn->m_rgbconn;
	m_rgbconnectionfg = conn->m_rgbfgconn;

	/// Get sql_mode value
	wyIni::IniGetString(connspe, "sqlmode_value", "", &sqlmodestr, dirstr.GetString());
	conn->m_sqlmode.SetAs(sqlmodestr);
	conn->m_sqlmode.LTrim();
	conn->m_sqlmode.RTrim();

    /// Getting init_command value
    wyIni::IniGetString(connspe, "init_command", "", &strinitcommand, dirstr.GetString());
	conn->m_initcommand.SetAs(strinitcommand);
	conn->m_initcommand.LTrim();
	conn->m_initcommand.RTrim();

	//Global mode or not
	mode = wyIni::IniGetInt(connspe, "sqlmode_global", 1, dirstr.GetString());
	conn->m_isglobalsqlmode = (mode == 1) ? wyTrue : wyFalse;

    //keep alive interval
    conn->m_keepaliveinterval= wyIni::IniGetInt(connspe, "keep_alive", 0, dirstr.GetString());
}

void    
ConnectionBase::FillAdvancedTab(HWND hwnd, ConnectionInfo *conninfo)
{
    EnableWindow(GetDlgItem(hwnd, IDC_COLORTXT2) , TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_COLORTXT1) , TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_COLORTXT3) , TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_COLORCOMBO), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_COLORCOMBO3), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_MODEVALUE), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_MODEGLOBAL), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_INITCOMMAND), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_EDITINITCOMMAND), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_INITCOMMANDDETAIL), TRUE);
	/*
#ifndef COMMUNITY
	ShowWindow(GetDlgItem(hwnd, IDC_COMPRESSHELP), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_READONLY), SW_HIDE);
#endif
	*/
	//fill connection color
	ColorComboInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO));
	ColorComboFgInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO3));
	
	//Set SQL mode value	
	SendMessage(GetDlgItem(hwnd, IDC_EDITMODE), WM_SETTEXT,(WPARAM)0, 
		(LPARAM)conninfo->m_sqlmode.GetAsWideChar());	

	if(conninfo->m_isglobalsqlmode == wyTrue)
	{
		SendMessage(GetDlgItem(hwnd, IDC_MODEGLOBAL), BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0); 
        SendMessage(GetDlgItem(hwnd, IDC_MODEVALUE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0); 
		EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), FALSE);
	}
	else
	{
        SendMessage(GetDlgItem(hwnd, IDC_MODEVALUE), BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0); 
        SendMessage(GetDlgItem(hwnd, IDC_MODEGLOBAL), BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
		EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), TRUE);
	}

    /// Setting init_command
    SendMessage(GetDlgItem(hwnd, IDC_EDITINITCOMMAND), WM_SETTEXT,(WPARAM)0, (LPARAM)conninfo->m_initcommand.GetAsWideChar());

    if(conninfo->m_keepaliveinterval)
    {
        SetDlgItemInt(hwnd, IDC_PINGINTERVAL, conninfo->m_keepaliveinterval, FALSE);
    }
    else
    {
        SendMessage(GetDlgItem(hwnd, IDC_PINGINTERVAL), WM_SETTEXT, 0 , (LPARAM)L"");
    }
}


//Add the UUID to regestry
VOID
ConnectionBase::HandleApplicationUUID()
{
	wyChar  name[1024 + 1]={0};
	HKEY		key;
    DWORD       dwnametype = REG_BINARY, dwnamedata = 1024, dwdisposition;
	UUID		uuid;
	UCHAR		*pszUuid = 0;


	if(RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(REGKEY), REG_OPTION_NON_VOLATILE, 
                                      KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS)
	{
		VERIFY((RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(REGKEY), 0, 
								NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 
								NULL, &key, &dwdisposition))== ERROR_SUCCESS);
	}


	VERIFY((RegQueryValueEx(key, TEXT(REGAPPID), 0, &dwnametype, (UCHAR*)name, &dwnamedata)== ERROR_SUCCESS));

	if(strlen(name))
	{
		pGlobals->m_appuuid.SetAs(name);
	}

	else
	{
		::UuidCreate(&uuid);
	
		if (UuidToStringA(&uuid, &pszUuid) == RPC_S_OK) 
		{
			pGlobals->m_appuuid.SetAs((const wyChar*)pszUuid);

			RpcStringFree((unsigned short **)&pszUuid);
		}

		VERIFY(RegSetValueEx(key, TEXT(REGAPPID), 0, REG_BINARY,
							(UCHAR*)pGlobals->m_appuuid.GetString(), 
							pGlobals->m_appuuid.GetLength()) == ERROR_SUCCESS);
	}

    RegCloseKey(key);

}

/*Handling MySQL tab of Connection dialog
*/
void
ConnectionBase::SaveServerDetails(HWND hwnd, const wyChar *conn, const wyChar *directory)
{
    wyInt32     ret = 1, value = 1;
    wyWChar     temp[SIZE_1024] = {0};
	wyString    tempstr, isencrypted;

    // Server Host
    SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_HOST), WM_GETTEXT, SIZE_512-1,(LPARAM)temp);
	tempstr.SetAs(temp);
	tempstr.RTrim();
    ret = wyIni::IniWriteString(conn, "Host", tempstr.GetString(), directory);

    // Server User
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_USER), WM_GETTEXT, SIZE_512-1,(LPARAM)temp);
    tempstr.SetAs(temp);
	tempstr.RTrim();
    ret = wyIni::IniWriteString(conn, "User", tempstr.GetString(), directory);

    // Delete the older profile string
    DeletePrivateProfileString("Password01", (wyChar *)conn, (wyChar *)directory);
    
    // Save the "Save Password" check box value
    ret	= SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_STOREPASSWORD), BM_GETCHECK, 0, 0);
    swprintf(temp, L"%d", ret);
    wyIni::IniWriteString(conn, "StorePassword", (LPCSTR)temp, directory);
	
    // Save Server Password
    if(ret)
    {
        ret	= SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_PASSWORD), WM_GETTEXT, sizeof(temp), (LPARAM)temp);
		tempstr.SetAs(temp);
        EncodePassword(tempstr);

		//wyChar *encodestr = NULL;
		/*encodestr = AllocateBuff(512);
		EncodeBase64(tempstr.GetString(), tempstr.GetLength(), &encodestr);*/
		wyChar *encodestr = tempstr.EncodeBase64Password();
		ret = wyIni::IniWriteString(conn, "Password", encodestr, directory);
		wyIni::IniWriteString(conn, "Isencrypted", "1", directory);

		if(encodestr)
			free(encodestr);
    }
    else
    {
		//Clear the password in ini
        ret = wyIni::IniWriteString(conn, "Password", "", directory);
    }

	// Server Port
	SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_PORT), WM_GETTEXT, SIZE_512-1,(LPARAM)temp);
	tempstr.SetAs(temp);
    ret = wyIni::IniWriteString(conn, "Port", tempstr.GetString(), directory);

    // Database name
    SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_DATABASE), WM_GETTEXT, SIZE_512-1,(LPARAM)temp);
	tempstr.SetAs(temp);
	tempstr.RTrim();
    ret = wyIni::IniWriteString(conn, "Database", tempstr.GetString(), directory);

	//Compressed protocol
	ret = Button_GetState(GetDlgItem(hwnd, IDC_COMPRESS));
	(ret == BST_CHECKED) ? value = 1 : value = 0;
	ret =	wyIni::IniWriteInt(conn, "compressedprotocol", value, directory);
#ifndef COMMUNITY
	//readonly
	ret = Button_GetState(GetDlgItem(hwnd, IDC_READONLY));
	(ret == BST_CHECKED) ? value = 1 : value = 0;
	ret =	wyIni::IniWriteInt(conn, "readonly", value, directory);
#endif
	//Clear text pwd
	/*ret = Button_GetState(GetDlgItem(hwnd, IDC_ISCLEARTEXT));
	(ret == BST_CHECKED) ? value = 1 : value = 0;
	ret =	wyIni::IniWriteInt(conn, "cleartextpwd", value, directory);*/


	//Wait_TimeOut
	ret = Button_GetState(GetDlgItem(hwnd, IDC_TIMEOUTDEF)); //Default option
	(ret == BST_CHECKED) ? value = 1 : value = 0;
	ret = wyIni::IniWriteInt(conn, "defaulttimeout", value, directory);

	SendMessage(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), WM_GETTEXT, 1024-1,(LPARAM)temp); //Specified timeout value
	tempstr.SetAs(temp);
	ret = wyIni::IniWriteString(conn, "waittimeoutvalue", tempstr.GetString(), directory);

	SendMessage(GetDlgItem(hwnd, IDC_PINGINTERVAL), WM_GETTEXT, 1024-1,(LPARAM)temp); //Specified timeout value
	tempstr.SetAs(temp);
	if(wcslen(temp))
		wyIni::IniWriteString(conn, "keep_alive", tempstr.GetString(), directory);

	//if the flag is already updated, then skip this section, only for clone connnection, use this block
	ret = wyIni::IniGetString(conn, "Isencrypted", "0", &isencrypted, directory);
	if (isencrypted.Compare("0") == 0)
	{
		if (m_isencrypted_clone)
		{
			wyIni::IniWriteString(conn, "Isencrypted", "1", directory);
		}

	}

}

//Handle the common options with the Connection dialog
VOID
ConnectionBase::HandleCommonConnectOptions(HWND hwnd, ConnectionInfo *dbname, wyInt32 id)
{
	switch(id)
	{
#ifndef COMMUNITY
	case IDC_READONLY:
		if(SendMessage(GetDlgItem(hwnd, IDC_READONLY), BM_GETCHECK, 0, 0)== BST_CHECKED)
		//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
				dbname->m_isreadonly = wyTrue;
		else
			dbname->m_isreadonly = wyFalse;

		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);	
		EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);	
		m_conndetaildirty = wyTrue;
		break;
#endif
	case IDC_COMPRESS:
		if(SendMessage(GetDlgItem(hwnd, IDC_COMPRESS), BM_GETCHECK, 0, 0)== BST_CHECKED)
			dbname->m_iscompress = wyTrue;
		else
			dbname->m_iscompress = wyFalse;

		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);	
		EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);	
		m_conndetaildirty = wyTrue;
		break;

	/*case IDC_ISCLEARTEXT:
		if(SendMessage(GetDlgItem(hwnd, IDC_ISCLEARTEXT), BM_GETCHECK, 0, 0)== BST_CHECKED)
			dbname->m_ispwdcleartext = wyTrue;
		else
			dbname->m_ispwdcleartext = wyFalse;

		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);	
		EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);	
		m_conndetaildirty = wyTrue;
		break;*/

	case IDC_DLGCONNECT_STOREPASSWORD:
		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);	
		EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
		m_conndetaildirty = wyTrue;		
		break;

	case IDC_TIMEOUTDEF:
		if(HandleTimeoutOption(hwnd, dbname, IDC_TIMEOUTDEF, IDC_TIMEOUTOPT) == wyTrue)
			m_conndetaildirty = wyTrue;

		break;

	case IDC_TIMEOUTOPT:
		if(HandleTimeoutOption(hwnd, dbname,  IDC_TIMEOUTOPT, IDC_TIMEOUTDEF) == wyTrue)
			m_conndetaildirty = wyTrue;
		break;	

	case IDC_EDITCONN:
		ChangeConnName(hwnd);
		break;

	case IDC_MODEGLOBAL:
		{
			if(SendMessage(GetDlgItem(hwnd, IDC_MODEGLOBAL), BM_GETCHECK, 0, 0)== BST_CHECKED) 
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), FALSE);
				dbname->m_isglobalsqlmode = wyTrue;
			}
			else
			{
				dbname->m_isglobalsqlmode = wyFalse;
				EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), TRUE);
			}
			
			EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
			m_conndetaildirty = wyTrue;
		}		
		break;

    case IDC_MODEVALUE:
        if(SendMessage(GetDlgItem(hwnd, IDC_MODEVALUE), BM_GETCHECK, 0, 0)== BST_CHECKED) 
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), TRUE);
                dbname->m_isglobalsqlmode = wyFalse;
			}
			else
			{
                dbname->m_isglobalsqlmode = wyTrue;
				EnableWindow(GetDlgItem(hwnd, IDC_EDITMODE), FALSE);
			}
			
			EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
			m_conndetaildirty = wyTrue;
        break;

	}
	return;
}

//For marking the engine type menu item
VOID
ConnectionBase::HandleTableEngineMenu(HMENU hmenuhandle, wyInt32 index)
{
	MDIWindow *wnd;
	wyString engine;

	wnd = GetActiveWin();
	if(!wnd || !wnd->m_pcqueryobject)
	{
		return;
	}
    
	wnd->m_pcqueryobject->SelectTableEngineMenuItem(wnd, hmenuhandle, index);	
}

//populate color array for connection combo
void
ConnectionBase::PopulateColorArray(HWND hwndcombo, wyString *dirstr, wyInt32 ConnNo, COLORREF bkcolor)
{
	wyInt32 i, conncount, count;
	wyString conn;


	if(m_arrayofcolor)
    {
        free(m_arrayofcolor);
        m_arrayofcolor = NULL;
	}

	conncount = SendMessage(hwndcombo, CB_GETCOUNT, 0 ,0);

	m_arrayofcolor = (wyInt32*)malloc(conncount * sizeof(wyInt32));

	for(i = 0; i < conncount; i++)
	{
		count = SendMessage(hwndcombo, CB_GETITEMDATA, i, 0);
		conn.Sprintf("Connection %u", count);
		if(ConnNo== count)
			m_arrayofcolor[i]= bkcolor;
		else
		m_arrayofcolor[i]  = wyIni::IniGetInt(conn.GetString(), "ObjectbrowserBkcolor",
			COLOR_WHITE, dirstr->GetString());
	}

	/*InvalidateRect(GetParent(hwndcombo), NULL, true);
	UpdateWindow(GetParent(hwndcombo));*/
}

void ConnectionBase::WriteMysqlDefValues(HWND hdlg)
{
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_HOST), WM_SETTEXT, 0, (LPARAM)L"localhost");
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_USER), WM_SETTEXT, 0, (LPARAM)L"root");
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PASSWORD), WM_SETTEXT, 0, (LPARAM)L"");
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_DATABASE), WM_SETTEXT, 0, (LPARAM)L"");
	SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PORT), WM_SETTEXT, 0, (LPARAM)L"3306");
	Button_SetCheck(GetDlgItem(hdlg, IDC_COMPRESS), 1);
#ifndef COMMUNITY
	Button_SetCheck(GetDlgItem(hdlg, IDC_READONLY), 0);
#endif
	//Button_SetCheck(GetDlgItem(hdlg, IDC_ISCLEARTEXT), 0);
	Button_SetCheck(GetDlgItem(hdlg, IDC_TIMEOUTDEF), 1);
	Button_SetCheck(GetDlgItem(hdlg, IDC_TIMEOUTOPT), 0);
	SendMessage(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), WM_SETTEXT, 0, (LPARAM)L"28800");
}

void
ConnectionBase::GetConnectionName(HWND hdlg, wyString *connnamestr, const wyChar *connum, const wyChar *directory)
{
	wyWChar		*conname = NULL;
	wyInt32		txtlen, index;
	wyString    conn(connum), dirstr(directory);
    HWND        hcb = GetDlgItem(hdlg, IDC_DESC);
	HWND        hwndCombo = (HWND) SendMessage(hcb, CCBM_GETCOMBOHWND, NULL, NULL);

	VERIFY((index = SendMessage(hcb, CB_GETCURSEL, 0, 0))!= CB_ERR);

	txtlen = (SendMessage(hcb, CB_GETLBTEXTLEN, index, 0));

	conname = AllocateBuffWChar(txtlen + 1);
	VERIFY((SendMessage(hwndCombo, CB_GETLBTEXT, index,(LPARAM)conname)));

	connnamestr->SetAs(conname);

	WritePrivateProfileSectionA(conn.GetString(), "", dirstr.GetString());
	wyIni::IniWriteString(conn.GetString(), "Name", connnamestr->GetString(), dirstr.GetString());

	free(conname);
}

void
ConnectionBase::DeleteNewConnectionDetails(HWND hwnd)
{
	wyInt32  cursel, totcount, ret;
	HWND	hwndcombo;
	wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport = 0;
    wyString    conn, dirstr;

	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return;

	dirstr.SetAs(directory);

	hwndcombo = GetDlgItem(hwnd, IDC_DESC);
    
	totcount  = SendMessage(hwndcombo, CB_GETCOUNT, 0, 0);
	cursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);
	SendMessage(hwndcombo, CB_DELETESTRING, cursel, 0);
    
    if(totcount == 1)
    {
		ChangeConnStates(hwnd, wyFalse);
		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), FALSE);
        SetWindowText(hwndcombo, L"");
        TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);
        OnConnDialogTabSelChange(hwnd, GetDlgItem(hwnd, IDC_CONNTAB));
		return ;
	} 
    else if(cursel ==(totcount - 1)) 
		SendMessage(hwndcombo, CB_SETCURSEL, cursel-1, 0);
	else if(cursel == 0)
		SendMessage(hwndcombo, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwndcombo, CB_SETCURSEL, cursel-1, 0);

	GetInitialDetails(hwnd);
	HandlerToGetInitialDetails(hwnd);

	//populate color array for connection combo
	PopulateColorArray(hwndcombo, &dirstr);

	TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);

	OnConnDialogTabSelChange(hwnd, GetDlgItem(hwnd, IDC_CONNTAB));

	m_isnewconnection = wyFalse;
    
    m_newconnid = -1;

	m_conndetaildirty = wyFalse;

}

//Get the sql_mode settings
void
ConnectionBase::GetSqlModeSettings(HWND hdlg, ConnectionInfo *coninfo)
{
	wyInt32 textlen = 0;
	wyWChar	*sqlmode = NULL;

	if(SendMessage(GetDlgItem(hdlg, IDC_MODEGLOBAL), BM_GETCHECK, 0, 0)== BST_UNCHECKED)
	{	
		textlen = SendMessage(GetDlgItem(hdlg, IDC_EDITMODE), WM_GETTEXTLENGTH,(WPARAM)0, (LPARAM)0);
		if(textlen)
		{
			sqlmode = AllocateBuffWChar(textlen + 1);
			SendMessage(GetDlgItem(hdlg, IDC_EDITMODE), WM_GETTEXT,(WPARAM)textlen + 1, (LPARAM)sqlmode);
		
			if(wcslen(sqlmode))
			{
				coninfo->m_sqlmode.SetAs(sqlmode);				
			}
			free(sqlmode);

			coninfo->m_sqlmode.LTrim();
			coninfo->m_sqlmode.RTrim();
		}
		else
		{
			coninfo->m_sqlmode.Clear();
		}
		
		coninfo->m_isglobalsqlmode = wyFalse;
	}
	else
	{
		coninfo->m_isglobalsqlmode = wyTrue;
	}	
}
void
ConnectionBase::GetInitCommands(HWND hdlg, ConnectionInfo *coninfo)
{
    wyWChar         *initcommand = NULL;
    wyString        initcommandstr;
    wyUInt32        datalen = 0;

    coninfo->m_initcommand.Clear();

    datalen = SendMessage(GetDlgItem(hdlg, IDC_EDITINITCOMMAND), WM_GETTEXTLENGTH, NULL,NULL);

    if(datalen)
    {
        initcommand = new wyWChar[datalen + 1];
        if(!initcommand)
            return;

        SendMessage(GetDlgItem(hdlg, IDC_EDITINITCOMMAND), WM_GETTEXT, datalen + 1,(LPARAM)initcommand);
        coninfo->m_initcommand.SetAs(initcommand);
        delete initcommand;
        initcommand = NULL;
    }
}

void
ConnectionBase::GetKeepAliveInterval(HWND hdlg, ConnectionInfo *coninfo)
{
    coninfo->m_keepaliveinterval = GetDlgItemInt(hdlg, IDC_PINGINTERVAL, NULL, wyFalse);
}

wyBool
ConnectionBase::CopyAndRename(wyString& directorystr, wyString& fullpathstr, wyString& newpath)
{
	fullpathstr.Strip(strlen("sqlyog.ini") + 1);
	if (!CreateDirectory(fullpathstr.GetAsWideChar(), NULL))
	{
		/* If the folder is there , then we will continue the process, otherwise we will return wyFalse */
		if (GetLastError() != ERROR_ALREADY_EXISTS)
			return wyFalse;
	}
	fullpathstr.AddSprintf("\\sqlyog_backup.ini");
	if (CopyFile(directorystr.GetAsWideChar(), fullpathstr.GetAsWideChar(), TRUE))
	{
		if (_wrename(directorystr.GetAsWideChar(), newpath.GetAsWideChar()) == 0)
		{
			return wyTrue;
		}
	}
	else
	{
		return wyFalse;
	}
	return wyTrue;
}
