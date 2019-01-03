#include "ConnectionCommunity.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "Http.h"
#include <time.h>

#define		VERIFIED		"WinReg"

extern	PGLOBALS		pGlobals;

CRegInfoCommunity::CRegInfoCommunity()
{
	m_hlinkfont = NULL;
	m_hlinkfont2 = NULL;
	m_tracking = wyFalse;
}

CRegInfoCommunity::~CRegInfoCommunity()
{
	if(m_hlinkfont)
		DeleteFont(m_hlinkfont);

	if(m_hlinkfont2)
		DeleteFont(m_hlinkfont2);
}

wyInt32 
CRegInfoCommunity::Show(HWND hwndParent, wyInt32 isregistered)
{
	wyInt32 ret;
	m_isRegistered = Registered(); 
	if(m_isRegistered  == 0)
	{	
		ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_REGISTERSPLASH), 
						   hwndParent, CRegInfoCommunity::DlgProc, (LPARAM)this);
		return	ret;
	}
	return 99;
}

wyInt32
	CRegInfoCommunity::Registered()
{
	HKEY		key;
	DWORD		dwdisverified = REG_BINARY, dwverfieddata = SIZE_128-1, dwdisposition ;
	wyChar      verifiedword[SIZE_128]={0};
	VERIFY((RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(REGKEY), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &key, &dwdisposition))== ERROR_SUCCESS);
	
	if(RegQueryValueEx(key, TEXT(VERIFIED), 0, &dwdisverified,(BYTE*)verifiedword, &dwverfieddata)!= ERROR_SUCCESS)
				return 0;
	else
				return 1;
}

INT_PTR CALLBACK
CRegInfoCommunity::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CRegInfoCommunity*	pcreg	=	(CRegInfoCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	HICON		hicon;

	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
			// changing the text of the buying URL displayed
			SetWindowText(GetDlgItem(hwnd, IDC_LINK), TEXT(BUYLABEL));
			// Adding logo in the trialsplash
			hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_MAIN));
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
            DestroyIcon(hicon);
		}
		break;

	case WM_CTLCOLORSTATIC:
        return pcreg->OnWmCtlcolorStatic(wparam, lparam);
		break;

	case WM_INITDLGVALUES:
        pcreg->OnWmInitDlgValues(hwnd);
		break;

	case WM_COMMAND:
        pcreg->OnWmCommand(hwnd, wparam);
	}

	return 0;
}

/* we use 32649 magic number as IDC_HAND is not defined in WINVER < 0x400 */

LRESULT CALLBACK
CRegInfoCommunity::StaticDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CRegInfoCommunity * pcreg = (CRegInfoCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		return 0;
	}

	return CallWindowProc(pcreg->m_wporigstaticproc, hwnd, message, wparam, lparam);
}

LRESULT CALLBACK
CRegInfoCommunity::ImageDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CRegInfoCommunity * pcreg = (CRegInfoCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		return 0;
	}

	return CallWindowProc(pcreg->m_wporigimageproc, hwnd, message, wparam, lparam);
}

void
CRegInfoCommunity::InitValues()
{
	if(m_isRegistered == 0)
	{
		ShowEmailWindow();
	}
}

wyInt32 
	CRegInfoCommunity::CheckValidEmail()
{
	wyWChar     trial_email[SIZE_128]={0};
	wyString	tempmail, str ;
	wyString	httppasscode;
	wyInt32		ret;

	// get the name.
	GetWindowText(GetDlgItem(m_hwnddlg, IDC_TRIALEMAIL), (wyWChar*)trial_email, SIZE_128-1);
	
	if(wcslen(trial_email)== 0)
    {
		yog_message(m_hwnddlg, _(L"Please enter your email"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return -1;
	}
	tempmail.SetAs(trial_email);
	tempmail.LTrim();
	tempmail.RTrim();
	m_newmail.SetAs(tempmail.GetString());
	
	if(m_newmail.Compare(m_oldmail) != 0)
	{
		// Will generate the new passcode only when the new email id is different from the old email id
		m_oldmail.SetAs(tempmail.GetString());
		m_passcode.SetAs("");
		m_passcode.AddSprintf("%d",GeneratePasscode());
	}
	
	if(m_passcode.GetLength() == 0)
	{
		m_passcode.SetAs("");
		m_passcode.AddSprintf("%d",GeneratePasscode());
	}
	
	httppasscode.SetAs("http://www.webyog.com/mailmgr/sqlyog_verification?email=");
	httppasscode.AddSprintf(tempmail.GetString());
	httppasscode.AddSprintf("&passcode=%s",m_passcode.GetString());
	httppasscode.AddSprintf("&MajorVersion=%s&MinorVersion=%s&UpdateVersion=%s",MAJOR_VERSION,MINOR_VERSION,UPDATE_VERSION);
	m_httpreqcode.SetAs(httppasscode.GetString());

	ret = SendPasscodeRequest(); // Send the passcode email request to webyog server

	return ret;
}

//Activates and deactivates controls w.r.t. the passcode verification window of trialsplash dialog
void 
	CRegInfoCommunity::ShowPasscodeWindow()
{	
		
	EnableWindow(GetDlgItem(m_hwnddlg, IDM_VERIFY),TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TITLE), FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TRIALEMAIL),FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_NEXT),FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_PROGRESS),FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDOK), FALSE);

	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TITLE2), TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TRIALNOTESTATIC), TRUE);
	SendMessage(GetDlgItem(m_hwnddlg, IDC_TRIALNOTESTATIC), WM_SETTEXT, 0,(LPARAM)_(L"If you don't receive the passcode in the inbox within 2 minutes, try looking into the spam folder. Still haven't found it? Click on Resend."));		
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_PASSCODE),TRUE);
	SendMessage(GetDlgItem(m_hwnddlg, IDC_PASSCODE), WM_SETTEXT, 0,(LPARAM)_(L""));		
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_BACK), TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_RESEND),TRUE);	
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_VERIFY),TRUE);
}

// Activates and deactivates controls w.r.t. the email window of trialsplash dialog
void 
	CRegInfoCommunity::ShowEmailWindow()
{
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TITLE), TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TRIALEMAIL),TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TRIALNOTESTATIC), TRUE);
	SendMessage(GetDlgItem(m_hwnddlg, IDC_TRIALNOTESTATIC), WM_SETTEXT, 0,(LPARAM)_(L"You will receive passcode in the email provided above. Click Next to receive passcode:"));		
//	ShowWindow(GetDlgItem(m_hwnddlg, IDM_BUY),TRUE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_NEXT),TRUE);	
	
	ShowWindow(GetDlgItem(m_hwnddlg, IDOK), FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_TITLE2), FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDC_PASSCODE),FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_BACK), FALSE);
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_RESEND),FALSE);	
	ShowWindow(GetDlgItem(m_hwnddlg, IDM_VERIFY),FALSE);
	EnableWindow(GetDlgItem(m_hwnddlg, IDM_VERIFY),FALSE);
}

// Verify the entered passcode
wyInt32	
	CRegInfoCommunity::PasscodeVerification()
{
	wyWChar passcode_entered[SIZE_64]={0};
	wyString passcode;
	wyInt32 ret;
	GetWindowText(GetDlgItem(m_hwnddlg, IDC_PASSCODE), (wyWChar*)passcode_entered, SIZE_64-1);
	passcode.SetAs(passcode_entered);
	if(passcode.Compare(m_passcode.GetString()) == 0)
	{
		m_httpreqcode.AddSprintf("&isVerified=Y");
		ret = SendPasscodeVerifiedStatus();
		if(ret == 0)
		{
			yog_message(m_hwnddlg, _(L"Verification failed. Check your internet connectivity."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);	
			return -1;		
		}
		yog_message(m_hwnddlg, _(L"Passcode verified successfully."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);	
		AddVerifiedToRegistry();
		return 1;
	}
	else
	{
		yog_message(m_hwnddlg, _(L"Invalid passcode. Please try again."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);	
		return -1;
	}
}

wyInt32  
CRegInfoCommunity::OnWmCtlcolorStatic(WPARAM wparam, LPARAM lparam)
{
	wyInt32 identifier;
	HDC	hdc = (HDC)wparam;
	DWORD   fontheight;	
	
	identifier = GetDlgCtrlID((HWND)lparam);

	if(identifier == IDC_LINK)
	{
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);

		if(!m_hlinkfont2)
			m_hlinkfont2 = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, TRUE, 0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_hlinkfont2);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 255));

		return(BOOL)GetStockObject(HOLLOW_BRUSH);
	} 
    else if(identifier == IDC_TRIALSTATIC)
	{
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);

		if(!m_hlinkfont2)
			m_hlinkfont2 = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, FALSE, 0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_hlinkfont2);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));

		return(BOOL)GetStockObject(HOLLOW_BRUSH);
	} 
    else
    {
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		if(!m_hlinkfont)
			m_hlinkfont = CreateFont(fontheight, 0, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_hlinkfont);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));
        return 0;
	}
}

void 
CRegInfoCommunity::OnWmInitDlgValues(HWND hwnd)
{
    HWND hwndstatic = GetDlgItem(hwnd, IDC_LINK);
	HWND hwndimg	= GetDlgItem(hwnd, IDC_IMAGELINK);
	
	m_hwnddlg = hwnd;
	InitValues();

	m_wporigstaticproc = (WNDPROC)SetWindowLongPtr(hwndstatic, GWLP_WNDPROC, (LONG_PTR)StaticDlgProc);	
	SetWindowLongPtr(hwndstatic, GWLP_USERDATA,(LONG_PTR)this);

    m_wporigimageproc = (WNDPROC)SetWindowLongPtr(hwndimg, GWLP_WNDPROC, (LONG_PTR)ImageDlgProc);	
	SetWindowLongPtr(hwndimg, GWLP_USERDATA,(LONG_PTR)this);

    return;
}

void 
CRegInfoCommunity::OnWmCommand(HWND hwnd, WPARAM wparam)
{
	wyString msg;
	wyInt32 ret;
    switch(LOWORD(wparam))
    {
    case IDOK:
	    break;

    case IDCANCEL:
	    yog_enddialog(hwnd, TRIAL_CLOSE);
	    break;

	case IDM_NEXT:
		ret = CRegInfoCommunity::CheckValidEmail();
		if(ret == -1 )
			break;
		if( ret == 1 )
		{
			CRegInfoCommunity::ShowPasscodeWindow();
		}
		break;

	case IDM_VERIFY:
		ret = CRegInfoCommunity::PasscodeVerification();
		if( ret == 1 )
			yog_enddialog(hwnd, TRIAL_LATER);

		break;

	case IDM_RESEND:
		ret = CRegInfoCommunity::SendPasscodeRequest();
		break;

	case IDM_BACK:
		CRegInfoCommunity::ShowEmailWindow();
		break;


    case IDC_LITE:
	    yog_enddialog(hwnd, TRIAL_LITE);
	    break;
    }

    if((HIWORD(wparam)== STN_CLICKED))
    {
	    if(LOWORD(wparam)== IDC_LINK)
		    ShellExecute(NULL, L"open", TEXT(BUYURL), NULL, NULL, SW_SHOWNORMAL);
        else if(LOWORD(wparam)== IDC_IMAGELINK)
            ShellExecute(NULL, L"open", TEXT(IMAGEURL), NULL, NULL, SW_SHOWNORMAL);
    }

    return;
}

wyBool
	CRegInfoCommunity::AddVerifiedToRegistry()
{
		DWORD		 dwdisposition;
		HKEY		key;
		VERIFY((RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(REGKEY), 0, NULL, REG_OPTION_NON_VOLATILE, 
                       KEY_READ | KEY_WRITE, NULL, &key, &dwdisposition))== ERROR_SUCCESS);
	
		VERIFY(RegSetValueEx(key, TEXT(VERIFIED), 0, REG_BINARY, (UCHAR*)m_newmail.GetString(), sizeof(DWORD))== ERROR_SUCCESS);
		VERIFY(RegCloseKey(key)== ERROR_SUCCESS);
		return wyTrue;
}

wyInt32
	CRegInfoCommunity::SendPasscodeRequest()
{
	CHttp		http;
	int			status;

	http.SetUrl(m_httpreqcode.GetAsWideChar());
	http.SetContentType(L"text/xml");
	
	if(!http.SendData("abc", 3, false, &status, false ))
	{
		yog_message(m_hwnddlg, _(L"Passcode not sent. Please try again."),pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return 0;  // internet connectivity issue
	}

	yog_message(m_hwnddlg, _(L"Passcode request sent successfully."),pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);	
	return 1;
}

wyInt32
	CRegInfoCommunity::SendPasscodeVerifiedStatus()
{
	CHttp		http;
	int			status;
	wyString	httpreqverified;
	httpreqverified.SetAs("");
	httpreqverified.AddSprintf("http://www.webyog.com/mailmgr/sqlyog_verified?email=%s&is_verified=Y",m_newmail.GetString());
	http.SetUrl(httpreqverified.GetAsWideChar());
	http.SetContentType(L"text/xml");
	
	if(!http.SendData("abc", 3, false, &status, false ))
	{
		return 0;  // internet connectivity issue
	}
	return 1;
}

wyInt32
	CRegInfoCommunity::GeneratePasscode()
{  
	srand(time(NULL));
	return rand()%10*1000+rand()%10*100+rand()%10*10+rand()%10;
}