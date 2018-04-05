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


#include <shlobj.h>

#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "ExportMultiFormat.h"
#include "ExportMultiFormat.h"
#include "OtherDialogs.h"
#include "FileHelper.h"
#include "SQLMaker.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "MySQLVersionHelper.h"

extern	PGLOBALS		 pGlobals;

/*
	Implementation of export as CSV/XML Dialog.
	This in itself calls the character dialog box and user can export the data in the 
	specified format.
																					*/

ExportMultiFormat::ExportMultiFormat()
{
	m_esch.m_towrite = wyTrue;
	
	//if import from csv then read from EXPORTCSVESCAPING section
	m_esch.m_isclipboard = wyFalse;
	m_esch.ReadFromFile(EXPORTCSVSECTION);

	m_p = new Persist;
	m_p->Create("IMPORTCSV");//This persist pointer will be used  CSV import.
	m_p_XML=new Persist;
	m_p_XML->Create("IMPORTXML");

}

ExportMultiFormat::~ExportMultiFormat()
{
}

wyBool 
ExportMultiFormat::Create(HWND hwnd, wyChar *db, wyChar *table, Tunnel * tunnel, PMYSQL mysql)
{
	wyInt64 ret;

	m_hwndparent    =	hwnd;
	m_mysql	        = mysql;
	m_tunnel        = tunnel;

	m_db.SetAs(db);
	m_table.SetAs(table);

    //Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_IMPORTCSV),
		hwnd, ExportMultiFormat::DlgProc, (LPARAM)this);

	if(ret)
        return wyTrue;

    return wyFalse;
}
//function to create import from XML dialog
wyBool 
ExportMultiFormat::CreateXML(HWND hwnd, wyChar *db, wyChar *table, Tunnel * tunnel, PMYSQL mysql)
{
	wyInt64 ret;

	m_hwndparent    =	hwnd;
	m_mysql	        = mysql;
	m_tunnel        = tunnel;

	m_db.SetAs(db);
	m_table.SetAs(table);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_IMPORTXML),
		hwnd, ExportMultiFormat::DlgProcXML, (LPARAM)this);

	if(ret)
        return wyTrue;

    return wyFalse;
}

INT_PTR CALLBACK
ExportMultiFormat::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ExportMultiFormat * picsv =(ExportMultiFormat*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
	    picsv->OnWmInitDlgValues(hwnd);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/100-import-csv-data-using-load-local");
		return TRUE;

	case WM_COMMAND:
		if(picsv->OnWmCommand(hwnd, wParam, lParam) == wyFalse)
            return 0;
		break;

	case WM_DESTROY:
		delete picsv->m_p;
		delete picsv->m_p_XML;
		break;

	}

	return 0;
}
INT_PTR CALLBACK
ExportMultiFormat::DlgProcXML(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ExportMultiFormat * picsv =(ExportMultiFormat*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
	    picsv->OnWmInitDlgValuesXML(hwnd);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/101-import-XML-data-using-load-local");
		return TRUE;

	case WM_COMMAND:
		if(picsv->OnWmCommandXML(hwnd, wParam, lParam) == wyFalse)
            return 0;
		break;

	case WM_DESTROY:
		delete picsv->m_p;
		delete picsv->m_p_XML;
		break;

	}

	return 0;
}
void
ExportMultiFormat::OnWmInitDlgValues(HWND hwnd)
{
	m_hwnd = hwnd;
	VERIFY(m_hwndtlist      = GetDlgItem(hwnd, IDC_TABLELIST));
	VERIFY(m_hwndfieldlist  = GetDlgItem(hwnd, IDC_FIELDLIST));
	VERIFY(m_hwndedit       = GetDlgItem(hwnd, IDC_EXPORTFILENAME));
	
	m_p->Add(hwnd, IDC_EXPORTFILENAME, "FileName", "", TEXTBOX);
	m_p->Add(hwnd, IDC_LOWPRIORITY, "LowPriority", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CONCURRENT, "Concurrent", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_REPLACE, "Replace", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_IGNORE, "Ignore", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_IGNORELINES, "IgnoreLines", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_IGNORENUM, "IgnoreLinesNumber", "", TEXTBOX);

	if(SendMessage(GetDlgItem(hwnd, IDC_IGNORELINES), BM_GETCHECK, 0, 0))
		EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), TRUE);

	SetStaticText();
	FillInitData();
    return;
}

void
ExportMultiFormat::OnWmInitDlgValuesXML(HWND hwnd)
{
	m_hwnd = hwnd;
	VERIFY(m_hwndtlist      = GetDlgItem(hwnd, IDC_TABLELIST));
	VERIFY(m_hwndfieldlist  = GetDlgItem(hwnd, IDC_FIELDLIST));
	VERIFY(m_hwndedit       = GetDlgItem(hwnd, IDC_EXPORTFILENAME));
	
	m_p_XML->Add(hwnd, IDC_EXPORTFILENAME, "FileName", "", TEXTBOX);
	m_p_XML->Add(hwnd, IDC_LOWPRIORITY, "LowPriority", "0", CHECKBOX);
	m_p_XML->Add(hwnd, IDC_CONCURRENT, "Concurrent", "0", CHECKBOX);
	m_p_XML->Add(hwnd, IDC_REPLACE, "Replace", "0", CHECKBOX);
	m_p_XML->Add(hwnd, IDC_IGNORE, "Ignore", "0", CHECKBOX);
	m_p_XML->Add(hwnd, IDC_IGNORELINES, "IgnoreLines", "0", CHECKBOX);
	m_p_XML->Add(hwnd, IDC_IGNORENUM, "IgnoreLinesNumber", "", TEXTBOX);
	m_p_XML->Add(hwnd, IDC_ROWSIDTEXT, "rowsidentifiedby", "row", TEXTBOX);

	if(SendMessage(GetDlgItem(hwnd, IDC_IGNORELINES), BM_GETCHECK, 0, 0))
		EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), TRUE);
	if(SendMessage(GetDlgItem(hwnd, IDC_ROWSIDBY), BM_GETCHECK, 0, 0))
		EnableWindow(GetDlgItem(hwnd, IDC_ROWSIDTEXT), TRUE);
	FillInitData();
    return;
}

wyBool
ExportMultiFormat::OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyBool ret;
	wyString	tempcharset;

    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		m_p->Cancel();
		m_esch.m_towrite = wyTrue;
		yog_enddialog(hwnd, 0);
		break;

	case IDC_SELECTALL:
		SendMessage(m_hwndfieldlist, LB_SETSEL, TRUE, -1);
		break;

	case IDC_UNSELECTALL:
		SendMessage(m_hwndfieldlist, LB_SETSEL, FALSE, -1);
		break;

	case IDC_EXPFILESELECT:
		SetExpFileName();
		break;

	case IDC_TABLELIST:
		if(HIWORD(wparam)== LBN_SELCHANGE)
		{
            wyWChar  table[SIZE_512] = {0};
			wyInt32 index = SendMessage(m_hwndtlist, LB_GETCURSEL, 0, 0);

			if(index == LB_ERR)
				return wyFalse;

			SendMessage(m_hwndtlist, LB_GETTEXT, index,(LPARAM)table);

            m_table.SetAs(table);
			FillFields();
		}
		break;

	case IDC_CHANGE:
		ret = GetEscapeCharacters();
        if(ret == wyTrue)
		    SetStaticText();
		break;

	case IDOK:
		if(SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, 0)== 0){
			SetFocus(m_hwndedit);
			yog_message(hwnd, _(L"Please specify a filename"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		if(SendMessage(m_hwndfieldlist, LB_GETSELCOUNT, 0, 0)== 0){
			SetFocus(m_hwndfieldlist);
			yog_message(hwnd, _(L"Select at least one field"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		ret = ImportData();

		if(ret)
			yog_enddialog(hwnd, 1);

		break;

	case IDC_LOWPRIORITY:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_CONCURRENT), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_CONCURRENT:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_LOWPRIORITY), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_REPLACE:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_IGNORE:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_REPLACE), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_IGNORELINES:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), TRUE);
		else
			EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), FALSE);
		break;

	case IDC_CHARSETCOMBO:
		{
			if((HIWORD(wparam))== CBN_SELENDOK)
			{
				FetchSelectedCharset(hwnd, &tempcharset, wyFalse);
				if(tempcharset.GetLength())
					m_importfilecharset.SetAs(tempcharset);              
			}	
		}
	}
    return wyTrue;
}
wyBool
ExportMultiFormat::OnWmCommandXML(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyBool ret;
	wyString	tempcharset;

    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		m_p->Cancel();
		m_esch.m_towrite = wyTrue;
		yog_enddialog(hwnd, 0);
		break;

	case IDC_SELECTALL:
		SendMessage(m_hwndfieldlist, LB_SETSEL, TRUE, -1);
		break;

	case IDC_UNSELECTALL:
		SendMessage(m_hwndfieldlist, LB_SETSEL, FALSE, -1);
		break;

	case IDC_EXPFILESELECT:
		SetExpFileName(wyFalse);
		break;

	case IDC_TABLELIST:
		if(HIWORD(wparam)== LBN_SELCHANGE)
		{
            wyWChar  table[SIZE_512] = {0};
			wyInt32 index = SendMessage(m_hwndtlist, LB_GETCURSEL, 0, 0);

			if(index == LB_ERR)
				return wyFalse;

			SendMessage(m_hwndtlist, LB_GETTEXT, index,(LPARAM)table);

            m_table.SetAs(table);
			FillFields();
		}
		break;

	case IDOK:
		if(SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, 0)== 0){
			SetFocus(m_hwndedit);
			yog_message(hwnd, _(L"Please specify a filename"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		if(SendMessage(m_hwndfieldlist, LB_GETSELCOUNT, 0, 0)== 0){
			SetFocus(m_hwndfieldlist);
			yog_message(hwnd, _(L"Select at least one field"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		ret = ImportData(wyFalse);

		if(ret)
			yog_enddialog(hwnd, 1);

		break;

	case IDC_LOWPRIORITY:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_CONCURRENT), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_CONCURRENT:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_LOWPRIORITY), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_REPLACE:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_IGNORE:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			SendMessage(GetDlgItem(hwnd, IDC_REPLACE), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case IDC_IGNORELINES:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), TRUE);
		else
			EnableWindow(GetDlgItem(hwnd, IDC_IGNORENUM), FALSE);
		break;
	case IDC_ROWSIDBY:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0 , 0)== BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDC_ROWSIDTEXT), TRUE);
		else
			EnableWindow(GetDlgItem(hwnd, IDC_ROWSIDTEXT), FALSE);

		break;

	case IDC_CHARSETCOMBO:
		{
			if((HIWORD(wparam))== CBN_SELENDOK)
			{
				FetchSelectedCharset(hwnd, &tempcharset, wyFalse);
				if(tempcharset.GetLength())
					m_importfilecharset.SetAs(tempcharset);              
			}	
		}
	}
    return wyTrue;
}

wyBool 
ExportMultiFormat::FillInitData()
{
	HWND			hwndcombo = NULL;
	wyInt32			index = 0, count, width;
    wyString		query;
	wyString		myrowstr;
	MYSQL_ROW		myrow;
	MYSQL_RES		*myres;
	wyBool			ismysql41 = (GetActiveWin())->m_ismysql41;

	// get all the tables and add them to the listbox.
	count = PrepareShowTable(m_tunnel, m_mysql, m_db, query);

	myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	// Sets the H-scroll for listbox
	//SendMessage(m_hwndtlist, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)SIZE_512, (LPARAM)0);

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		SendMessage(m_hwndtlist, LB_ADDSTRING, 0,(LPARAM)myrowstr.GetAsWideChar());
	}

	if(m_table.GetLength())
		index = SendMessage(m_hwndtlist, LB_FINDSTRING, -1,(LPARAM)m_table.GetAsWideChar());

	if(index != LB_ERR)
		SendMessage(m_hwndtlist, LB_SETCURSEL, index, 0);
	else
		SendMessage(m_hwnd, LB_SETCURSEL, 0, 0);

    //sets maximum width of listbox	
    width = SetListBoxWidth(m_hwndtlist);
	
	// Sets the H-scroll for listbox
	SendMessage(m_hwndtlist, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)width + 5, (LPARAM)0);
	
	m_tunnel->mysql_free_result(myres);

	FillFields();

	// Init Charset combo
	if(IsMySQL5038( m_tunnel, m_mysql) == wyTrue)
	{
		InitCharacterSetCombo(m_hwnd);

		//Set the 'Charset' combo text(Set utf8 if file type is utf8, set ucs2 if it's utf16)
		SetComboText();		
	}
	
	else
	{
		VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_CHARSETCOMBO));
		VERIFY(EnableWindow(hwndcombo, FALSE));
	}

	return wyTrue;
}

///Set the default charset(selected file format type) for charset combo box.
void
ExportMultiFormat::SetComboText()
{
	HANDLE		hfile = NULL;
	DWORD		dwbytestoread;
	wyChar		*data = NULL;
	wyInt32     headersize;
	wyString	datastr, filenamestr;
	wyInt32		ret = 0, fileformat;
	HWND		hwndcombo;

	/// Charset with csv supports since v 5.0.38
	if(IsMySQL5038(m_tunnel, m_mysql) == wyFalse)
		return;		

	//Get the file name got selected from text box
	GetFileName(filenamestr);

	if(filenamestr.GetLength())
	{	
		hfile = CreateFile(filenamestr.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);                     
	
		//before here throughing error when file handle with given path is not exist.. 
		if(hfile == INVALID_HANDLE_VALUE)
		{
			//DisplayErrorText(GetLastError(), "Could not open file.");
			return;
		}	  		
		dwbytestoread = GetFileSize(hfile, NULL);
		
		//We limit the size of the memory to be allocated is 2MB(it may not possible if try for memory like 2GB)
		if(dwbytestoread > MAXFILESIZE_2MB - 10)
			dwbytestoread = MAXFILESIZE_2MB - 10;

		data = new wyChar[dwbytestoread + 10];
		
		VERIFY (ReadFile(hfile, data, dwbytestoread+1, &dwbytestoread, NULL));
		data[dwbytestoread/sizeof(wyChar)] = 0;

		//Detect the file format & set the Charset combo box
		fileformat = DetectFileFormat(data, dwbytestoread, &headersize);
		
		if(fileformat == NCP_UTF16)
			datastr.SetAs("ucs2");
		
		else if(fileformat != NCP_UTF8)
		{
			datastr.SetAs(data + headersize);
			// there is a chance that the data may be Utf8 without BOM, so we are checking for the pattern
			if(CheckForUtf8(datastr) == wyTrue)
				datastr.SetAs("utf8");
		}

		else
			datastr.SetAs(STR_DEFAULT);
	}

	//If the filename text box is empty 
	else
		datastr.SetAs(STR_DEFAULT);

	if(data)
	{
		free(data);
		data = NULL;
	}
	m_importfilecharset.SetAs(datastr.GetString());
	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_CHARSETCOMBO));	
	ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)datastr.GetAsWideChar());
	if(ret == CB_ERR)
		SendMessage(hwndcombo, CB_SETCURSEL,(WPARAM)0, 0);
	else
		SendMessage(hwndcombo, CB_SETCURSEL,(WPARAM)ret, 0);

	if(hfile)
		VERIFY(CloseHandle(hfile));
}

wyBool 
ExportMultiFormat::FillFields()
{
    wyString    query;
	wyString	myrowstr;
	MYSQL_ROW	myrow;
	MYSQL_RES	*myres;
	wyBool		ismysql41 = (GetActiveWin())->m_ismysql41;
	wyInt32     width;

	// first remove all the content of the field field list and diable two buttons.
	SendMessage(m_hwndfieldlist, LB_RESETCONTENT, 0, 0);
	// get all the tables and add them to the listbox.
	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), m_table.GetString());

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	// Sets H-scroll to the list box
	//SendMessage(m_hwndfieldlist, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)SIZE_512, (LPARAM)0);

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[GetFieldIndex(myres,"field", m_tunnel, m_mysql)], ismysql41);
	        SendMessage(m_hwndfieldlist, LB_ADDSTRING, 0,(LPARAM)myrowstr.GetAsWideChar());
	}

    //sets maximum width of listbox	
    width = SetListBoxWidth(m_hwndfieldlist);
	
	// Sets the H-scroll for listbox
	SendMessage(m_hwndfieldlist, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)width + 5, (LPARAM)0);

	SendMessage(m_hwndfieldlist, LB_SETSEL, TRUE, -1);

	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

// Gets a file name and add it to edit box.
wyBool 
ExportMultiFormat::SetExpFileName(wyBool iscsv)
{
	wyWChar  filename[MAX_PATH + 1] = {0};
	if(iscsv==wyTrue)
	{
		if(!InitOpenFile(m_hwnd, filename, CSVINDEX, MAX_PATH))
		return wyFalse;
	}
	else
	{
		if(!InitOpenFile(m_hwnd, filename, XMLINDEX, MAX_PATH))
		return wyFalse;
	}

	SendMessage(m_hwndedit, WM_SETTEXT, 0,(LPARAM)filename);

	//Set the 'Charset' combo text(Set utf8 if file type is utf8, set ucs2 if it's utf16
	SetComboText();

	return wyTrue;
}


wyBool 
ExportMultiFormat::SetStaticText()
{
	
	wyString	escape, lescape, fescape, enclosed;

	if(m_esch.m_isescaped == wyTrue)
	{
		if(strlen(m_esch.m_escape)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			escape.SetAs(m_esch.m_escape);
			SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)escape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");

	if(m_esch.m_isfieldsterm)
	{
		if(strlen(m_esch.m_fescape)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			fescape.SetAs(m_esch.m_fescape);
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)fescape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");

	if(m_esch.m_isenclosed == wyTrue)
	{
		if(m_esch.m_isoptionally == wyTrue)
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0,(LPARAM)_(L"Optionally enclosed by :"));
		else
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0,(LPARAM)_(L"Enclosed by :"));

		if(strlen(m_esch.m_enclosed)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			enclosed.SetAs(m_esch.m_enclosed);
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0, (LPARAM)enclosed.GetAsWideChar());
		}
	}
	else
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0, (LPARAM)_(L"Enclosed by :"));
		if(strlen(m_esch.m_enclosed)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0, (LPARAM)L"<NONE>");
		else
		{
			enclosed.SetAs(m_esch.m_enclosed);
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0, (LPARAM)enclosed.GetAsWideChar());
		}
	}

	if(m_esch.m_islineterminated)
	{
		if(strlen(m_esch.m_lescape)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0, (LPARAM)L"<NONE>");
		else
		{
			lescape.SetAs(m_esch.m_lescape);
			SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0,(LPARAM)lescape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");

	return wyTrue;
}


wyBool 
ExportMultiFormat::GetEscapeCharacters()
{
	EscapeChar	cech;
	cech.GetEscapeChar(m_hwnd, &m_esch, wyFalse, wyFalse);
	return wyTrue;
}

// The real function to export data.
wyBool 
ExportMultiFormat::ImportData(wyBool iscsv)
{
    wyString        query, msg;
	CShowWarning	warn;
    MYSQL_RES       *res;
	if(iscsv==wyTrue)
	{
	VERIFY(PrepareQuery(query));
	}
	else
	{
	VERIFY(PrepareQueryXML(query));
	}

	SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    //mysql_thread_init();
	res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false);
    //mysql_thread_end();

	if( GetForegroundWindow() != m_hwnd)
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);

	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	    goto cleanup;

	msg.SetAs(m_tunnel->mysql_info(*m_mysql));
	m_tunnel->mysql_free_result(res);

	/* starting from 4.07 we show it in a different dialog that allows us to show warnings */
	warn.Create(m_hwnd, m_tunnel, m_mysql, L"CSV Import Result", msg.GetAsWideChar());
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;

cleanup:

	ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyFalse;
}

///Create queryy for Load inifile , use charset if version >= 5.0.38
wyInt32 
ExportMultiFormat::PrepareQuery(wyString &query)
{
    wyWChar	    fieldname[SIZE_512] = {0}, ignore[SIZE_64+1] = {0};
    	wyUInt32    selcount=0, fieldcount=0,i=0;
	wyString	filenamestr, fieldnamestr, ignorestr;
	wyBool		ismysql538 = wyFalse;

	selcount	= SendMessage(m_hwndfieldlist, LB_GETSELCOUNT, 0, 0);
	fieldcount	= SendMessage(m_hwndfieldlist, LB_GETCOUNT, 0, 0);

	GetFileName(filenamestr);	
	
	//Gets the Character set
	if(IsMySQL5038(m_tunnel, m_mysql) == wyTrue)
		ismysql538 = wyTrue;
	
	query.Sprintf("load data ");
	
	// get low_priority or concurrent
	if(SendMessage(GetDlgItem(m_hwnd, IDC_LOWPRIORITY), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("low_priority ");
	else if(SendMessage(GetDlgItem(m_hwnd, IDC_CONCURRENT), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("concurrent ");

	// add the file name
	query.AddSprintf("local infile '%s' ", filenamestr.GetString());

	// get replace ignore
	if(SendMessage(GetDlgItem(m_hwnd, IDC_REPLACE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("replace ");
	else if(SendMessage(GetDlgItem(m_hwnd, IDC_IGNORE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("ignore ");
	
	// add the table name
	query.AddSprintf("into table `%s`.`%s` ", m_db.GetString(), m_table.GetString());

	///Add 'Charcter set' if importing to mysql version >= 5.0.38
	if(ismysql538 == wyTrue && m_importfilecharset.GetLength() != 0 && m_importfilecharset.CompareI(STR_DEFAULT) != 0)
          query.AddSprintf("character set '%s' ", m_importfilecharset.GetString());
    	
	// if fields stuff then/
	if(m_esch.m_isescaped == wyTrue || m_esch.m_isenclosed == wyTrue || m_esch.m_isfieldsterm == wyTrue)
		query.Add("fields ");

	// now we add the fields stuff.
	if(m_esch.m_isescaped == wyTrue)
	{
        if(strcmp(m_esch.m_escape, "\\") == 0)
            strcpy(m_esch.m_escape, "\\\\");

		if(strlen(m_esch.m_escape) > 0)
			query.AddSprintf("escaped by '%s' ", m_esch.m_escape);
		else
			query.Add("escaped by '' ");
	}

	// if fixed.
	if(m_esch.m_isfixed)
	{
		if(m_esch.m_isescaped == wyFalse)
			query.Add("fields ");

		query.Add("enclosed by '' terminated by '' ");
	}
	else
	{
		// fields terminated by.
		if(m_esch.m_isfieldsterm)
		{
			if(strlen(m_esch.m_fescape)> 0)
				query.AddSprintf("terminated by '%s' ", m_esch.m_fescape);
			else
				query.Add("terminated by '' ");
		}

		if(m_esch.m_isenclosed)
		{
			if(m_esch.m_isoptionally)
				query.Add("optionally ");

			if(strlen(m_esch.m_enclosed) > 0)
				query.AddSprintf("enclosed by '%s' ", m_esch.m_enclosed);
			else
				query.Add("enclosed by '' ");
		}
	}

	// line termination.
	if(m_esch.m_islineterminated)
	{
		if(strlen(m_esch.m_lescape)> 0)
			query.AddSprintf("lines terminated by '%s' ", m_esch.m_lescape);
		else
			query.Add("lines terminated by '' ");
	}

	// ignore lines.
	if(SendMessage(GetDlgItem(m_hwnd, IDC_IGNORELINES), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		// first get the text.
		SendMessage(GetDlgItem(m_hwnd, IDC_IGNORENUM), WM_GETTEXT, SIZE_64-1,(LPARAM)ignore);

		ignorestr.SetAs(ignore);
		query.AddSprintf("ignore %s lines ", ignorestr.GetString());
	}

	// add the columns bu first add the(
	query.Add("(" );

	for(i = 0; i < fieldcount; i++)
	{
		if(SendMessage(m_hwndfieldlist, LB_GETSEL, i, 0))
		{
			SendMessage(m_hwndfieldlist, LB_GETTEXT, i,(LPARAM)fieldname);
			fieldnamestr.SetAs(fieldname);
			query.AddSprintf("`%s`, ", fieldnamestr.GetString());
		}
	}
	
    //strip last space and comma(,), so total 2 chars
	query.Strip(2);
	query.Add(")");

	return query.GetLength();
}
///Create queryy for Load XML inifile
wyInt32 
ExportMultiFormat::PrepareQueryXML(wyString &query)
{
	wyWChar	    fieldname[SIZE_512] = {0}, ignore[SIZE_64+1] = {0},identified_by[SIZE_512+1]={0};
    	wyUInt32    selcount=0, fieldcount=0,i=0;
	wyString	filenamestr, fieldnamestr, ignorestr,identified_bystr;
	wyBool		ismysql538 = wyFalse;

	selcount	= SendMessage(m_hwndfieldlist, LB_GETSELCOUNT, 0, 0);
	fieldcount	= SendMessage(m_hwndfieldlist, LB_GETCOUNT, 0, 0);

	GetFileName(filenamestr);
	
	query.Sprintf("load XML ");
	
	// get low_priority or concurrent
	if(SendMessage(GetDlgItem(m_hwnd, IDC_LOWPRIORITY), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("low_priority ");
	else if(SendMessage(GetDlgItem(m_hwnd, IDC_CONCURRENT), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("concurrent ");

	// add the file name
	query.AddSprintf("local infile '%s' ", filenamestr.GetString());

	// get replace ignore
	if(SendMessage(GetDlgItem(m_hwnd, IDC_REPLACE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("replace ");
	else if(SendMessage(GetDlgItem(m_hwnd, IDC_IGNORE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		query.Add("ignore ");
	
	// add the table name
	query.AddSprintf("into table `%s`.`%s` ", m_db.GetString(), m_table.GetString());

	///Add 'Charcter set' if importing to mysql version >= 5.0.38
	if( m_importfilecharset.GetLength() != 0 && m_importfilecharset.CompareI(STR_DEFAULT) != 0)
          query.AddSprintf("character set '%s' ", m_importfilecharset.GetString());
 
	//rows identified by
	if(SendMessage(GetDlgItem(m_hwnd, IDC_ROWSIDBY), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
	// first get the text.
		SendMessage(GetDlgItem(m_hwnd, IDC_ROWSIDTEXT), WM_GETTEXT, SIZE_64-1,(LPARAM)identified_by);
		identified_bystr.SetAs(identified_by);
		query.AddSprintf("ROWS IDENTIFIED BY '<%s>' ", identified_bystr.GetString());
	}
	// ignore lines.
	if(SendMessage(GetDlgItem(m_hwnd, IDC_IGNORELINES), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		// first get the text.
		SendMessage(GetDlgItem(m_hwnd, IDC_IGNORENUM), WM_GETTEXT, SIZE_64-1,(LPARAM)ignore);

		ignorestr.SetAs(ignore);
		query.AddSprintf("ignore %s lines ", ignorestr.GetString());
	}

	// add the columns bu first add the(
	query.Add("(" );

	for(i = 0; i < fieldcount; i++)
	{
		if(SendMessage(m_hwndfieldlist, LB_GETSEL, i, 0))
		{
			SendMessage(m_hwndfieldlist, LB_GETTEXT, i,(LPARAM)fieldname);
			fieldnamestr.SetAs(fieldname);
			query.AddSprintf("`%s`, ", fieldnamestr.GetString());
		}
	}
	
    //strip last space and comma(,), so total 2 chars
	query.Strip(2);
	query.Add(")");

	return query.GetLength();
}

wyBool 
ExportMultiFormat::GetFileName(wyString &filename)
{
    wyInt32     pathlen = 0;
    wyWChar     *fname = NULL;
    wyChar      *tempfilename = NULL;

    pathlen = SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, (LPARAM)0);
    fname = AllocateBuffWChar(pathlen + 1);
    
    //twice the amount reqd to accomodate double backslash
    tempfilename = AllocateBuff((pathlen + 1) * 2);

	SendMessage(m_hwndedit, WM_GETTEXT, MAX_PATH, (LPARAM)fname);
    filename.SetAs(fname);

    m_tunnel->mysql_real_escape_string(*m_mysql, tempfilename, filename.GetString(), filename.GetLength());
    filename.SetAs(tempfilename);
    
    free(tempfilename);
    free(fname);
	return wyTrue;
}

/*
	Implementaton of Restore Database option
												*/

CRestore::CRestore()
{
	m_p = new Persist;
	m_p->Create("Restore ");
}

CRestore::~CRestore()
{
}

wyBool 
CRestore::Create(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql, wyChar *db)
{
	wyInt64         ret;
	m_hwndparent  =	hwndparent;
	m_mysql       = mysql;
	m_tunnel      = tunnel;

	if(db)
		m_db.SetAs(db);

	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_RESTORE),
		m_hwndparent, CRestore::DlgProc,(LPARAM)this);

    if(ret)
        return wyTrue;

	return wyFalse;
}

INT_PTR CALLBACK
CRestore::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CRestore * pback = (CRestore*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
		pback->OnWmInitDlgValues(hwnd);
		break;

	case WM_DESTROY:
		delete pback->m_p;
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/117-excute-sql-script");
		return 1;

	case WM_COMMAND:
		pback->OnWmCommand(hwnd, wParam);
		break;
	}
	
	return 0;
}

void 
CRestore::OnWmInitDlgValues(HWND hwnd)
{
	m_hwnd = hwnd;

	m_p->Add(m_hwnd, IDC_EXPORTDIRNAME, "DirName", "", TEXTBOX);

	VERIFY(m_hwndtablelist = GetDlgItem(m_hwnd, IDC_TABLELIST));
	VERIFY(m_hwnddirname = GetDlgItem(m_hwnd, IDC_EXPORTDIRNAME));

	FillListBox();

    return;
}

wyBool 
CRestore::OnWmCommand(HWND hwnd, WPARAM wparam)
{
    wyBool ret;

    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		m_p->Cancel();
		VERIFY(yog_enddialog(hwnd, 0));
		break;

	case IDC_EXPFILESELECT:
		ret = GetDirectoryName();
		if(ret)
			SendMessage(m_hwnddirname, WM_SETTEXT, 0,(LPARAM)m_dir.GetAsWideChar());
		break;

	case IDOK:
		if(SendMessage(m_hwnddirname, WM_GETTEXTLENGTH, 0, 0)== 0)
		{
			SetFocus(m_hwnddirname);
			yog_message(hwnd, _(L"Please specify a folder"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		if(SendMessage(m_hwndtablelist, LB_GETSELCOUNT, 0, 0)== 0)
		{
			SetFocus(m_hwndtablelist);
			yog_message(hwnd, _(L"Select at least one table"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		ret = RestoreTable();
		if(ret == wyTrue)
			VERIFY(yog_enddialog(hwnd, 1));
		break;	

	case IDC_SELALL2:
		SendMessage(GetDlgItem(hwnd, IDC_TABLELIST), LB_SETSEL, TRUE, -1);
		break;

	case IDC_UNSELALL2:
		SendMessage(GetDlgItem(hwnd, IDC_TABLELIST), LB_SETSEL, FALSE, -1);
		break;
	}
    return wyTrue;
}


wyBool
CRestore::GetDirectoryName()
{
	LPMALLOC		lpMalloc;
	BROWSEINFO		brwinf;
	LPITEMIDLIST	lpidl;
    wyWChar          dir[MAX_PATH+1]={0};

	memset(&brwinf, 0, sizeof(brwinf));

	if(SHGetMalloc(&lpMalloc)!= NOERROR)
		return wyFalse;

	brwinf.hwndOwner		= m_hwnd;
	brwinf.pidlRoot			= NULL;
	brwinf.pszDisplayName	= dir;
	brwinf.lpszTitle		= _(L"Select a folder where your database is backed up");
	brwinf.ulFlags			= BIF_EDITBOX | BIF_VALIDATE;

	lpidl = SHBrowseForFolder(&brwinf);

	if(!lpidl	)
	{
		lpMalloc->Release();
		return wyFalse;
	}

	VERIFY(SHGetPathFromIDList(lpidl, dir));

    m_dir.SetAs(dir);

	FindAndAddFiles();
	lpMalloc->Free(lpidl);
	lpMalloc->Release();

	return wyTrue;
}


wyBool 
CRestore::FillListBox()
{
	wyString    query;
	wyString	myrow1str;
	wyString	myrow0str;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 = (GetActiveWin())->m_ismysql41;

	query.Sprintf("show table status from `%s`", m_db.GetString());

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
		goto cleanup;

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		if(myrow[1])
		{
			myrow1str.SetAs(myrow[1], ismysql41);
			if(myrow[1] && stricmp(myrow1str.GetString(), "MyISAM")== 0)
			{
				if(myrow[0])
				{
					myrow0str.SetAs(myrow[0], ismysql41);
					SendMessage(m_hwndtablelist, LB_ADDSTRING, 0,(LPARAM)myrow0str.GetAsWideChar());
				}
			}
		}
	}			

	VERIFY((SendMessage(m_hwndtablelist, LB_SETSEL, TRUE, -1)!= LB_ERR));
	m_tunnel->mysql_free_result(myres);

	return wyTrue;

cleanup:
	ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
	EnableWindow(GetDlgItem(m_hwnd, IDC_EXPFILESELECT), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_TABLELIST), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_TABLELIST), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
	return wyFalse;
}



wyBool 
CRestore::RestoreTable()
{
	wyInt32		i;
	wyString    query, usequery;
	CShowInfo	csi;
	MYSQL_RES	*myres;
    wyWChar     dir[MAX_PATH+1] = {0};

	SendMessage(m_hwnddirname, WM_GETTEXT, MAX_PATH,(LPARAM)dir);

	// first change the directory.
	for(i=0; dir[i] != NULL; i++)
	{
		if(dir[i] == C_BACK_SLASH)
			dir[i] = C_FRONT_SLASH;
	}

    m_dir.SetAs(dir);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// first change the database pointer to the current database.
	usequery.Sprintf("use `%s`", m_db.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, usequery);
	if(!myres &&(*m_mysql)->affected_rows == -1)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, usequery.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	m_tunnel->mysql_free_result(myres);
	m_tunnel->SetDB(m_db.GetString());

	VERIFY(PrepareQuery(query));

	myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	csi.ShowInfo(m_hwnd, m_tunnel, myres, _(" Restore Table Information"));
	m_tunnel->mysql_free_result(myres);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	pGlobals->m_pcmainwin->ChangeDBInCombo(m_tunnel, m_mysql);
	
	return wyTrue;
}


wyInt32 
CRestore::PrepareQuery(wyString &query)
{
	wyWChar     table[SIZE_512]={0};
	wyUInt32    selcount, i, tcount;
	wyString	tablestr;

	selcount = SendMessage(m_hwndtablelist, LB_GETSELCOUNT, 0, 0);
	tcount	 = SendMessage(m_hwndtablelist, LB_GETCOUNT, 0, 0);

	query.Sprintf("restore table ");

	// allocate some buffer,
	for(i=0; i<tcount; i++)
	{
		if(SendMessage(m_hwndtablelist, LB_GETSEL, i, 0 ))
		{
			SendMessage(m_hwndtablelist, LB_GETTEXT, i,(LPARAM)table);
			tablestr.SetAs(table);
			query.AddSprintf("`%s`, ", tablestr.GetString());
		}
	}

    // strip the last comma and space
	query.Strip(2);
	query.AddSprintf(" from '%s'", m_dir.GetString());
	
	return query.GetLength();
}


wyBool 
CRestore::FindAndAddFiles()
{
	wyInt32             i = 0;
	wyWChar              tablename[MAX_PATH + 10], find[MAX_PATH + 10];
	BOOL				ret;
	HANDLE				hfile;
	WIN32_FIND_DATAW	wfd;

	ZeroMemory((PVOID)&wfd, sizeof(WIN32_FIND_DATA));

	swprintf(find, L"%s\\*.MYD", m_dir.GetAsWideChar());

	SendMessage(m_hwndtablelist, LB_RESETCONTENT, 0, 0);

	hfile = FindFirstFile(find, &wfd); 

	if(hfile == INVALID_HANDLE_VALUE)
	{
		yog_message(m_hwnd, _(L"Could not locate any .MYD files in the folder!"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}
	else
	{
		do
		{
			for(i = 0; wfd.cFileName[i] != '.'; i++)
				tablename[i] = wfd.cFileName[i];
			
			tablename[i] = 0;

			VERIFY((SendMessage(m_hwndtablelist, LB_ADDSTRING, 0,(LPARAM)tablename)!= LB_ERR));
			ret = FindNextFile(hfile, &wfd); // When there is no more file.
		}
		while(ret != 0);
	}

	SendMessage(m_hwndtablelist, LB_SETSEL, TRUE, -1);
	VERIFY(FindClose(hfile));
	return wyTrue;
}
