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
// Modified and Functions aded By Manohar.s 

#include "Global.h"
#include "BlobMgmt.h"
#include "MDIWindow.h"
#include "PreferenceBase.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "EditorFont.h"

#include <ole2.h>
#include <mstask.h>
#include <mlang.h>
#include <wtypes.h>

#ifndef VC6
#include <gdiplus.h>
#include <GdiPlusGraphics.h> 
#include <olectl.h>
#endif

extern PGLOBALS	pGlobals;

#define one 1
#define zero 0

#define MAXHT 350
#define MAXWD 660  
#define	FIND_STR_LEN		256

BlobMgmt::BlobMgmt()
{
	m_hfont = NULL;
	m_edit  = wyTrue;
	m_isencodingchanged = wyFalse;
	m_isutf8 = wyFalse;
    m_isansi = wyFalse;
    m_isucs2 = wyFalse;
	m_changedcombotext.SetAs("");
	m_findreplace = NULL;
}

BlobMgmt::~BlobMgmt()
{
	if(m_hfont)
		DeleteObject(m_hfont);
}

wyBool 
BlobMgmt::Create(HWND hwndParent, PINSERTUPDATEBLOB pib, wyBool edit)
{
	wyInt64     ret;
    MDIWindow   *wnd = GetActiveWin();

	m_piub			=	pib;
	m_olddata		=	pib->m_data;
	m_olddatasize	=	pib->m_datasize;
	m_oldnull		=	pib->m_isnull;	
	m_newdata		=	pib->m_data;
	m_newdatasize	=	pib->m_datasize;
	m_edit          =	edit;
	m_hfont         =	NULL;
	m_isblob		=   pib->m_isblob;
	m_isJson		=	pib->m_isJson;
	donot_close_window = wyTrue;

	//Post 8.01F
	//RepaintTabModule();
    
    if(wnd)
        wnd->m_lastfocus = NULL;

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_INSERTUPDATEBLOB), hwndParent, 
                                                BlobMgmt::DlgProc,(LPARAM)this);
	if(ret)
		return wyTrue;

    return wyFalse;
}

// The dialog procedure of the BlobInsertUpdate dialog box.
INT_PTR CALLBACK
BlobMgmt::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BlobMgmt *pcib = (BlobMgmt*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
 
	switch(message)
	{
	case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return TRUE;

	case WM_INITDLGVALUES:
		pcib->m_hwnddlg = hwnd;
		pcib->InitDlgVales();
		pcib->MoveDialog();	
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
		break;

	case WM_COMMAND:
        pcib->OnDlgProcWmCommand(wParam, lParam);
		break;

	case WM_NOTIFY:
		{
			LPNMHDR		lpnmhdr =(LPNMHDR)lParam;
			
			if(lpnmhdr->code == TCN_SELCHANGE)
				pcib->OnTabSelChange();
		}
		break;

	case WM_SIZE:
		pcib->Resize(lParam);
		break;

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
		pMMI->ptMinTrackSize.x = MAXWD;
		pMMI->ptMinTrackSize.y = MAXHT;
	}
	break;

	case WM_DESTROY:
		SaveInitPos(hwnd, BLOBVIEWER_SECTION);
		//StoreDialogPersist(hwnd, BLOBVIEWER_SECTION);
		break;

	default:
		break;
	}

	return 0;
}

wyBool
BlobMgmt::InitDlgVales()
{
	wyString codepage;

    //if find dialog is open, we need to close find dialog 
	if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

	VERIFY(m_hwndtab   = GetDlgItem(m_hwnddlg, IDC_MAINTAB));
	VERIFY(m_hwndedit  = GetDlgItem(m_hwnddlg, IDC_TEXT));
	VERIFY(m_hwndimage = GetDlgItem(m_hwnddlg, IDC_IMAGE));
	VERIFY(m_hwndnull  = GetDlgItem(m_hwnddlg, IDC_SETNULL));
	VERIFY(m_hwndcombo = GetDlgItem(m_hwnddlg, IDC_COMBO));
    
	InsertTab(m_hwndtab, 0, _(L"Text"));
  
	if(! m_isJson)
	InsertTab(m_hwndtab, 1, _(L"Image"));
	
	// if its not word wrap then we need to destroy it.
	SetEditWordWrap(m_hwndedit, wyTrue, wyTrue);
    
	// just for the sake hide both of them
	VERIFY(ShowWindow(m_hwndimage, FALSE));
	VERIFY(ShowWindow(m_hwndedit, FALSE));

	if(m_piub && m_piub->m_isnull == wyTrue)
    {
		SendMessage(GetDlgItem(m_hwnddlg, IDC_SETNULL), BM_SETCHECK, BST_CHECKED, 0);
		DisableAll();
	}

	SetWindowLongPtr(m_hwndimage, GWLP_USERDATA,(LONG_PTR)this);
	SetWindowLongPtr(m_hwndedit, GWLP_USERDATA,(LONG_PTR)this);
	
	SendMessage(m_hwndedit, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
	
	//To avoid printing of special characters in scintilla on ctrl+F and ctrl+H
	SendMessage (m_hwndedit, SCI_CLEARCMDKEY, (SCMOD_CTRL << 16)|'F', 0);
	SendMessage (m_hwndedit, SCI_CLEARCMDKEY, (SCMOD_CTRL << 16)|'H', 0);
	
    //Line added because to turn off the margin
    SendMessage(m_hwndedit, SCI_SETMARGINWIDTHN,1,0);

	SendMessage(m_hwndedit, SCI_SETSCROLLWIDTHTRACKING, TRUE, 0);
    
	//m_encodingtype.SetAs(codepage.GetString());
	
	//InitEncodingType();
	m_wporigtextproc =(WNDPROC)SetWindowLongPtr(m_hwndedit, GWLP_WNDPROC,(LONG_PTR)TextProc);
	m_wporigpaintproc =(WNDPROC)SetWindowLongPtr(m_hwndimage, GWLP_WNDPROC,(LONG_PTR)ImageProc);
	
	//initializing text in combobox
	InitComboText();
	ShowData();

    // set the font after reading it from ini file.
	SetEditFont(m_hwndedit);

	// if its not editable then we disable all except the ok button
	if(m_edit == wyFalse)
    {
		DisableAll();
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_SETNULL), FALSE);
        EnableWindow(GetDlgItem(m_hwnddlg, IDC_TEXT), TRUE);
        SendMessage(GetDlgItem(m_hwnddlg, IDC_TEXT), EM_SETREADONLY, TRUE, TRUE);
        SetFocus(GetDlgItem(m_hwnddlg, IDC_TEXT));
	}

	m_checkboxstate = SendMessage(GetDlgItem(m_hwnddlg, IDC_SETNULL), BM_GETCHECK, 0, 0);
    SendMessage(m_hwndedit, SCI_EMPTYUNDOBUFFER, 0, 0);

	return wyTrue;
}

void
BlobMgmt::InitEncodingType()
{
	if(m_encodingtype.CompareI("Unicode (UTF-8)") == 0)
		m_isutf8 = wyTrue;
	else if(m_encodingtype.CompareI("UCS-2") == 0)
		m_isucs2 = wyTrue;
	else
		m_isansi = wyTrue;
}

void 
BlobMgmt::InitComboText()
{
	SendMessage(m_hwndcombo, CB_ADDSTRING, 0, (LPARAM)L"Utf-8");
	SendMessage(m_hwndcombo, CB_ADDSTRING, 0, (LPARAM)L"US-ascii/Ansi ");
	SendMessage(m_hwndcombo, CB_ADDSTRING, 0, (LPARAM)L"UCS2");
}

void 
BlobMgmt::OnDlgProcWmCommand(WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		
		if(ProcessCancel() == wyTrue)
			VERIFY(yog_enddialog(m_hwnddlg, 0));
		break;
    
	case IDOK:
		if(ProcessOK() == wyTrue)
		    VERIFY(yog_enddialog(m_hwnddlg, 1));
		else	if(! m_isJson)
			VERIFY(yog_enddialog(m_hwnddlg, 0));
		else if( m_isJson && ( donot_close_window==wyFalse ) ) // for closing the window if no changes are made and its a JSON type
			VERIFY(yog_enddialog(m_hwnddlg, 0));
		break;

	case IDC_SAVETOFILE:
		SaveToFile();
		break;

	case IDC_IMPORT:
		OpenFromFile();
		break;

	case IDC_SETNULL:
		{
			// if a user has selected it then we disable evrything
			// otherwise enable everything
			if(Button_GetCheck((HWND)lparam)== BST_CHECKED)
				DisableAll(wyTrue);
			else 
				DisableAll(wyFalse);
		}
	break;	
	case IDC_COMBO:
		{
			if((HIWORD(wparam))== CBN_SELENDOK)
				OnComboChange();
			break;
		}
	}
    return;
}

void 
BlobMgmt::OnComboChange()
{
	wyWChar		*buff = NULL;
	wyString	buffstr;
	wyInt32	length, ret, ncursel;
	
	ncursel = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);
	length  = SendMessage(m_hwndcombo, CB_GETLBTEXTLEN, NULL, NULL);
	buff    = AllocateBuffWChar(15+ 1);
	VERIFY(ret = SendMessage(m_hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)buff) != CB_ERR);
	
	m_isencodingchanged = wyTrue;
	m_changedcombotext.SetAs(buff);
	free(buff);
}

void 
BlobMgmt::ProcessComboSelection(wyString& comboselparam)
{
 	wyChar *ansistr = NULL;
	wyChar *utf8str = NULL;
	wyWChar *ucs2str = NULL;
	wyInt32 lenucs2 = 0, i;
	
	if(comboselparam.CompareI("US-ascii/Ansi ") == 0 && m_isutf8 == wyTrue)
	{
		ansistr = Utf8toAnsi(m_blobdata.GetString(), m_blobdata.GetLength());
        m_piub->m_data = ansistr;
		m_piub->m_datasize      = strlen(ansistr);
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
	else if(comboselparam.CompareI("UTF-8") == 0 && m_isansi == wyTrue)
	{
		utf8str = AnsitoUtf8(m_blobdata.GetString(), m_blobdata.GetLength());
		m_piub->m_data = utf8str;
		m_piub->m_datasize      = strlen(utf8str);
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
	else if(comboselparam.CompareI("ucs2") == 0 && m_isutf8 == wyTrue)
	{
		ucs2str = Utf8toUcs2(m_blobdata.GetString(), m_blobdata.GetLength());
		lenucs2 = wcslen(ucs2str);
		memset(m_piub->m_data, 0, strlen(m_piub->m_data));
		for(i=0; i < lenucs2; i++)
			memcpy(m_piub->m_data + i, ucs2str + i, 1);
		
		m_piub->m_datasize      = lenucs2;
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
	else if(comboselparam.CompareI("US-ascii/Ansi ") == 0 && m_isucs2 == wyTrue)
	{
		ansistr = Ucs2toAnsi(m_blobdata.GetString(), m_blobdata.GetLength());
		m_piub->m_data = ansistr;
		m_piub->m_datasize      = strlen(ansistr);
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
	else if(comboselparam.CompareI("UTF-8") == 0 && m_isucs2 == wyTrue)
	{
		utf8str = Ucs2toUtf8(m_blobdata.GetString(), m_blobdata.GetLength());
		m_piub->m_data = utf8str;
		m_piub->m_datasize      = strlen(utf8str);
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
	else if(comboselparam.CompareI("ucs2") == 0 && m_isansi == wyTrue)
	{
		ucs2str = AnsitoUcs2(m_blobdata.GetString(), m_blobdata.GetLength());
		lenucs2 = wcslen(ucs2str);
		memcpy(m_piub->m_data, ucs2str, lenucs2);
		for(i=0; i < lenucs2; i++)
			memcpy(m_piub->m_data + i, ucs2str + i, 1);
		m_piub->m_datasize      = lenucs2;
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
	}
}

wyChar*
BlobMgmt::Utf8toAnsi(const wyChar *utf8, wyInt32 len)
{

	wyChar *ansistr = NULL;
	wyInt32 length = MultiByteToWideChar(CP_UTF8, 0, utf8, len, NULL, NULL );
	wyWChar *lpszw = NULL;

	lpszw = new wyWChar[length+1];
	ansistr = (wyChar *)calloc(sizeof(wyChar), length + 1);

	//this step intended only to use WideCharToMultiByte
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, lpszw, length );

	//Conversion to ANSI (CP_ACP)
	WideCharToMultiByte(CP_ACP, 0, lpszw, -1, ansistr, length, NULL, NULL);

	ansistr[length] = 0;

	delete[] lpszw;

	return ansistr;
}

wyChar*
BlobMgmt::AnsitoUtf8(const wyChar *ansistr, wyInt32 len)
{
	wyChar *utf8str = NULL;
	wyInt32 length = MultiByteToWideChar(CP_ACP, 0, ansistr, len, NULL, NULL );
	wyWChar *lpszw = NULL;

	lpszw = new wyWChar[length+1];
	utf8str = (wyChar *)calloc(sizeof(wyChar), length + 1);

	//this step intended only to use WideCharToMultiByte
	MultiByteToWideChar(CP_ACP, 0, ansistr, -1, lpszw, length );

	//Conversion to ANSI (CP_ACP)
	WideCharToMultiByte(CP_UTF8, 0, lpszw, -1, utf8str, length, NULL, NULL);

	utf8str[length] = 0;

	delete[] lpszw;

	return utf8str;
}

wyWChar*
BlobMgmt::Utf8toUcs2(const wyChar *utf8str, wyInt32 len)
{
	wyWChar	*ucs2str = NULL;
	wyInt32 length = MultiByteToWideChar(CP_UTF8, 0, utf8str, len, NULL, NULL );

	ucs2str = AllocateBuffWChar(length + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, ucs2str, length);
	ucs2str[length] = 0;

	return ucs2str;
}

wyWChar*
BlobMgmt::AnsitoUcs2(const wyChar *ansistr, wyInt32 len)
{
	wyInt32 length = MultiByteToWideChar(CP_ACP, 0, ansistr, len, NULL, NULL );
	wyWChar *ucs2str = NULL;

	ucs2str = AllocateBuffWChar(length + 1);
	MultiByteToWideChar(CP_ACP, 0, ansistr, -1, ucs2str, length);
	ucs2str[length] = 0;

	return ucs2str;
}

wyChar*
BlobMgmt::Ucs2toAnsi(const wyChar *ucs2str, wyInt32 len)
{
	wyInt32 length = MultiByteToWideChar(CP_UTF8, 0, ucs2str, len, NULL, NULL );
	wyWChar *lpszw = NULL;
	wyChar *ansistr = NULL;
	lpszw = new wyWChar[length+1];

	ansistr = AllocateBuff(length + 1);
	MultiByteToWideChar(CP_UTF8, 0, ucs2str, -1, lpszw, length);
	
	WideCharToMultiByte(CP_ACP, 0, lpszw, -1, ansistr, length, NULL, NULL);

	ansistr[length] = 0;
	delete lpszw;

	return ansistr;
}

wyChar*
BlobMgmt::Ucs2toUtf8(const wyChar *ucs2str, wyInt32 len)
{
	wyWChar *lpszw = NULL;
	wyChar *utf8str = NULL;

    wyInt32 length = MultiByteToWideChar(CP_UTF8, 0, ucs2str, len, NULL, NULL );

	lpszw = new wyWChar[length+1];
    utf8str = AllocateBuff(length + 1);
	MultiByteToWideChar(CP_UTF8, 0, ucs2str, -1, lpszw, length);
	
	WideCharToMultiByte(CP_UTF8, 0, lpszw, -1, utf8str, length, NULL, NULL);

	utf8str[length] = 0;
	delete lpszw;
	return utf8str;
 }

void 
BlobMgmt::InsertTab(HWND hwndtab, wyInt32 pos, wyWChar *caption)
{
    TCITEM	tci;
	memset(&tci, 0, sizeof(TCITEM));

    tci.mask	    = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	tci.iImage	    = 0;	
	tci.pszText     = caption;
	tci.cchTextMax  = wcslen(caption);
	tci.lParam	    = (LPARAM)NULL;

	VERIFY(TabCtrl_InsertItem(m_hwndtab, pos, &tci)> -1);
}

wyBool
BlobMgmt::DisableAll(wyBool disable)
{
		
	wyInt32 ids[] = { IDC_MAINTAB, IDC_IMPORT, IDC_TEXT, IDC_IMAGE, IDC_TEXT };
	wyInt32 size = sizeof(ids)/ sizeof(ids[0]);
	wyInt32 i;

	for(i = 0; i < size; i++)
        EnableWindow(GetDlgItem(m_hwnddlg, ids[i]), (disable)?0:1);

     /*if(disable == wyTrue)
        SendMessage(m_hwndedit, SCI_SETTEXT, 0, (LPARAM)"");*/
		
	return wyTrue;
}

wyBool
BlobMgmt::SetEditFont(HWND hwndedit)
{
	EditorFont::SetFont(hwndedit, BLOBFONT, wyTrue);  
	return wyTrue;
}

wyBool
BlobMgmt::ShowData(wyBool setsave)
{
	wyString	blobdata, buttontext;

    if(m_piub->m_isblob && IsDataBinary(m_piub->m_data, m_piub->m_datasize ))
	{
		SendMessage(m_hwndedit, SCI_SETTEXT, 0,(LPARAM)"");
		
		if(setsave)
			SendMessage(m_hwndedit, SCI_SETSAVEPOINT, FALSE, 0);
		
		//post 8.01 painting. Paint the window with image
		VERIFY(InvalidateRect(m_hwndimage, NULL, TRUE));
		ShowBinary();

		TabCtrl_SetCurSel(m_hwndtab, 1);
	}	
    else
    {
        if(ExamineData() == wyFalse && m_piub->m_data)
            m_blobdata.SetAs(m_piub->m_data);

		if(m_piub->m_data)
			SendMessage(m_hwndedit, SCI_SETTEXT, m_blobdata.GetLength(),(LPARAM)m_blobdata.GetString());
		
		if(setsave)
			SendMessage(m_hwndedit, SCI_SETSAVEPOINT, FALSE, 0);

		ShowText();
		SetFocus(m_hwndedit);

		TabCtrl_SetCurSel(m_hwndtab, 0);
	}
       
	ShowBlobSize(m_piub->m_datasize);
    //SendMessage(m_hwndedit, SCI_EMPTYUNDOBUFFER, 0, 0);
	return wyTrue;
}

wyBool
BlobMgmt::ExamineData()
{
	DWORD				dwbytestoread = 0;
	wyInt32				headersize = 0;
	wyWChar				*buff;
	wyString			datastr;
	wyInt32				fileformat = 0;
	wyString			dataformat;
			
	if(!m_piub->m_data)
		return wyFalse;

	//Finding the fileformat whether its utf-8, utf-16(ucs2) or ansi 
	fileformat = DetectFileFormat(m_piub->m_data, dwbytestoread, &headersize);
	
	if(fileformat == NCP_UTF16)
	{
		buff = (wyWChar *)(m_piub->m_data + headersize);

		buff[(dwbytestoread - headersize)/sizeof(wyWChar)] = 0;

		datastr.SetAs(buff);

		dataformat.SetAs("ucs2"); //For setting the combo box value

	}
	else if(fileformat != NCP_UTF8)
	{
		dataformat.SetAs("unicode (utf-8)");

		if(m_piub->m_data)
				datastr.SetAs(m_piub->m_data + headersize);
		// there is a chance that the data may be Utf8 without BOM, so we are checking for the pattern
		if(CheckForUtf8(datastr) == wyFalse)
		{
				datastr.SetAs(m_piub->m_data, wyFalse); 
				dataformat.SetAs("US-ascii/Ansi");
		}
	}
   	
	else
	{
		dataformat.SetAs("unicode (utf-8)");
		datastr.SetAs(m_piub->m_data + headersize);
	}

	m_encodingtype.SetAs(dataformat);
	InitEncodingType();

	//Sets the combo box
	SetComboBox(dataformat.GetAsWideChar());

	return wyTrue;	
}

void
BlobMgmt::SetComboBox(wyWChar *codepagename)
{
	wyString	buffstr(codepagename);
    if(buffstr.CompareI("unicode (utf-8)") == 0 )//|| buffstr.CompareI("Western European (Windows)") == 0 || strstr(buffstr.GetString(), "Windows"))
	{
		m_blobdata.SetAs(m_piub->m_data);
		AddTextInCombo("Utf-8");
	}
	else if(buffstr.CompareI("US-ascii/Ansi")==0 )//|| buffstr.CompareI("ucs2") != 0)
	{
		m_blobdata.SetAs(m_piub->m_data, wyFalse);
        AddTextInCombo("US-ascii/Ansi");
	}
	else
		AddTextInCombo("ucs2");
}


// Function to Select text in the combobox. 
wyBool
BlobMgmt::AddTextInCombo(const wyChar * text)
{
	wyString		textstr(text);
	
	SendMessage(m_hwndcombo, CB_SELECTSTRING, 0, (LPARAM)textstr.GetAsWideChar());
	return wyTrue;
}

wyBool
BlobMgmt::IsDataBinary2(void * data, wyInt32 length)
{
    void    *tmp = NULL;
    wyChar* chardata = NULL;
	if(!data)
		return wyFalse;

    chardata = (wyChar*)data;

    for(int i=0; i<length; i++)
    {
        tmp = chardata + (i*sizeof(char));
        if(!memchr(tmp, 0, sizeof(char)) && !memchr(tmp, 0, sizeof(char)))
            return wyFalse;
    }

    if(length && (memchr(data, 0, length-1)))
		return wyTrue;
	
	return wyFalse;
}

wyBool
BlobMgmt::IsDataBinary(void * data, wyInt32 length)
{
	if(!data)
		return wyFalse;
    
    if(length && (memchr(data, 0, length-1)))
		return wyTrue;
	
	return wyFalse;
}

wyBool
BlobMgmt::ShowBinary()
{
	ShowWindow(m_hwndedit, FALSE);
	ShowWindow(m_hwndcombo, FALSE);
	ShowWindow(m_hwndimage, TRUE);
	return wyTrue;
}

wyBool
BlobMgmt::ShowText()
{	
	ShowWindow(m_hwndedit, TRUE);
	
	if(m_isblob == wyTrue)
		ShowWindow(m_hwndcombo, TRUE);
	else
		ShowWindow(m_hwndcombo, FALSE);

    ShowWindow(m_hwndimage, FALSE);
	return wyFalse;
}

// The wndproc of the image so that we can get paint.
LRESULT CALLBACK
BlobMgmt::ImageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BlobMgmt	* pib =(BlobMgmt*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_PAINT:
		return pib->ShowImage();
	}

	return CallWindowProc(pib->m_wporigpaintproc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK
BlobMgmt::TextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BlobMgmt	*pib = (BlobMgmt*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        //FindReplace DialogBox functions
	if(message == (wyUInt32)pGlobals->m_pcmainwin->m_findmsg)
	{
		if(pib->m_findreplace->FindReplace(hwnd, lParam) == wyFalse)
		{
			//when we are closing the find dialog, deleting the memory allocated for Find
			delete(pib->m_findreplace);
			pib->m_findreplace = NULL;
		}
		return 0;
	}

	switch(message)
	{
	case WM_KEYDOWN:
	    {
            wyInt32 ctl = GetKeyState(VK_CONTROL) & 0x8000;
            wyInt32  key = LOWORD(wParam);
            
			if(ctl && (key == 'a' || key == 'A' ))
            {
				SendMessage(hwnd, EM_SETSEL, 0, -1);
            }
			//To implement Find DialogBox
			else if(ctl && (key == 'f' || key == 'F' ))
            {
				pib->FindOrReplace(pib->m_hwndedit, wyFalse);
            }			
			//To implement Replace DialogBox
			else if(ctl && (key == 'h' || key == 'H' ))
            {
				pib->FindOrReplace(pib->m_hwndedit, wyTrue);
            }
            else if(key == VK_F3)
            {
                pib->OnAccelFindNext(hwnd);
            }
			else if(key == VK_ESCAPE)
            {
				if(pib->ProcessCancel() == wyTrue)
				{					
					VERIFY(yog_enddialog( pib->m_hwnddlg, 0));				
				}
                return 0;
            }
		}
		break;

		case WM_NCDESTROY:
			return 0;
			
	}
	return CallWindowProc(pib->m_wporigtextproc, hwnd, message, wParam, lParam);
}


// Clears the window with anything before so that a small image is imposed on a big image
// the big image is not there.
wyBool
BlobMgmt::PrepareScreen(HDC hdc)
{
	RECT	rect;
	
	VERIFY(GetClientRect(m_hwndimage, &rect));
	SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));
	VERIFY(Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom));

	return wyTrue;
}

LRESULT
BlobMgmt::ShowImage()
{
#ifndef VC6
	HDC             hdc;
	RECT			rectwin;
	wyInt32         renderwidth, renderheight;
	PAINTSTRUCT     ps;
	LPSTREAM        stream = NULL;
	HGLOBAL         glbmem;
	void            *glbbuffer;
    wyWChar         tempfilename[MAX_PATH+1] = {0}, path[MAX_PATH + 1] = {0};
	wyString		tempstr;
	HANDLE          hfile = INVALID_HANDLE_VALUE;
	DWORD           byteswritten = 0;

	if(!m_piub->m_data || m_piub->m_datasize == 0)
	{
		VERIFY(hdc = BeginPaint(m_hwndimage, &ps));
		VERIFY(EndPaint(m_hwndimage, &ps));
		return 0;
	}
	/* allocate global memory and copy image data in it*/
	VERIFY(glbmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, m_piub->m_datasize));
	if(!glbmem)
        return 0;

	/* lock the global memory and get a pointer */
	glbbuffer = GlobalLock(glbmem);
	/* copy the memory to buffer */
	CopyMemory(glbbuffer, m_piub->m_data, m_piub->m_datasize);
	/* unlock it */
	VERIFY(GlobalUnlock(glbmem)== NO_ERROR);
	/* create the stream */
	VERIFY(CreateStreamOnHGlobal(glbmem, FALSE, &stream)== S_OK);
	/* prepare window for painting */
	VERIFY(hdc = BeginPaint(m_hwndimage, &ps));
	/* clear the window */
	PrepareScreen(ps.hdc);

    if(pGlobals->m_configdirpath.GetLength() || SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (LPWSTR)path)))
    {
		if(pGlobals->m_configdirpath.GetLength())
		{
			//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());			
			wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);			
			path[MAX_PATH] = '\0';
		}
		
		else
		{
			wcscat(path, L"\\");
			wcscat(path, L"SQLyog");
		}
        
        VERIFY(GetTempFileName(path, L"img", 0, tempfilename));
 	    hfile = CreateFile(tempfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
								   NULL, NULL);
	    VERIFY(hfile != INVALID_HANDLE_VALUE);
	    VERIFY(WriteFile(hfile, m_piub->m_data, m_piub->m_datasize, &byteswritten, NULL));
	    VERIFY(CloseHandle(hfile));
    }
	tempstr.SetAs(tempfilename);
	
	WCHAR *wpath = GetWideString(tempstr.GetString());
	
	Gdiplus::Graphics	graphics(hdc);
	Gdiplus::Image		*image = new Gdiplus::Image(wpath);

	HeapFree(GetProcessHeap(), 0, wpath);
	/* in win95 image will be null so we exit */
	if(!image)
		goto ExitPara;

	/* the binary data might not be image so image.getlastatus will not return Ok */
	if(image->GetLastStatus()!= Gdiplus::Ok)
    {
		delete image;
		goto ExitPara;
	}

	/* get the window width and calculate the correct render stats */
	VERIFY(GetClientRect(m_hwndimage, &rectwin));

	renderheight =(((LONG)image->GetHeight())> rectwin.bottom)?(rectwin.bottom):(image->GetHeight());
	renderwidth  =(((LONG)image->GetWidth())> rectwin.right)?(rectwin.right):(image->GetWidth());

	graphics.DrawImage(image, 0, 0, renderwidth, renderheight);
	delete image;
	EndPaint(m_hwndimage, &ps);

ExitPara:
	/* free up stuff */
	VERIFY(DeleteFile(tempfilename));
	if(stream)
		stream->Release();

	VERIFY(GlobalFree(glbmem)== NULL);
#endif
	
	return 0;
}

// Function to process various things when the tab changes.
wyBool
BlobMgmt::OnTabSelChange()
{
	wyInt32   index;

	index  = TabCtrl_GetCurSel(m_hwndtab);

	switch(index)
	{
	case zero:
		if(IsDataBinary(m_piub->m_data, m_piub->m_datasize) && m_edit == wyFalse)
			SendMessage(m_hwndedit, SCI_SETREADONLY, TRUE, 0);
		else 
			SendMessage(m_hwndedit, SCI_SETREADONLY, FALSE, 0);
		
        ShowText();
		break;

	case one:
		ShowBinary();
		break;
	}
	return wyTrue;
}


wyBool
BlobMgmt::SaveToFile()
{
	OPENFILENAME	open;
	wyWChar         filename[MAX_PATH + 1] = {0};
	wyWChar			temp[MAX_PATH + 1] = {0};
	wyWChar			extention[]=L".json";
	HANDLE			hfile = NULL;
	DWORD			dwbytesread;

	if(!m_piub->m_data)
    {
		yog_message(m_hwnddlg, _(L"Cannot write NULL value!"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}

	memset(&open, 0, sizeof(open));

	open.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	open.hwndOwner         = m_hwnddlg;
	open.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
	if(m_isJson)
		open.lpstrFilter       = L"JSON Files(*.json)\0*.json\0";
	else
		open.lpstrFilter       = L"All Files(*.*)\0*.*\0";
	open.lpstrCustomFilter =(LPWSTR)NULL;
	open.nFilterIndex      = 1L;
	open.lpstrFile         = filename;
	open.nMaxFile          = MAX_PATH;
	open.lpstrTitle        = L"Save As";
	open.Flags             = OFN_OVERWRITEPROMPT;
	
	if(! GetSaveFileName(&open))
	   return wyFalse;

	// else successfull.
	wcscpy(temp,filename);
	wcsrev(temp);
	wcsrev(extention);
	if(m_isJson && wcsncmp(temp,extention,5))
	wcscat(filename,L".json");
	open.lpstrTitle		=	filename;
	hfile = CreateFile((LPCWSTR)filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
						 NULL);	

	if(hfile == INVALID_HANDLE_VALUE)
	{
		DisplayErrorText(GetLastError(), _("Could not create file !"));
		return wyFalse;
	}

	VERIFY(WriteFile(hfile, m_piub->m_data, m_piub->m_datasize, &dwbytesread, NULL));
	CloseHandle(hfile);
	return wyTrue;
}


wyBool
BlobMgmt::OpenFromFile()
{
	OPENFILENAME	open;
	wyWChar			filename[MAX_PATH+1] = {0};
	HANDLE			hfile = NULL;
	wyChar			*buffer = {0};
	DWORD			dwbytesread, filesize;
	wyString		bufferstr;

	memset(&open, 0, sizeof(open));

	open.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	open.hwndOwner         = m_hwnddlg;
	open.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
	open.lpstrFilter       = L"All Files(*.*)\0*.*\0";
	open.lpstrCustomFilter =(LPWSTR)NULL;
	open.nFilterIndex      = 1L;
	open.lpstrFile         = filename;
	open.nMaxFile          = MAX_PATH;
	open.lpstrTitle        = L"Open File";
	
	if(! GetOpenFileName(&open))
	   return wyFalse;

	// else successfull.
	hfile = CreateFile((LPCWSTR)filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
						 NULL);	

	if(hfile == INVALID_HANDLE_VALUE)
	{
		DisplayErrorText(GetLastError(), _("Could not open file !"));
		return wyFalse;
	}

	VERIFY((filesize = GetFileSize(hfile, NULL))!= INVALID_FILE_SIZE);
	VERIFY (buffer =(wyChar*)malloc(filesize + 1));
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if(!ReadFile(hfile, buffer, filesize, &dwbytesread, NULL))
	{
		DisplayErrorText(GetLastError(), _("Could not open file. The data is not changed"));
		free(buffer);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		CloseHandle(hfile);
		return wyFalse;
	}

	*(buffer + dwbytesread)= NULL;

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	// now we add the new buffer.
	if(m_newdata && m_newdata != m_olddata)
		free(m_newdata);
	
	m_newdata       = (wyChar *)buffer;
	m_newdatasize   = dwbytesread;
	
	/* store the old data too */
	m_piub->m_olddata		= m_piub->m_data;
	m_piub->m_olddatasize	= m_piub->m_datasize;
	m_piub->m_data			= m_newdata;
	m_piub->m_datasize		= dwbytesread;
	m_piub->m_isnull		= wyFalse;

	VERIFY(CloseHandle(hfile));

	// upddate with the new.
	ShowData(wyFalse);

	/* we need to do this after showdata coz showdata changes the modify flag to 0 */
	/* now if its text data then we set the text control modification flag */
	if(!memchr(buffer, 0, dwbytesread))
		SendMessage(m_hwndedit, EM_SETMODIFY, TRUE, 0);

	return wyTrue;
}

// Function to process stuff when the user presses OK.

wyBool
BlobMgmt::ProcessOK()
{
	wyChar				*newbuf = {0};
	wyInt32				bufsize, sel;
	wyString			newdatastr, codepagestr;
	
	// check in three steps
	if(SendMessage(GetDlgItem(m_hwnddlg, IDC_SETNULL), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		m_newdata			= NULL;
		m_piub->m_isnull	= wyTrue;
		m_piub->m_ischanged = wyTrue;

		return wyTrue;
	}

	sel = TabCtrl_GetCurSel(m_hwndtab);

	// then we see whether edit box has been modified
	if(sel == 0 && SendMessage(m_hwndedit, SCI_GETMODIFY, 0, 0) && m_isencodingchanged == wyFalse)
	{
		bufsize = SendMessage(m_hwndedit, SCI_GETTEXTLENGTH, 0, 0);

		VERIFY(newbuf = AllocateBuff(bufsize + 2));
		
		bufsize = SendMessage(m_hwndedit, SCI_GETTEXT, bufsize + 1,(LPARAM)newbuf);
		
		m_piub->m_data          = (wyChar *)newbuf;
		m_piub->m_datasize      = bufsize;
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
		
		if(! m_isJson)
			return wyTrue;
		if(m_isJson && IsValidJsonText(m_piub->m_data))
			return wyTrue;
		else
		{
	        m_piub->m_ischanged     = wyFalse;  // to disable the save option in dataview
			MessageBox(m_hwnddlg,L"Invalid JSON value. Please enter a valid JSON", 
				pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
			return wyFalse;
		}

	}


	// we change the blob otherwise
	if(sel == 1 && m_olddata != m_piub->m_data && m_isencodingchanged == wyFalse)
	{
		m_piub->m_olddata	= 	m_olddata;	
		m_piub->m_isnull	=   wyFalse;
		m_piub->m_ischanged =	wyTrue;

		return wyTrue;
	}
	
	if(m_isencodingchanged == wyTrue)
	{
		bufsize = SendMessage(m_hwndedit, SCI_GETTEXTLENGTH, 0, 0);
		VERIFY(newbuf = AllocateBuff(bufsize + 2));
		bufsize = SendMessage(m_hwndedit, SCI_GETTEXT, bufsize + 1,(LPARAM)newbuf);

		m_piub->m_data          = (wyChar *)newbuf;
		m_piub->m_datasize      = bufsize;
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
		m_blobdata.SetAs(newbuf);
		ProcessComboSelection(m_changedcombotext);
		if(! m_isJson)
			return wyTrue;
		if(m_isJson && IsValidJsonText(m_piub->m_data))
			return wyTrue;
		else
		{
			m_piub->m_ischanged     = wyFalse;   // to disable the save option in the dataview 
			
			MessageBox(m_hwnddlg,L"Invalid JSON value. Please Enter a valid JSON", 
				pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
			return wyFalse;
		}
		return wyTrue;
	}

	if((sel == 0 || sel == 1)&& m_checkboxstate == BST_CHECKED)
	{
		VERIFY(newbuf = AllocateBuff(1));
		newbuf[0] = '\0';

		m_piub->m_data          = (wyChar *)newbuf;
		m_piub->m_datasize      = 0;
		m_piub->m_isnull		= wyFalse;
		m_piub->m_ischanged     = wyTrue;
		if(! m_isJson)
			return wyTrue;
		if(m_isJson && IsValidJsonText(m_piub->m_data))
			return wyTrue;
		else
		{
			m_piub->m_ischanged     = wyFalse;   // to disable the save option in the dataview 
			
			MessageBox(m_hwnddlg,L"Invalid JSON value. Please Enter a valid JSON", 
				pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
			return wyFalse;
		}
		
	}
	
	m_piub->m_olddata       = NULL;
	m_piub->m_olddatasize   = 0;
	m_piub->m_data          = m_olddata;
	m_piub->m_datasize      = m_olddatasize;
	m_piub->m_ischanged     = wyFalse;
	if(m_isJson)    // IF no changes in the json viewer then on clicking OK it will close
		donot_close_window=wyFalse;

	return wyFalse;
}

// Function to process stuff when the user presses cancel.
wyBool
BlobMgmt::ProcessCancel()
{
	wyInt32				checked, msgreturn;
	
	//Message to save or discard changes made in blob viewer on clicking X button
	checked = Button_GetCheck(GetDlgItem(m_hwnddlg, IDC_SETNULL));

	if(SendMessage(m_hwndedit, SCI_GETMODIFY, 0, 0) || (m_olddata != m_piub->m_data) || (checked != m_checkboxstate))
	{
		//To avoid printing of ESC on pressing escape
		SendMessage(GetDlgItem(m_hwnddlg, IDC_TEXT), EM_SETREADONLY, TRUE, 0);
		
		msgreturn = MessageBox(m_hwnddlg, _(TEXT("You will lose all changes. Do you want to quit?")), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	    
		if(msgreturn == IDYES )
		{
			if(m_newdata != m_olddata)
			{
				if(m_newdata)
				{
					free(m_newdata);
					m_newdata = NULL;
				}
			}

			m_piub->m_olddata       = NULL;
			m_piub->m_olddatasize   = 0;
			m_piub->m_isoldnull     = m_oldnull;	
			m_piub->m_data          = m_olddata;
			m_piub->m_datasize      = m_olddatasize;
			m_piub->m_isnull        = m_oldnull;
		
			return wyTrue;
		}
		else
		{
			//To avoid printing of ESC on pressing escape
			SendMessage(GetDlgItem(m_hwnddlg, IDC_TEXT), EM_SETREADONLY, FALSE, 0);
			
			return wyFalse;
		}
	}
	
	return wyTrue;
}

void
BlobMgmt::MoveDialog()
{
	HICON hicon;

	//Set icon for dialog	
	hicon = CreateIcon(IDI_MYSQL);
	SendMessage(m_hwnddlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

	SetInitPos(m_hwnddlg, BLOBVIEWER_SECTION);
	//SetDialogPos(m_hwnddlg, BLOBVIEWER_SECTION);
}


void	
BlobMgmt::Resize(LPARAM lParam)
{
	wyInt32 dlght, dlgwd;
	HWND	hwndimport, hwndsave, hwndchk, hwndok, hwndcancel, hwndline;
	HWND	hwndsizetext, hwndsize;
	RECT rcttmp, rctopt;

	VERIFY(hwndimport = GetDlgItem(m_hwnddlg, IDC_IMPORT));
	VERIFY(hwndsave = GetDlgItem(m_hwnddlg, IDC_SAVETOFILE));
	VERIFY(hwndchk = GetDlgItem(m_hwnddlg, IDC_SETNULL));
	VERIFY(hwndok = GetDlgItem(m_hwnddlg, IDOK));
	VERIFY(hwndcancel = GetDlgItem(m_hwnddlg, IDCANCEL));
	VERIFY(hwndline = GetDlgItem(m_hwnddlg, IDC_TOPLINE));
	VERIFY(hwndsizetext = GetDlgItem(m_hwnddlg, IDC_SIZETEXT));
	VERIFY(hwndsize = GetDlgItem(m_hwnddlg, IDC_BLOBSIZE));
	
	/*m_hwndtab m_hwndedit	m_hwndimage	m_hwndnull	m_hwndcombo*/

	dlgwd = LOWORD(lParam);
	dlght = HIWORD(lParam);

	//tab
	GetClientRect(m_hwndtab, &rcttmp);
	MoveWindow(m_hwndtab, WIDTHBETBUTTON, WIDTHBETBUTTON, rcttmp.right, rcttmp.bottom, FALSE);

	//cmbo
	GetClientRect(m_hwndcombo, &rcttmp);
	MoveWindow(m_hwndcombo, dlgwd - WIDTHBETBUTTON - rcttmp.right, WIDTHBETBUTTON, rcttmp.right, rcttmp.bottom, FALSE);
	GetWindowRect(m_hwndcombo, &rctopt);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rctopt, 2);

	//Save button
	GetClientRect(hwndsave, &rcttmp);
	MoveWindow(hwndsave, rctopt.left - rcttmp.right -  WIDTHBETBUTTON, rctopt.top, 
		rcttmp.right, rctopt.bottom - rctopt.top, FALSE);
	GetWindowRect(hwndsave, &rctopt);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rctopt, 2);

	//Import button
	MoveWindow(hwndimport, rctopt.left - rcttmp.right -  WIDTHBETBUTTON, rctopt.top, 
		rcttmp.right, rctopt.bottom - rctopt.top, FALSE);
	GetWindowRect(hwndimport, &rctopt);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rctopt, 2);

	//set null check-box
	GetClientRect(hwndchk, &rcttmp);
	MoveWindow(hwndchk, rctopt.left - rcttmp.right -  WIDTHBETBUTTON, rctopt.top, 
		rcttmp.right, rctopt.bottom - rctopt.top, FALSE);

	//Cancel
	GetClientRect(hwndcancel, &rcttmp);
	MoveWindow(hwndcancel, dlgwd - WIDTHBETBUTTON - rcttmp.right,  dlght- WIDTHBETBUTTON - rcttmp.bottom, 
		rcttmp.right, rcttmp.bottom, FALSE);
	GetWindowRect(hwndcancel, &rctopt);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rctopt, 2);

	//OK
	MoveWindow(hwndok, rctopt.left - WIDTHBETBUTTON - rcttmp.right, rctopt.top, 
		rcttmp.right, rcttmp.bottom, FALSE);

	//Line
	GetClientRect(hwndline, &rcttmp);
	MoveWindow(hwndline, WIDTHBETBUTTON, rctopt.top - WIDTHBETBUTTON,	
		dlgwd - WIDTHBETBUTTON * 2, 1, FALSE);

	//Editor
	GetWindowRect(m_hwndcombo, &rctopt);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rctopt, 2);

	GetWindowRect(hwndline, &rcttmp);
	MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rcttmp, 2);

	MoveWindow(m_hwndedit, rcttmp.left, rctopt.bottom + WIDTHBETBUTTON / 2, 
		rcttmp.right - rcttmp.left, rcttmp.top - rctopt.bottom - WIDTHBETBUTTON + WIDTHBETBUTTON / 2, FALSE);

	//Image
	MoveWindow(m_hwndimage, rcttmp.left, rctopt.bottom + WIDTHBETBUTTON / 2, 
		rcttmp.right - rcttmp.left, rcttmp.top - rctopt.bottom - WIDTHBETBUTTON + WIDTHBETBUTTON / 2, FALSE);
    
	//Size
	GetClientRect(hwndsizetext, &rcttmp);
	MoveWindow(hwndsizetext, WIDTHBETBUTTON, dlght - WIDTHBETBUTTON - rcttmp.bottom, 
		rcttmp.right, rcttmp.bottom, FALSE);
		
	GetClientRect(hwndsize, &rctopt);
	MoveWindow(hwndsize, rcttmp.right +  WIDTHBETBUTTON ,  dlght - WIDTHBETBUTTON - rcttmp.bottom, 
		rctopt.right, rctopt.bottom, FALSE);
    
	InvalidateRect(m_hwnddlg, NULL, TRUE);

}

void	
BlobMgmt::ShowBlobSize(wyInt32 datasize)
{
	wyString	strnumber ;
	//XFormat
	NUMBERFMT	nf;
	wyInt32		nsize;
	wyWChar		*formattednumber;
	wyString	str;

	strnumber.Sprintf("%d", datasize);
	memset(&nf, 0, sizeof(nf));
	
	nf.lpThousandSep = L",";
	nf.lpDecimalSep = L".";
	nf.LeadingZero = 1;
	nf.Grouping = 3;
	nf.NegativeOrder = 1;

	//string length + noof commas + 1 
	nsize = strnumber.GetLength() + strnumber.GetLength()/3 + 1;
	formattednumber = AllocateBuffWChar(nsize);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 
						0, 
						(LPCWSTR)strnumber.GetAsWideChar(), 
						&nf, 
						formattednumber, 
						nsize))
	{
		strnumber.SetAs(formattednumber);
	}

	//adding "bytes" 
	strnumber.AddSprintf(" bytes");
   		
	SendMessage(GetDlgItem(m_hwnddlg, IDC_BLOBSIZE ), WM_SETTEXT, strnumber.GetLength(), (LPARAM) strnumber.GetAsWideChar());

	if(formattednumber)
		free(formattednumber);
}

// Function to implement Find and Replace in Blob Viewer.
wyBool
BlobMgmt::FindOrReplace(HWND hwnd, wyBool uIsReplace)
{
	static FINDREPLACE  fr;
	static wyWChar      texttofind[FIND_STR_LEN + 1];
	static wyWChar      texttoreplace[FIND_STR_LEN + 1];
  	wyChar				*textfind = NULL; 
   	wyUInt32            start, end;
	wyString			findtextstr;
    wyInt32             ret, len = 0;
	
	// If the dialog box is available then we dont do anything.
	if(pGlobals->m_pcmainwin->m_finddlg && IsWindowVisible(pGlobals->m_pcmainwin->m_finddlg))
	{		
		SetFocus(pGlobals->m_pcmainwin->m_finddlg);
		return wyTrue;
	}
	
	// set the flag
	pGlobals->m_findreplace = wyTrue;
	
	// prepare the structure.
	memset(&fr, 0, sizeof(FINDREPLACE));

	fr.lStructSize = sizeof(FINDREPLACE);

	start = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	len = (end - start)+ 1;
	textfind = AllocateBuff(len); 
	ret = SendMessage(hwnd, SCI_GETSELTEXT, (WPARAM)len, (LPARAM)textfind);
	
	findtextstr.SetAs(textfind );
	len = findtextstr.GetLength();
	if(len > 256)
	{
		textfind[256] = 0;
		findtextstr.SetAs(textfind);
	}
	free(textfind );

	//To remember last string in find dialog box
	if(len != 0)
	{
		//wcscpy(texttofind, findtextstr.GetAsWideChar());
		wcsncpy(texttofind, findtextstr.GetAsWideChar(), FIND_STR_LEN);
		texttofind[FIND_STR_LEN] = '\0';
	}

    if(!findtextstr.GetLength())
	{
        //wcscpy(texttofind, pGlobals->m_pcmainwin->m_findtext.GetAsWideChar());
		wcsncpy(texttofind, pGlobals->m_pcmainwin->m_findtext.GetAsWideChar(), FIND_STR_LEN);
		texttofind[FIND_STR_LEN] = '\0';
	}
    else
	{
        pGlobals->m_pcmainwin->m_findtext.SetAs(findtextstr);
	}
		
    fr.hwndOwner = hwnd;

	fr.lpstrFindWhat = texttofind;
	fr.wFindWhatLen = FIND_STR_LEN;

	m_findreplace = new FindAndReplace(hwnd);
	
	if(uIsReplace)
	{
		fr.lpstrReplaceWith = texttoreplace;
		fr.wReplaceWithLen = FIND_STR_LEN;
	}
	else
    {
        if(pGlobals->m_pcmainwin->m_frstruct.hwndOwner == NULL)
        {
            pGlobals->m_pcmainwin->m_frstruct.hwndOwner = fr.hwndOwner;
            pGlobals->m_pcmainwin->m_frstruct.Flags = fr.Flags = FR_DOWN;
        }
        else
        {
            fr.Flags = pGlobals->m_pcmainwin->m_frstruct.Flags; 
        }
    }

	// Call the appropriate function.
	if(uIsReplace && (GetFocus() == hwnd))
		pGlobals->m_pcmainwin->m_finddlg = ReplaceText(&fr);
	else
		pGlobals->m_pcmainwin->m_finddlg = FindText(&fr);

    //subclass the FindText dialog
    if(!pGlobals->m_pcmainwin->m_findproc)
    {
        pGlobals->m_pcmainwin->m_findproc = (WNDPROC)SetWindowLongPtr(pGlobals->m_pcmainwin->m_finddlg, 
                                                                   GWLP_WNDPROC, 
                                                                   (LONG_PTR)FindAndReplace::FindWndProc);
    }
	
	return wyTrue;
}

void 
BlobMgmt::OnAccelFindNext(HWND hwnd)
{
	FindAndReplace** pfindrep = NULL;
    LPFINDREPLACE   temp;

    //get the FindReplace pointer for the window
    pfindrep = &m_findreplace;
    
    //if search string is already there and the window handle is valid
    if(pGlobals->m_pcmainwin->m_findtext.GetLength() && hwnd)
    {
        if(!pGlobals->m_pcmainwin->m_finddlg)
        {
            pGlobals->m_findreplace = wyTrue;
        }
        else
        {
            hwnd = pGlobals->m_pcmainwin->m_frstruct.hwndOwner;
        }

        pGlobals->m_pcmainwin->m_frstruct.hwndOwner = hwnd;
        pGlobals->m_pcmainwin->m_frstruct.lpstrFindWhat = pGlobals->m_pcmainwin->m_findtext.GetAsWideChar();

        //if no FindReplace is allocated, allocate it
        if(*pfindrep == NULL)
            *pfindrep = new FindAndReplace(hwnd);
        
        temp = &pGlobals->m_pcmainwin->m_frstruct;

        //send the find next message to the scintilla window
        SendMessage(hwnd, pGlobals->m_pcmainwin->m_findmsg, (WPARAM)0, (LPARAM)temp);
    }
    else
    {
       FindOrReplace(hwnd, wyFalse);
    }
}
wyBool
	BlobMgmt::IsValidJsonText(wyString txt)
{
	MYSQL_RES       *myres;
	MYSQL_ROW		row;
	wyString query;
	MDIWindow		*wnd = NULL;
	wyString		validator;
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;
	query.SetAs("SELECT JSON_VALID('");
	query.AddSprintf(("%s')"),txt.GetString());
	myres	=	ExecuteAndGetResult(GetActiveWin(),wnd->m_tunnel,&wnd->m_mysql,query,wyFalse);
	// wyFalse so that SELECT JSON_VALID is not displayed in the History tab
	if(!myres && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	row = wnd->m_tunnel->mysql_fetch_row(myres);
	validator.AddSprintf(row[0]);
	mysql_free_result(myres);
	if(validator.GetAsInt32()==1)
	return wyTrue;
	else
		return wyFalse;
}
