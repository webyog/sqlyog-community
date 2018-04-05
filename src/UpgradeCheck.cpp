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

#include "UpgradeCheck.h"
#include "Global.h"
#include "FrameWindowHelper.h"
#include "Include.h"
#include "Http.h"

extern	PGLOBALS		pGlobals;

//#define SERVERURL L"http://192.168.1.3/check_update.php"
//#define SERVERURL L"http://192.168.1.56:8080//check_update.php"
#define SERVERURL   L"http://stats.webyog.com/check_update.php"
#define CONTENT_TYPE	L"text/html"
# define UPDDAYS		1
#define  DELAY_12HOUR  12*3600000

UpgradeCheck::UpgradeCheck()
{
	m_isupgradeexplicit = wyFalse;
	m_isupgrade			= wyFalse;
	m_hwndmain			= pGlobals->m_hwndclient;
	m_linkdlgproc		= NULL;	
	m_hfont				= NULL;
	m_ispopupdlg		= wyFalse;
	m_iserror			= wyFalse;
	m_ischecked			= wyFalse;
	m_thrid				= NULL;	
    m_availableversion[0] = m_versiontobeignored[0] = 0;
}

UpgradeCheck::~UpgradeCheck()
{		
	ExitUpgradeCheckThread();

	if(m_hfont)
		VERIFY(DeleteFont(m_hfont));
}

VOID
UpgradeCheck::ExitUpgradeCheckThread()
{
	wyInt32 ret;
	DWORD	thrid;
	bool st = false;

	if(m_thrid)
	{
		ret = WaitForSingleObject(m_thrid, 0);

		if(ret == WAIT_TIMEOUT)
		{
			GetExitCodeThread(m_thrid, &thrid);
			st = TerminateThread(m_thrid, thrid);						
		}

		CloseHandle(m_thrid);
		m_thrid = NULL;
	}
}

void
UpgradeCheck::HandleUpgradeCheck(wyBool isexplict)
{	
	UPGRD		*evt;
	wyString	dirstr;
	wyUInt32	thdid = 0;
	wyInt64		ret = 0, upgradechk = 1;
	wyWChar		directory[MAX_PATH +1]={0}, *lpfileport;
	DWORD		currntday = 0, lastchkdday = 0;
	wyString	keyvaluestr;
	wyBool		istocheck = wyFalse;
	SYSTEMTIME	systime ={0};

    m_isupgrade		= wyFalse;
	m_ispopupdlg	= wyFalse;
	m_iserror		= wyFalse;
	m_ischecked		= wyFalse;
	
	m_isupgradeexplicit = isexplict;	

			
	//For explicit check for upgrade
	if(isexplict == wyTrue)
	{
		ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_UPGRADECHK), 
			m_hwndmain, UpgradeCheck::DlgProc,(LPARAM)this);				
	}

	//For implicit check for upgrade
	else
	{
		m_ispopupdlg = wyTrue;

		SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
		if(directory)
		{
			dirstr.SetAs(directory);
		}

		evt = new UPGRD;

		//Sets the struct variables
		SetThreadParam(evt, isexplict);

		//check for upgrade: if Upgradecheck == 0, then automatic upgrade check will be disabled
		//if(dirstr.GetLength())
			//upgradechk = wyIni::IniGetInt(GENERALPREFA, "UpdateCheck", 1, dirstr.GetString());
		
		//If the Dont remind me check box is checked
		if(upgradechk == 0)
		{
			m_ispopupdlg = wyFalse;			
		}

		else
		{
			//Read last upgrade check date
			if(dirstr.GetLength())
			{
				ret = wyIni::IniGetString(GENERALPREFA, "UpdatesCheckedDate", "0", &keyvaluestr, dirstr.GetString());
				lastchkdday = strtoul(keyvaluestr.GetString(), NULL, 10); 
			}
			
			//Upgrade check while start up limits only once in a week
			GetSystemTime(&systime);		
			currntday = HandleSetDayFormat(systime.wDay, systime.wMonth, systime.wYear);

			if(currntday > lastchkdday && currntday - lastchkdday >= UPDDAYS)
			{
				m_currentdate.Sprintf("%lu", currntday);
				
				//set new upgrade check date
				//ret = wyIni::IniWriteString(GENERALPREFA, "UpgradeCheckedDate", keyvaluestr.GetString(), dirstr.GetString());

				istocheck = wyTrue;

                if(dirstr.GetLength())
                {
                    wyIni::IniGetString(GENERALPREFA, "VersionToBeIgnored", "0", &keyvaluestr, dirstr.GetString());
                    strncpy(m_versiontobeignored, keyvaluestr.GetString(), (sizeof(m_versiontobeignored) / sizeof(m_versiontobeignored[0])) - 1);
                }
			}

			//No upgrade check 
			if(istocheck == wyFalse)
				m_ispopupdlg = wyFalse;				
			
		}

        VERIFY(m_thrid = (HANDLE)_beginthreadex(NULL, 0, UpgradeCheck::HandleHttpRequest, evt, 0, &thdid));
	}
}

wyBool		
UpgradeCheck::SetThreadParam(UPGRD *evt, wyBool isexplict)
{		
	evt->m_upchk = this;
	evt->m_isexplicitcheck = isexplict;
	evt->m_hwndmain = m_hwndmain;
		
	return wyTrue;
}

unsigned __stdcall
UpgradeCheck::HandleHttpRequest(LPVOID param)
{			
	UPGRD		*uphd;
	HWND		hwndparent;
	UpgradeCheck *upgrd;	
	//wyString	url;
	/*wyString	ret;*/
	/*wyString	appmajorversion, appminorversion,extrainfo;*/
	wyInt32		status, retint, newlinepos;
	wyWChar		headers[1024*3] = {0};
	wyChar		*xmlreceived	= NULL;
	wyString	garbage;
	wyBool		ischkexplicit = wyFalse;
    const wyChar    *langcode;
    const wyWChar*  versionptr;

	uphd = (UPGRD*)param;
	upgrd = uphd->m_upchk;
	
	ischkexplicit = uphd->m_isexplicitcheck;
	hwndparent = uphd->m_hwndmain;
   
	if(uphd)
		delete uphd;

	if(!upgrd)
		return 1;

#if	IS_FROM_WINDOWS_STORE == 1
	return 1;
#endif
	//Gets the product id(Ent, Commmunity or Trial)
#ifdef COMMUNITY
	upgrd->m_productid.SetAs("SQLyog Community");
#elif ENTERPRISE
	//SQLyog versions
	if(!pGlobals)
		return 1;

	if(!pGlobals->m_entlicense.CompareI("Professional"))
		upgrd->m_productid.SetAs("SQLyog Pro");
		
	else if(!pGlobals->m_entlicense.CompareI("Enterprise"))
		upgrd->m_productid.SetAs("SQLyog Ent");

	else if(!pGlobals->m_entlicense.CompareI("Ultimate"))			
		upgrd->m_productid.SetAs("SQLyog Ulti");		
		
#else
	upgrd->m_productid.SetAs("SQLyog Trial");
#endif

	upgrd->m_productid.EscapeURL();
	
	//Major and minor version num
	upgrd->m_appmajorversion.SetAs(MAJOR_VERSION);
	upgrd->m_appminorversion.SetAs(MINOR_VERSION UPDATE_VERSION);
	upgrd->m_extrainfo.SetAs(EXTRAINFO); // beta string	
			
	upgrd->m_url.SetAs(SERVERURL);	

	/*variables send from application are 
	1)Product ID(community, ent or trial) 
	2) Major_version
	3) Minor_version
	4) Upgrade count
	5) Beta strinng
	*/

#ifdef _WIN64

		upgrd->m_url.AddSprintf("?product_id=%s&major_version=%s&minor_version=%s&extra=%s&os=win64&language=%s&uuid=%s", upgrd->m_productid.GetString(), 
		upgrd->m_appmajorversion.GetString(), upgrd->m_appminorversion.GetString(), upgrd->m_extrainfo.GetString(), 
        (langcode = GetL10nLangcode()) ? langcode : "en", 
        pGlobals->m_appuuid.GetString());
#else
		upgrd->m_url.AddSprintf("?product_id=%s&major_version=%s&minor_version=%s&extra=%s&os=win32&language=%s&uuid=%s", upgrd->m_productid.GetString(), 
		upgrd->m_appmajorversion.GetString(), upgrd->m_appminorversion.GetString(), upgrd->m_extrainfo.GetString(), 
        (langcode = GetL10nLangcode()) ? langcode : "en", 
        pGlobals->m_appuuid.GetString());
#endif
	if(upgrd->m_url.GetLength())
	{	
		CHttp		http, http_success;
		wyString	ret;
		wyString	url_success;
	    url_success.SetAs(upgrd->m_url.GetString());
		url_success.AddSprintf("&shown=1");
		http.SetUrl(upgrd->m_url.GetAsWideChar());
		http.SetContentType(CONTENT_TYPE);

		garbage.SetAs("SQLyog");

		if(upgrd->m_isupgradeexplicit == wyTrue)
			garbage.SetAs("sss");


		if(upgrd->m_ispopupdlg == wyFalse && upgrd->m_isupgradeexplicit == wyFalse)
			goto checkalways;

		//for update check authentication is not required, so the last parameter sets 'checkauth = false'
		if(!http.SendData((char*)garbage.GetString(), garbage.GetLength(), false, &status, false))
		{
			if(ischkexplicit == wyTrue)
			{
				upgrd->m_iserror = wyTrue;
				PostMessage(upgrd->m_hwnddlg, UM_CHANGETEXT, 0, 0);		

				return 1;
			}
        
			else
			{
				goto checkalways;				
			}
		}
    
		/* get all header informations */
		if(!http.GetAllHeaders(headers, sizeof(headers)))
		{
			if(ischkexplicit == wyTrue)
			{
				upgrd->m_iserror = wyTrue;			
				PostMessage(upgrd->m_hwnddlg, UM_CHANGETEXT, 0, 0);		

				return 1;
			}
			else
			{
				goto checkalways;		
			}
		}

		//check whether its 200 or not
		if(status != HTTP_STATUS_OK )
		{
			upgrd->m_iserror = wyTrue;	
			xmlreceived = http.GetResponse();

			if(xmlreceived)
				free(xmlreceived);

			xmlreceived= NULL;
		
			if(ischkexplicit == wyTrue)
			{
				PostMessage(upgrd->m_hwnddlg, UM_CHANGETEXT, 0, 0);				
				return 1;
			}
			else
			{
				goto checkalways;	
			}
		}
		
		/* get the data */
		xmlreceived = http.GetResponse();
		if(!xmlreceived)
		{
			if(ischkexplicit == wyTrue)
			{
				upgrd->m_iserror = wyTrue;	
				PostMessage(upgrd->m_hwnddlg, UM_CHANGETEXT, 0, 0);		

				return 1;
			}

			else
			{
				goto checkalways;			
			}
		}

		ret.SetAs(xmlreceived);	
		ret.LTrim();
		ret.RTrim();
	
		if(xmlreceived)
		{
			free(xmlreceived);
			xmlreceived = NULL;
		}
	
		if(!ret.Compare("upgrade=1"))
		{
			upgrd->m_isupgrade = wyTrue;
		}

		else
		{
			upgrd->m_isupgrade = wyFalse;
		}
		http_success.SetUrl(url_success.GetAsWideChar());
		http_success.SetContentType(CONTENT_TYPE);
		http_success.SendData((char*)garbage.GetString(), garbage.GetLength(), false, &status, false);
	}
		
	/*ret.SetAs(xmlreceived);	
	ret.LTrim();
	ret.RTrim();
	
	if(xmlreceived)
	{
		free(xmlreceived);
		xmlreceived = NULL;
	}
	
	if(!ret.Compare("upgrade=1"))
	{
		upgrd->m_isupgrade = wyTrue;
	}

	else
	{
		upgrd->m_isupgrade = wyFalse;
	}*/

	/*For explicit 'upgrade check'(help->check for update) dialog box pops if upgrade is available or not.
	But for implicit 'upgrade check'  dialog box pops only if upgrade is available.*/	
	if(upgrd->m_isupgradeexplicit == wyTrue)
	{			
		upgrd->m_ispopupdlg = wyTrue;	
				
		//Changeing the dlg text
		PostMessage(upgrd->m_hwnddlg, UM_CHANGETEXT, 0, 0);		
	}	

    else if(upgrd->m_isupgrade == wyTrue && upgrd->m_isupgradeexplicit == wyFalse)
	{
        if((versionptr = wcsstr(headers, L"X-SQLyog-Version")) && (versionptr = wcsstr(versionptr, L":")) && (++versionptr))
        {
            wyString ret;
            
            ret.SetAs(versionptr);
            
            if((newlinepos = ret.Find("\n", 0)) != -1)
            {
                ret.Erase(newlinepos, ret.GetLength() - newlinepos);
            }

            ret.LTrim();
            ret.RTrim();
            strncpy(upgrd->m_availableversion, ret.GetString(), (sizeof(upgrd->m_availableversion) / sizeof(upgrd->m_availableversion[0])) - 1);
        }

        if(strcmp(upgrd->m_availableversion, upgrd->m_versiontobeignored))
        {
            retint = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_UPGRADECHK), 
					hwndparent, UpgradeCheck::DlgProc,(LPARAM)upgrd);	
        }
		
	}	

checkalways:

	if(upgrd->m_isupgradeexplicit == wyFalse)
	{
		//If dialog(Upgrade check) on start-up then wait else send now
		if(upgrd->m_ispopupdlg == wyTrue)
			Sleep(DELAY_12HOUR);

		//Sending Http request every 1 hour
		while(1)
		{			
			CHttp		httpsend; 
			httpsend.SetUrl(upgrd->m_url.GetAsWideChar());
			httpsend.SetContentType(CONTENT_TYPE);

			if(httpsend.SendData((char*)garbage.GetString(), garbage.GetLength(), false, &status, false)) 
			{				
				/* get all header informations */
				if(httpsend.GetAllHeaders(headers, sizeof(headers)))
				{
					xmlreceived = httpsend.GetResponse(); 
					if(xmlreceived) 
					{
						free(xmlreceived);
						xmlreceived = NULL;
					}
				}
				//YOGLOG(1, "http.SendData Success");
			}
            
			Sleep(DELAY_12HOUR); // 12 Hours			
			//Sleep(60000); //1 min

		}
	}	

	return 1;
}

INT_PTR	CALLBACK	
UpgradeCheck::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UpgradeCheck *upchk = (UpgradeCheck*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
		//link static text window process
		if(upchk->m_ispopupdlg == wyFalse)
		{
			upchk->m_linkdlgproc =(WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_LINK), GWLP_WNDPROC,(LONG_PTR)LinkDlgProc);	
			SetWindowLongPtr(GetDlgItem(hwnd, IDC_LINK), GWLP_USERDATA,(LONG_PTR)upchk);
		}

        upchk->InitDialog(hwnd);

		if(upchk->m_isupgradeexplicit == wyTrue)
		{
			upchk->HandleExplicitUpgradeCheck();
		}	

		//Making this dialog to top of SQLyog only, not for other applications also
		SetWindowPos(hwnd, pGlobals->m_pcmainwin->m_hwndmain, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
		
		break;

	case UM_CHANGETEXT:
		{
			upchk->ChangeTextOnExplicitCheck(hwnd);			
			break;
		}
		
	case WM_CTLCOLORSTATIC:
		return upchk->OnDlgProcColorStatic(hwnd, wParam, lParam);
		break;

	case WM_COMMAND:
		upchk->OnWMCommand(hwnd, wParam);
		break;			
	}
    
	return 0;
}

void	
UpgradeCheck::InitDialog(HWND hwnd)
{
	HWND		hwndchk, hwndstatic = NULL, hwndlink = NULL, hwndok = NULL;
	wyInt32		ret;
	wyString	dirstr;
	wyWChar     directory[MAX_PATH +1]={0}, *lpfileport;
	
	m_hwnddlg =  hwnd;

	VERIFY(hwndchk = GetDlgItem(hwnd, IDC_UPGRDCHK));
	VERIFY(hwndstatic = GetDlgItem(hwnd, IDC_STATICTITLE));
	VERIFY(hwndlink = GetDlgItem(hwnd, IDC_DOWNLOAD));
	VERIFY(hwndok = GetDlgItem(hwnd, IDOK));

	//'Dont remid check box be hidden if upgradecheck explicitlle.
	if(m_isupgradeexplicit == wyTrue)
		ShowWindow(hwndchk, SW_HIDE);

	else
		ShowWindow(hwndchk, SW_SHOW);


	//set text for static text
	if(m_isupgrade == wyTrue)
	{
		SetWindowText(hwndstatic, UPGRADEYES);

		//set new upgrade check date into .ini file, if implicit upgrade is successful
		if(m_currentdate.GetLength() && m_isupgradeexplicit == wyFalse)
		{
			SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
			if(!directory)
				return;

			dirstr.SetAs(directory);
			
			ret = wyIni::IniWriteString(GENERALPREFA, "UpdatesCheckedDate", m_currentdate.GetString(), dirstr.GetString());

			ShowWindow(hwndok, SW_SHOW);
			ShowWindow(hwndlink, SW_SHOW);
		}
	}

	//While cheking for upgrade during explicit check
	if(m_ispopupdlg == wyFalse && m_isupgradeexplicit == wyTrue)
	{	
		
		SetWindowText(hwndstatic, UPGRDCHKING);

		ShowWindow(hwndlink, SW_HIDE);	 //hide ok & link
		ShowWindow(hwndok, SW_HIDE);		
	}
		
	//Make it top
	VERIFY(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));

	return;
}

void	
UpgradeCheck::HandleExplicitUpgradeCheck()
{	
	UPGRD *evt = NULL;
	wyUInt32 thdid;

	evt = new UPGRD;

	//Sets the struct variables
	SetThreadParam(evt, m_isupgradeexplicit);

	VERIFY(m_thrid = (HANDLE)_beginthreadex(NULL, 0, UpgradeCheck::HandleHttpRequest, evt, 0, &thdid));			
	return;
}

//Sets the color and font for text
wyInt32
UpgradeCheck::OnDlgProcColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	wyInt32 id, fontheight;
	HDC		hdc;

	hdc = (HDC)wParam;
	id = GetDlgCtrlID((HWND)lParam);

	switch(id)
	{
		case IDC_LINK:
		{
			if(m_hfont)
			{
				VERIFY(DeleteFont(m_hfont));
				m_hfont = NULL;
			}

			fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			m_hfont = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, TRUE, 0, 0, 0, 0, 0, 0, L"Verdana");
			SelectObject(hdc, m_hfont);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 0, 255));

			return(wyInt32)GetStockObject(HOLLOW_BRUSH);
		}		
	}

	return 0;
}

void		
UpgradeCheck::ChangeTextOnExplicitCheck(HWND hwnd)
{	
	HICON hicon;
	HWND hwndstatic = NULL, hwnddownload = NULL, hwndcancel = NULL; 

	VERIFY(hwnddownload = GetDlgItem(hwnd, IDC_DOWNLOAD));
	VERIFY(hwndstatic = GetDlgItem(hwnd, IDC_STATICTITLE));
	VERIFY(hwndcancel = GetDlgItem(hwnd, IDCANCEL));
		    
	if(m_isupgrade == wyTrue)	
	{
#ifndef ENTERPRISE
		SetWindowText(hwndstatic, UPGRADEYES);
        SetWindowText(hwndcancel, _(L"&Cancel"));
#else //Version specific      
		switch(pGlobals->m_pcmainwin->m_connection->m_enttype)
		{
		case ENT_PRO:
			SetWindowText(hwndstatic, UPGRADEYESPRO);            
			break;

		case ENT_NORMAL:
			SetWindowText(hwndstatic, UPGRADEYESENT);
			break;

		case ENT_ULTIMATE:
			SetWindowText(hwndstatic, UPGRADEYESULT);
			break;
		}
#endif
		ShowWindow(hwnddownload, SW_SHOW);
	}

	else if(m_isupgrade == wyFalse && m_iserror == wyFalse)	
	{
        SetWindowText(hwndcancel, _(L"&OK"));
        ShowWindow(hwnddownload, SW_HIDE);
#ifndef ENTERPRISE
		SetWindowText(hwndstatic, UPGRADENO);

#else  //Version specific
		switch(pGlobals->m_pcmainwin->m_connection->m_enttype)
		{
		case ENT_PRO:
			SetWindowText(hwndstatic, UPGRADENOPRO);
			break;

		case ENT_NORMAL:
			SetWindowText(hwndstatic, UPGRADENOENT);
			break;

		case ENT_ULTIMATE:
			SetWindowText(hwndstatic, UPGRADENOULT);
			break;
		}
#endif		
	}

	else if(m_iserror == wyTrue)
	{				
		//Load the Warning icon
		hicon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hwnd, IDC_LOGO, STM_SETICON, (WPARAM)hicon, 0);
		DestroyIcon(hicon);

		SetWindowText(hwndstatic, CONERROR);
		ShowWindow(hwnddownload, SW_HIDE);
        SetWindowText(hwndcancel, _(L"&OK"));
	}

	ShowWindow(hwndcancel, SW_SHOW);
	m_ischecked = wyTrue;
}

/*If 'dont remind' checkbox is checked, then 'UpgradeCheck' variable in .ini sets '0'. 
If its '0' then automatic upgrade check will be disabled
*/
wyInt32
UpgradeCheck::OnWMCommand(HWND hwnd, WPARAM wParam)
{
	wyInt32		ret;
	HWND		hwndchk;
	wyString	dirstr, strcnt;
	wyWChar     directory[MAX_PATH +1]={0}, *lpfileport;
	
	switch(LOWORD(wParam))
	{
	case IDCANCEL:
	case IDOK:
		{
			if(m_ischecked == wyFalse && m_isupgradeexplicit == wyTrue)
				break;

			hwndchk = GetDlgItem(hwnd, IDC_UPGRDCHK);
			ret = SendMessage(hwndchk, BM_GETCHECK, 0, 0);
			
			//if checkbox checked 'then the upgradecheck dialog box never pops up when application starts
			if(ret == BST_CHECKED)
			{									
				//Increase the upgradecount value by 1
				SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
				if(!directory)
				{
					EndDialog(hwnd, 0);
					break;
				}

				dirstr.SetAs(directory);		
                wyIni::IniWriteString(GENERALPREFA, "VersionToBeIgnored", m_availableversion, dirstr.GetString());
			}	

			EndDialog(hwnd, 0);
		}
		break;

	case IDC_DOWNLOAD:

		//url link
#ifdef COMMUNITY
		ShellExecute(NULL, L"open", TEXT(COMMUNITYURL), NULL, NULL, SW_SHOWNORMAL);		
#elif ENTERPRISE
		ShellExecute(NULL, L"open", TEXT(SUPPORTURL), NULL, NULL, SW_SHOWNORMAL);
#else
        ShellExecute(NULL, L"open", TEXT(PRODUCTURL), NULL, NULL, SW_SHOWNORMAL);
#endif
		EndDialog(hwnd, 0);
		break;	
	}
	return 1;
}

LRESULT	CALLBACK
UpgradeCheck::LinkDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UpgradeCheck *upchk = (UpgradeCheck*)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		break;
	}

	return CallWindowProc(upchk->m_linkdlgproc, hwnd, message, wParam, lParam);
}

//DDYYYY format
wyUInt32
UpgradeCheck::HandleSetDayFormat(wyInt32 pday, wyInt32 pmonth, wyInt32 pyear)
{
	wyUInt32    day;
    wyInt32     a, b;
    float 		yearcorr;

    if(pyear < 0)
        pyear++;

    yearcorr = (float)(pyear > 0 ? 0.0 : 0.75);    
    if(pmonth <= 2)
    {
        pyear--;
        pmonth += 12;
    }
    
    b = 0;    
    if(pyear * 10000.0 + pmonth * 100.0 + pday >= 15821015.0)
    {
        a = pyear / 100;
        b = 2 - a + a / 4;
    }    
    day = (wyUInt32)(365.25 * pyear - yearcorr)+
    (wyUInt32)(30.6001 *(pmonth + 1))+ pday + 1720995L + b;
    
    return day;
}