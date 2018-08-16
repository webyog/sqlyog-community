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
#include "CommonHelper.h"
#include "FrameWindow.h"
#include "ExportMultiFormat.h"
#include "FileHelper.h"
#include "SQLMaker.h"
#include "GUIHelper.h"
#include "ExportToExcel.h"
#include "ExportAsSimpleSQL.h"

#define	 SHOWWARNING	WM_USER + 1
#define  SIZE_8K		(8*1024)


extern	PGLOBALS		 pGlobals;

#define SQLWARNING  _(L"This saves the data in most simple form of SQL statement.\n\
Additional table information (indexes, charset, comments etc.) is not saved.\n\n\
If you want to take a backup of your data please use 'Backup table as SQL Dump' instead ")

/*
	Implementation of export as CSV Dialog.
	This in itself calls the character dialog box and user can export the data in the 
	specified format.
																					*/

ExportCsv::ExportCsv()
{
}

ExportCsv::~ExportCsv()
{
}

wyBool 
ExportCsv::GetEscapeCharacters(HWND hwnd)
{
	EscapeChar	cech;

	cech.GetEscapeChar(hwnd, &m_esch, wyFalse, wyFalse);

	return wyTrue;
}


wyBool 
ExportCsv::GetEscapeCharacters()
{
	EscapeChar	cech;

	cech.GetEscapeChar(m_hwnd, &m_esch, wyFalse, wyFalse);

	return wyTrue;
}

wyBool 
ExportCsv::WriteFixed(wyString * buffer, wyChar *text, MYSQL_FIELD * field,wyUInt32 length, 
					  wyBool isescaped, wyChar fterm, wyBool isfterm, wyChar lterm, 
					  wyBool islterm, wyChar encl, wyBool isencl)
{
	wyChar	*pad, *temp;
	wyUInt32 newsize = 0, size = 0, sizelen;
	wyString tempbuff;

	if(field->type >= FIELD_TYPE_TINY_BLOB && field->type <= FIELD_TYPE_BLOB)
		return wyTrue;

	if(!text)
    {
		VERIFY(pad =(wyChar *)calloc(sizeof(wyChar), field->length));
		memset(pad, C_SPACE, field->length);
	
		//adding to a temp buffer 
		tempbuff.SetAs(pad);
		
		if(field->length <= tempbuff.GetLength())
			tempbuff.Strip(tempbuff.GetLength() - field->length);

		free(pad);
	} 
    else 
    {
		temp = MySqlEscape(text, length, &newsize, m_esch.m_escape[0], isfterm, 
						   fterm, isencl, encl, islterm, lterm, wyTrue, wyTrue);

        sizelen = field->length == 0 ? field->max_length : field->length;
		size = sizelen + (newsize-length);

		pad =(wyChar *)calloc(sizeof(wyChar), (size + 1));

		memset(pad, C_SPACE, (size + 1));

		memcpy(pad, temp, newsize);
        
		//adding to a temp buffer 
		
		tempbuff.SetAs(pad);

		if(size <= tempbuff.GetLength())
			tempbuff.Strip(tempbuff.GetLength() - size);

		free(pad);
		free(temp);
	}

	//Add in buffer
	buffer->Add(tempbuff.GetString());
			
	return wyTrue;
}


wyBool 	
ExportCsv::WriteVarible(wyString * buffer, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, 
						wyBool isescaped, wyChar fterm, wyBool isfterm, wyChar lterm, 
						wyBool islterm, wyChar encl, wyBool isencl)
{
	wyChar	 *temp;
	wyUInt32 newsize=0;

	//Add in buffer
	if((!text) || strncmp(text,"(NULL)", 7)==0 || strncmp(text,"NULL", 5)==0 )
        {
		buffer->Add(m_esch.m_nullchar);
	}
	else
	{
		if(isencl)
        {
			if(m_esch.m_isoptionally == wyFalse ||(field->type == FIELD_TYPE_VAR_STRING 
				|| field->type == FIELD_TYPE_STRING ||  field->type == FIELD_TYPE_ENUM 
				|| field->type == FIELD_TYPE_SET))
				
				buffer->AddSprintf("%c", encl);
				
		}

		temp = MySqlEscape(text, length,&newsize, m_esch.m_escape[0], isfterm, fterm, isencl, 
							encl, islterm, lterm, m_esch.m_isescaped, wyTrue);
				
		//Add in buffer
		buffer->Add(temp);
		
		free(temp);

		if(isencl)
		{
			if(m_esch.m_isoptionally == wyFalse ||(field->type == FIELD_TYPE_VAR_STRING 
				|| field->type == FIELD_TYPE_STRING ||  field->type == FIELD_TYPE_ENUM 
				|| field->type == FIELD_TYPE_SET))
			
				buffer->AddSprintf("%c", encl);
		}
	}
	
	return wyTrue;
}


wyBool
ExportCsv::GetDefEscChar(wyChar *fterm, wyBool *isfterm, wyChar *lterm, wyBool *islterm, wyChar *encl, wyBool *isencl)
{
	*fterm		= NULL;
	*isfterm	= wyFalse;
	*lterm		= NULL;
	*islterm	= wyFalse;
	*encl		= NULL;
	*isencl		= wyFalse;

	// get the escape character for fields.
	if(m_esch.m_isfieldsterm == wyTrue)
    {
		if(strlen(m_esch.m_fescape) != 0)
        {
			*fterm	 = m_esch.m_fescape[0];
			*isfterm = wyTrue;
		}
	} else 
    {
		*fterm	 = C_TAB;
		*isfterm = wyTrue;
	}

	// get the escape character for line termination.
	if(m_esch.m_islineterminated == wyTrue)
    {
		if(strlen(m_esch.m_lescape) != 0)
        {
			*lterm	 = m_esch.m_lescape[0];
			*islterm = wyTrue;
		}
	} else 
    {
		*lterm = C_NEWLINE;
		*islterm = wyTrue; 
	}

	// get the enclosing escape character
	if(m_esch.m_isenclosed == wyTrue)
    {
		if(strlen(m_esch.m_enclosed) != 0)
        {
			*encl	 = m_esch.m_enclosed[0];
			*isencl = wyTrue;
		}
	}
	
	return wyTrue;
}


wyBool 
ExportCsv::ProcessEscChar(wyChar *buffer)
{
 	wyInt32 buffcounter, counter=0;

	for(buffcounter = 0; buffer[buffcounter] != NULL; buffcounter++)
    {
		if(buffer[buffcounter] == C_BACK_SLASH && buffer[buffcounter + 1] != NULL)	{
			switch(buffer[buffcounter + 1])
			{
			case 'r':
				buffer[buffcounter] = C_CARRIAGE;
				memmove(buffer + buffcounter + 1, buffer + buffcounter + 2, strlen(buffer + buffcounter + 2));
				counter++;
				break;

			case 'n':
				buffer[buffcounter] = C_NEWLINE;
				memmove(buffer + buffcounter + 1, buffer + buffcounter + 2, strlen(buffer + buffcounter + 2));
				counter++;
				break;

			case 't':
				buffer[buffcounter] = C_TAB;
				memmove(buffer + buffcounter + 1, buffer + buffcounter + 2, strlen(buffer + buffcounter + 2));
				counter++;
				break;

			case C_BACK_SLASH:
				buffer[buffcounter] = C_BACK_SLASH;
				memmove(buffer + buffcounter + 1, buffer + buffcounter + 2, strlen(buffer + buffcounter + 2));
				counter++;
				break;				
			}
		}
	}

	if(buffcounter)
		buffer[buffcounter - counter] = NULL;

	return wyTrue;
}

/*
	Implementation of escaping characters dialog box.
													*/
EscapeChar::EscapeChar()
{
}

EscapeChar::~EscapeChar()
{
}

wyBool 
EscapeChar::GetEscapeChar(HWND hwndparent, PESCAPECHAR pesc, wyBool type, wyBool choice)
{
	wyInt64 ret;

	m_hwndparent    = hwndparent;
	m_pesc          = pesc;
	m_type          = type;
	m_fieldnames    = choice;

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_ESCAPECHAR),
			hwndparent,EscapeChar::DlgProc,(LPARAM)this);

    if(ret)
        return wyTrue;
	return wyFalse;

}

INT_PTR CALLBACK
EscapeChar::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	EscapeChar * pesch =(EscapeChar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
		pesch->OnWmInitDlgvalues(hwnd);
		break;

	case WM_COMMAND:
		pesch->OnWmCommand(hwnd, wParam, lParam);		
	}

	return 0;
}

void 
EscapeChar::OnWmInitDlgvalues(HWND hwnd)
{
	m_hwnd = hwnd;

	SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), EM_LIMITTEXT, 2,(LPARAM)0);
	SendMessage(GetDlgItem(m_hwnd, IDC_LESCAPE), EM_LIMITTEXT, 10,(LPARAM)0);
	SendMessage(GetDlgItem(m_hwnd, IDC_FESCAPE), EM_LIMITTEXT, 10,(LPARAM)0);
	SendMessage(GetDlgItem(m_hwnd, IDC_FENCLOSED), EM_LIMITTEXT, 2,(LPARAM)0);
	SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHAR), EM_LIMITTEXT, 8,(LPARAM)0);

	SetEscapeChar();

	if(m_fieldnames)
		ShowWindow(GetDlgItem(m_hwnd, IDC_COLUMNS), TRUE);

    return;
}

void 
EscapeChar::OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;

	case IDC_NULLREPLACEBY:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
		{
			SendMessage(GetDlgItem(hwnd, IDC_NULLCHAR), WM_SETTEXT, 0, (LPARAM)L"");
			EnableWindow(GetDlgItem(hwnd, IDC_NULLCHAR), FALSE);
		}
		else
			EnableWindow(GetDlgItem(hwnd, IDC_NULLCHAR), TRUE);
		break;

	case IDC_ESCAPED:
		if(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
		{
			SendMessage(GetDlgItem(hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)L"");
			EnableWindow(GetDlgItem(hwnd, IDC_ESCAPECHAR), FALSE);
		}
		else
			EnableWindow(GetDlgItem(hwnd, IDC_ESCAPECHAR), TRUE);
		break;

	case IDC_LINESTERM:
		if(!(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0)))
		{
			SendMessage(GetDlgItem(hwnd, IDC_LESCAPE), WM_SETTEXT, 0,(LPARAM)L"");
			EnableWindow(GetDlgItem(hwnd, IDC_LESCAPE), FALSE);
		}
		else
			EnableWindow(GetDlgItem(hwnd, IDC_LESCAPE), TRUE);
		break;			

	case IDC_FIXEDLENGTH:
		{
			SendMessage(GetDlgItem(hwnd, IDC_FIELDSTERMINATED), BM_SETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_FIELDSTERMINATED), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_FESCAPE), FALSE);
			SendMessage(GetDlgItem(hwnd, IDC_FESCAPE), WM_SETTEXT, 0,(LPARAM)L"");
			SendMessage(GetDlgItem(hwnd, IDC_FIELDSENCLOSED), BM_SETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_FIELDSENCLOSED), FALSE);
			SendMessage(GetDlgItem(hwnd, IDC_FENCLOSED), WM_SETTEXT, 0,(LPARAM)L"");
			SendMessage(GetDlgItem(hwnd, IDC_OPTIONAL), BM_SETCHECK, BST_UNCHECKED, 0);

            SendMessage(GetDlgItem(hwnd, IDC_NULLREPLACEBY), BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_NULLREPLACEBY), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NULLCHAR), FALSE);
            SendMessage(GetDlgItem(hwnd, IDC_NULLCHAR), WM_SETTEXT, 0,(LPARAM)L"");
			SendMessage(GetDlgItem(hwnd, IDC_OPTIONAL), BM_SETCHECK, BST_UNCHECKED, 0);

            EnableOptions(hwnd, 0);
		}
		break;

	case IDC_VARIABLE:
		{
			EnableWindow(GetDlgItem(hwnd, IDC_FIELDSTERMINATED), TRUE);
			if(SendMessage(GetDlgItem(hwnd, IDC_FIELDSTERMINATED), BM_GETCHECK, 0, 0))
				EnableWindow(GetDlgItem(hwnd, IDC_FESCAPE), TRUE);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_FESCAPE), FALSE);

			EnableWindow(GetDlgItem(hwnd, IDC_FIELDSENCLOSED), TRUE);
			if(SendMessage(GetDlgItem(hwnd, IDC_FIELDSENCLOSED), BM_GETCHECK, 0, 0))
			    EnableOptions(hwnd, 1);
			else
			    EnableOptions(hwnd, 0);

            EnableWindow(GetDlgItem(hwnd, IDC_NULLREPLACEBY), TRUE);
            if(SendMessage(GetDlgItem(hwnd, IDC_NULLREPLACEBY), BM_GETCHECK, 0, 0))
			    EnableOptions(hwnd, 1);
			else
			    EnableOptions(hwnd, 0);

		}
		break;

	case IDC_FIELDSTERMINATED:
		{
			if(!(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0)))
			{
				SendMessage(GetDlgItem(hwnd, IDC_FESCAPE), WM_SETTEXT, 0,(LPARAM)L"");
				EnableWindow(GetDlgItem(hwnd, IDC_FESCAPE), FALSE);
			}
			else
				EnableWindow(GetDlgItem(hwnd, IDC_FESCAPE), TRUE);
		}
		break;

	case IDC_FIELDSENCLOSED:
		{
			if(!(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0)))
			{
				SendMessage(GetDlgItem(hwnd, IDC_FENCLOSED), WM_SETTEXT, 0,(LPARAM)L"");
                EnableOptions(hwnd, 0);
			}
			else
			{
				EnableOptions(hwnd, 1);
			}
		}
		break;

	case IDC_COLUMNS:

		break;

	case IDOK:
		ProcessOK();
		yog_enddialog(hwnd, 1);
		break;

	case IDC_EXCELDEFAULT:
		SetExcelEscapeChar(m_type);
		break;
	}
}

void 
EscapeChar::EnableOptions(HWND hwnd, wyInt32 enable)
{
    EnableWindow(GetDlgItem(hwnd, IDC_FENCLOSED), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_OPTIONAL), enable);
}

wyBool 
EscapeChar::ProcessOK()
{
	wyWChar		escape[10] = {0}, lescape[10] = {0}, fescape[10] = {0}, enclosed[10] = {0}, nullchar[10] = {0};
	wyString	escapestr, lescapestr, fescapestr, enclosedstr, nullcharstr;

	// get the escaping information.
	if(SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPED), BM_GETCHECK, 0, 0))
		m_pesc->m_isescaped = wyTrue;
	else
		m_pesc->m_isescaped = wyFalse;
	SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_GETTEXT, 3,(LPARAM)escape);
	escapestr.SetAs(escape);

	//If the escape character text box is empty consider the escaped by is invalid
	if(!escapestr.GetLength() && m_pesc->m_isescaped == wyTrue)
		m_pesc->m_isescaped = wyFalse;

	strcpy(m_pesc->m_escape, escapestr.GetString());

	// get the lines terminated message.
	if(SendMessage(GetDlgItem(m_hwnd, IDC_LINESTERM), BM_GETCHECK, 0, 0))
		m_pesc->m_islineterminated = wyTrue;
	else
		m_pesc->m_islineterminated = wyFalse;
	SendMessage(GetDlgItem(m_hwnd, IDC_LESCAPE), WM_GETTEXT, 10,(LPARAM)lescape);
	lescapestr.SetAs(lescape);
	strcpy(m_pesc->m_lescape, lescapestr.GetString());
    
	// get the NULL character information.
	if(SendMessage(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), BM_GETCHECK, 0, 0))
		m_pesc->m_isnullreplaceby = wyTrue;
	else
		m_pesc->m_isnullreplaceby = wyFalse;
	SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHAR), WM_GETTEXT, 10,(LPARAM)nullchar);
	nullcharstr.SetAs(nullchar);
	strcpy(m_pesc->m_nullchar, nullcharstr.GetString());

	//If the NULL character text box is empty consider the escaped by is invalid
	if(!nullcharstr.GetLength() && m_pesc->m_isnullreplaceby == wyTrue)
		m_pesc->m_isnullreplaceby = wyFalse;
	
	// get the fixed length information
	if(SendMessage(GetDlgItem(m_hwnd, IDC_FIXEDLENGTH), BM_GETCHECK, 0, 0))
		m_pesc->m_isfixed = wyTrue;
	else
		m_pesc->m_isfixed = wyFalse;
	if(SendMessage(GetDlgItem(m_hwnd, IDC_COLUMNS), BM_GETCHECK, 0, 0))
		m_pesc->m_isincludefieldnames = wyTrue;
	else
		m_pesc->m_isincludefieldnames = wyFalse;

	if(m_pesc->m_isfixed == wyTrue)
	{
		m_pesc->m_isfieldsterm = wyFalse;
		SendMessage(GetDlgItem(m_hwnd, IDC_FESCAPE), WM_SETTEXT, 0,(LPARAM)L"");
		m_pesc->m_isenclosed = wyFalse;
		m_pesc->m_isoptionally = wyFalse;
		SendMessage(GetDlgItem(m_hwnd, IDC_FENCLOSED), WM_SETTEXT, 0,(LPARAM)L"");
	}
	else
	{
		// gets the fields termination data.,
		if(SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), BM_GETCHECK, 0, 0) )
			m_pesc->m_isfieldsterm = wyTrue;
		else
			m_pesc->m_isfieldsterm = wyFalse;

		SendMessage(GetDlgItem(m_hwnd, IDC_FESCAPE), WM_GETTEXT, 10,(LPARAM)fescape);
		fescapestr.SetAs(fescape);
		strcpy(m_pesc->m_fescape, fescapestr.GetString());
		
		if(SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), BM_GETCHECK, 0, 0))
			m_pesc->m_isenclosed = wyTrue;
		else
			m_pesc->m_isenclosed = wyFalse;

		SendMessage(GetDlgItem(m_hwnd, IDC_FENCLOSED), WM_GETTEXT, 3,(LPARAM)enclosed);
		enclosedstr.SetAs(enclosed);
		strcpy(m_pesc->m_enclosed, enclosedstr.GetString());

		if(SendMessage(GetDlgItem(m_hwnd, IDC_OPTIONAL), BM_GETCHECK, 0, 0))
			m_pesc->m_isoptionally = wyTrue;
		else
			m_pesc->m_isoptionally = wyFalse;
	}

	//Get include column names message
	if(SendMessage(GetDlgItem(m_hwnd, IDC_COLUMNS), BM_GETCHECK, 0, 0))
		m_pesc->m_iscolumns = wyTrue;
	else
		m_pesc->m_iscolumns = wyFalse;


	return wyTrue;
}

wyBool 
EscapeChar::SetEscapeChar()
{
	wyString	escape, lescape, fescape, enclosed, nullchar;

	if(m_pesc->m_isescaped == wyTrue)
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPED), BM_SETCHECK, BST_CHECKED, 0);
		escape.SetAs(m_pesc->m_escape);
		SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)escape.GetAsWideChar());
	}
	else
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPED), BM_SETCHECK, BST_UNCHECKED, 0);
	    EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), FALSE);
	}
	
	if(m_pesc->m_islineterminated == wyTrue)
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_LINESTERM), BM_SETCHECK, BST_CHECKED, 0);
		lescape.SetAs(m_pesc->m_lescape);
		SendMessage(GetDlgItem(m_hwnd, IDC_LESCAPE), WM_SETTEXT, 0,(LPARAM)lescape.GetAsWideChar());
	}
	else
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_LINESTERM), BM_SETCHECK, BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(m_hwnd, IDC_LESCAPE), FALSE);
	}

	if(m_pesc->m_isfixed == wyTrue)
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_FIXEDLENGTH), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(m_hwnd, IDC_VARIABLE), BM_SETCHECK, BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_FESCAPE), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), FALSE);

        SendMessage(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_NULLCHAR), FALSE);
		EnableOptions(m_hwnd, 0);
	}
	else
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_VARIABLE), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(m_hwnd, IDC_FIXEDLENGTH), BM_SETCHECK, BST_UNCHECKED, 0);
		
		if(m_pesc->m_isfieldsterm == wyTrue)
		{
            SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), BM_SETCHECK, BST_CHECKED, 0);
			fescape.SetAs(m_pesc->m_fescape);
			SendMessage(GetDlgItem(m_hwnd, IDC_FESCAPE), WM_SETTEXT, 0,(LPARAM)fescape.GetAsWideChar());
		}
		else
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(m_hwnd, IDC_FESCAPE), FALSE);
		}
		
		if(m_pesc->m_isenclosed == wyTrue)
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), BM_SETCHECK, BST_CHECKED, 0);
			enclosed.SetAs(m_pesc->m_enclosed);
            //enclosed.SetAs("\"");
			SendMessage(GetDlgItem(m_hwnd, IDC_FENCLOSED), WM_SETTEXT, 0,(LPARAM)enclosed.GetAsWideChar());
			if(m_pesc->m_isoptionally)
				SendMessage(GetDlgItem(m_hwnd, IDC_OPTIONAL), BM_SETCHECK, BST_CHECKED, 0);
		}
		else
		{
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableOptions(m_hwnd, 0);
		}
        if(m_pesc->m_isnullreplaceby == wyTrue)
	    {
		    SendMessage(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), BM_SETCHECK, BST_CHECKED, 0);
		    nullchar.SetAs(m_pesc->m_nullchar);
		    SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHAR), WM_SETTEXT, 0,(LPARAM)nullchar.GetAsWideChar());
	    }
	    else
	    {
		    SendMessage(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), BM_SETCHECK, BST_UNCHECKED, 0);
		    EnableWindow(GetDlgItem(m_hwnd, IDC_NULLCHAR), FALSE);
	    }
	}

	//to set the state of include column names ,enabled or disabled
	if(m_pesc->m_iscolumns == wyTrue)
		SendMessage(GetDlgItem(m_hwnd, IDC_COLUMNS), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_COLUMNS), BM_SETCHECK, BST_UNCHECKED, 0);

	return wyTrue;
}

void 
EscapeChar::SetExcelEscapeChar(wyBool clipboard)
{
    wyString	escape, lescape, fescape, enclosed;

	EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FESCAPE), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), TRUE);
	EnableOptions(m_hwnd, 1);
	
	SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPED), BM_SETCHECK, BST_CHECKED, 0);
	EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), TRUE);
    escape.SetAs("\\\\");
	SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)escape.GetAsWideChar());

	SendMessage(GetDlgItem(m_hwnd, IDC_LINESTERM), BM_SETCHECK, BST_CHECKED, 0);
	EnableWindow(GetDlgItem(m_hwnd, IDC_LESCAPE), TRUE);
    lescape.SetAs("\\r\\n");
	SendMessage(GetDlgItem(m_hwnd, IDC_LESCAPE), WM_SETTEXT, 0,(LPARAM)lescape.GetAsWideChar());

	//..Enable "NULL replace by" options
    EnableWindow(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), TRUE);
    SendMessage(GetDlgItem(m_hwnd, IDC_NULLREPLACEBY), BM_SETCHECK, BST_CHECKED, 0);
	EnableWindow(GetDlgItem(m_hwnd, IDC_NULLCHAR), TRUE);
	SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHAR), WM_SETTEXT, 0, (LPARAM) L"\\N");

	SendMessage(GetDlgItem(m_hwnd, IDC_VARIABLE), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(m_hwnd, IDC_FIXEDLENGTH), BM_SETCHECK, BST_UNCHECKED, 0);

    SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSTERMINATED), BM_SETCHECK, BST_CHECKED, 0);
            
    if(clipboard)
        fescape.SetAs("\\t");
    else
        fescape.SetAs(",");

	SendMessage(GetDlgItem(m_hwnd, IDC_FESCAPE), WM_SETTEXT, 0,(LPARAM)fescape.GetAsWideChar());

	SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSENCLOSED), BM_SETCHECK, BST_CHECKED, 0);

    enclosed.SetAs("\"");
	SendMessage(GetDlgItem(m_hwnd, IDC_FENCLOSED), WM_SETTEXT, 0,(LPARAM)enclosed.GetAsWideChar());
}

/*
	Implementation of Backup Table Dialog Box.
												*/

BackUp::BackUp()
{
	m_p = new Persist;
}

BackUp::~BackUp()
{
}

wyBool
BackUp::Create(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql, wyChar *db)
{
	wyInt64 ret;
	
	m_hwndparent = hwndparent;
	m_mysql      = mysql;
	m_tunnel     = tunnel;

	if(db)
		m_db.SetAs(db);

	//Post 8.01
    //RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_BACKUP),
		hwndparent, BackUp::DlgProc,(LPARAM)this);

    if(ret)
        return wyTrue;

	return wyFalse;
}

INT_PTR CALLBACK
BackUp::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BackUp * pback =(BackUp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
		pback->OnWmInitDlgvalues(hwnd);
		break;

	case WM_DESTROY:
		delete pback->m_p;
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/116-backup-database-as-sql-dump");
		return TRUE;

	case UM_BACKUPTABLE:
		if(pback->BackUpTable() == wyFalse)
			yog_enddialog(hwnd, 1);
		break;

	case WM_COMMAND:
		pback->OnWmCommand(hwnd, wParam, lParam);
		break;
	}	
	return 0;
}

void 
BackUp::OnWmInitDlgvalues(HWND hwnd)
{
    m_hwnd = hwnd;
	m_p->Create("Backup");
	m_p->Add(m_hwnd, IDC_EXPORTDIRNAME, "DirName", "", TEXTBOX);
	VERIFY(m_hwndtablelist = GetDlgItem(m_hwnd, IDC_TABLELIST));
	VERIFY(m_hwnddirname = GetDlgItem(m_hwnd, IDC_EXPORTDIRNAME));
	FillListBox();
}

wyBool 
BackUp::OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		m_p->Cancel();
		yog_enddialog(hwnd, 0);
		break;

	case IDC_EXPFILESELECT:
		if(GetDirectoryName() == wyTrue)
			SendMessage(m_hwnddirname, WM_SETTEXT, 0,(LPARAM)m_dir.GetAsWideChar());
		break;

	case IDOK:
		if(SendMessage(m_hwnddirname, WM_GETTEXTLENGTH, 0, 0)== 0)
		{
			SetFocus(m_hwnddirname);
			yog_message(hwnd, _(L"Please specify a folder"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		if(SendMessage(m_hwndtablelist, LB_GETSELCOUNT, 0, 0) == 0)
		{
			SetFocus(m_hwndtablelist);
			yog_message(hwnd, _(L"Select at least one table"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		PostMessage(hwnd, UM_BACKUPTABLE, NULL, NULL);
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

// Gets the directory name.
wyBool 
BackUp::GetDirectoryName()
{
    wyWChar dir[MAX_PATH+1]={0};
    wyBool  ret;

	ret = GetDirectoryFromUser(m_hwnd, dir, _(L"Select a folder where you want to backup the database"));

    if(ret == wyTrue)
        m_dir.SetAs(dir);

    return ret;
}

// Function fills the listbox with the tables of the selected database.
wyBool 
BackUp::FillListBox()
{
    wyString    query;
    MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyString	myrow0str, myrow1str;
	wyBool		ismysql41 = IsMySQL41(m_tunnel, m_mysql);

	query.Sprintf("show table status from `%s`", m_db.GetString());

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
        return OnError();
	}

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		if(myrow[1] && myrow[0])
		{
		myrow1str.SetAs(myrow[1], ismysql41);
		myrow0str.SetAs(myrow[0], ismysql41);

		if(myrow[1] && (myrow1str.CompareI("MyISAM"))== 0)
			SendMessage(m_hwndtablelist, LB_ADDSTRING, 0,(LPARAM)myrow0str.GetAsWideChar());
		}
	}			

	m_tunnel->mysql_free_result(myres);
	VERIFY((SendMessage(m_hwndtablelist, LB_SETSEL, TRUE, -1))!= LB_ERR);
	return wyTrue;
}

wyBool 
BackUp::OnError()
{
    //ShowMySQLError(m_hwnd, m_tunnel, m_mysql);
	EnableWindow(GetDlgItem(m_hwnd, IDC_EXPORTDIRNAME), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_EXPFILESELECT), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_TABLELIST), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_TABLELIST), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
	return wyFalse;
}


wyBool 
BackUp::BackUpTable()
{
	wyInt32     i;
	CShowInfo	csi;
    wyString    query;
	MYSQL_RES	*myres;
    wyWChar     dir[MAX_PATH+1];
	MDIWindow	*wnd = GetActiveWin();

	SendMessage(m_hwnddirname, WM_GETTEXT, MAX_PATH,(LPARAM)dir);

	// first change the directory.
	for(i=0; dir[i] != NULL; i++)
    {
		if(dir[i] == C_BACK_SLASH)
			dir[i] = C_FRONT_SLASH;
	}

    m_dir.SetAs(dir);
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	PrepareQuery(query);
	pGlobals->m_pcmainwin->AddTextInStatusBar(_(L"Backing up database..."));
	wnd->m_statusbartext.SetAs(_(L"Backing up database..."));
    myres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);

	if(!myres)
    {
        ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
        UnlockTables();
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return wyFalse;
    }
	
	csi.ShowInfo(m_hwnd, m_tunnel, myres, _(" Backup Table Information"));
	m_tunnel->mysql_free_result(myres);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}


wyInt32 
BackUp::PrepareQuery(wyString &query)
{
	wyWChar     table[SIZE_512] = {0};
	wyUInt32    selcount, i,tcount;
	wyString	tablestr;	

	selcount = SendMessage(m_hwndtablelist, LB_GETSELCOUNT, 0, 0);
	tcount	 = SendMessage(m_hwndtablelist, LB_GETCOUNT, 0, 0);

	query.Sprintf("backup table ");

	// allocate some buffer,
	for(i = 0; i < tcount; i++)
    {
		if(SendMessage(m_hwndtablelist, LB_GETSEL, i, 0 ))
		{
			SendMessage(m_hwndtablelist, LB_GETTEXT, i, (LPARAM)table);

			tablestr.SetAs(table);
			query.AddSprintf("`%s`.`%s`, ", m_db.GetString(), tablestr.GetString());
		}
	}
	
    query.Strip(2);
	query.AddSprintf(" to '%s'", m_dir.GetString());
	
	return query.GetLength();
}


wyBool 
BackUp::LockTables()
{
	wyString    query, tablestr;
	wyWChar     table[SIZE_512] = {0};
	MYSQL_RES	*res;
	wyUInt32    selcount, tcount, i;

	selcount = SendMessage(m_hwndtablelist, LB_GETSELCOUNT, 0, 0);
	tcount	 = SendMessage(m_hwndtablelist, LB_GETCOUNT, 0, 0);

	query.Sprintf("lock tables ");

	// allocate some buffer,
	for(i = 0; i < tcount; i++)
    {
		if(SendMessage(m_hwndtablelist, LB_GETSEL, i, 0 )){
			SendMessage(m_hwndtablelist, LB_GETTEXT, i, (LPARAM)table);
			
			tablestr.SetAs(table);

			query.AddSprintf("`%s`.`%s` read, ", m_db.GetString(), tablestr.GetString());
		}
	}

	query.Strip(2);

    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res &&(*m_mysql)->affected_rows == -1)
    {
        ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
	    return wyFalse;
    }

	m_tunnel->mysql_free_result(res);
	return wyTrue;
}

// unlock tables.
wyBool 
BackUp::UnlockTables()
{
	wyString    query;      
	MYSQL_RES	*res;

	query.Sprintf("unlock tables");

    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res &&(*m_mysql)->affected_rows == -1)
    {
        ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
	    return wyFalse;
    }
	m_tunnel->mysql_free_result(res);

	return wyTrue;
}

/*
	Implementation of the CExportResultSet Dialog Box
	Through this dialog box the user can Export their resultset in a format they want
																						*/
CExportResultSet::CExportResultSet()
{
	m_cesv.m_esch.m_towrite = wyFalse;
    m_exporting = wyFalse;

	//if export as csv then read from EXPORTCSVESCAPING section
	m_cesv.m_esch.m_isclipboard = wyFalse;
	m_cesv.m_esch.ReadFromFile(EXPORTCSVSECTION);

	m_selectedfields    = NULL;
    m_ptr               = NULL;

    m_resultfromquery = wyFalse;
    m_isenablesqlexport = wyTrue;
	
    // Initialize the critical section one time only.
    InitializeCriticalSection(&m_cs);

}

CExportResultSet::~CExportResultSet()
{
    // Release resources used by the critical section object.
    DeleteCriticalSection(&m_cs);

	m_structonly = wyFalse;
	m_dataonly = wyFalse;
	m_structdata =  wyFalse;

	m_includeversion =  wyTrue;
	if(m_selectedfields)
		free(m_selectedfields);
}

wyBool  
CExportResultSet::Export(HWND hwndparent, MySQLDataEx *ptr, wyBool fromquery, wyBool isenablesqlexport)
{
	wyInt64		ret;

    if(ptr == NULL)
        return wyFalse;

    m_isenablesqlexport = isenablesqlexport;
    m_tunnel        = ptr->m_pmdi->m_tunnel;
	m_ptr           = ptr;
    m_res           = ptr->m_datares;
    if(m_res)
        m_field         = ptr->m_datares->fields;
    else
        m_field = NULL;
    m_fromquerytab  = fromquery;

	m_p = new Persist;
	m_p2 = new Persist;
	
	m_p->Create("EXPORTRESULTSET");
	m_p2->Create("EXPORTCSV");

	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_EXPORTRESULTSET), 
                            hwndparent, CExportResultSet::DlgProc,(LPARAM)this );

    if(ret)
        return wyTrue;
    else
	    return wyFalse;
}

// Dialog box procedure for the CExportResultDialogBox
INT_PTR CALLBACK	
CExportResultSet::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	CExportResultSet * cer =(CExportResultSet*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	DlgControl* pdlgctrl;

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
		cer->OnWmInitDlgValues(hwnd);
		break;

	case WM_DESTROY:
		SaveInitPos(hwnd, EXPORTAS_SECTION);
		//delete the dialog control list
		{
			while((pdlgctrl = (DlgControl*)cer->m_controllist.GetFirst()))
			{
				cer->m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
		}
		delete cer->m_p;
		delete cer->m_p2;
		break;

	case WM_SIZE:
		cer->ExpDatResize(hwnd);
		break;

	case WM_GETMINMAXINFO:
		cer->OnWMSizeInfo(lParam);
        return 1;
		break;

	case WM_ERASEBKGND:
		//done for double buffering
		return 1;

	case WM_PAINT:
		cer->OnPaint(hwnd);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/98-export-data");
		return 1;

	case WM_COMMAND:
	    cer->OnWmCommand(hwnd, wParam, lParam);
        break;

	case SHOWWARNING:
		MessageBox(hwnd, SQLWARNING, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		break;
	}

	return 0;
}

void 
CExportResultSet::OnWmInitDlgValues(HWND hwnd)
{
    m_hwnd = hwnd;
	VERIFY(m_hwndedit = GetDlgItem(hwnd, IDC_EXPORTFILENAME));
	VERIFY(m_hwndxml  = GetDlgItem(hwnd, IDC_XML));
	VERIFY(m_hwndhtml = GetDlgItem(hwnd, IDC_HTML));
	VERIFY(m_hwndcsv = GetDlgItem (hwnd, IDC_CSV));
	VERIFY(m_hwndjson=GetDlgItem (hwnd, IDC_JSON));
	VERIFY(m_hwndchange = GetDlgItem(hwnd, IDC_CHANGE));
    
    VERIFY(m_hwndbloblimit = GetDlgItem(hwnd, IDC_BLOB));
    VERIFY(m_hwnddecimal = GetDlgItem(hwnd, IDC_DECIMAL));
    VERIFY(m_hwndmessage = GetDlgItem(hwnd, IDC_MESSAGE));
    VERIFY(m_hwndexcel = GetDlgItem(hwnd, IDC_EXCEL));

	VERIFY(m_hwndsql = GetDlgItem(hwnd, IDC_SQL));
	VERIFY(m_hwndstructonly = GetDlgItem(hwnd, IDC_STRUCTUREONLY));
	VERIFY(m_hwnddataonly = GetDlgItem(hwnd, IDC_DATAONLY));
	VERIFY(m_hwndstructdata = GetDlgItem(hwnd, IDC_BOTH));
	VERIFY(m_hwndversion = GetDlgItem(hwnd, IDC_VERSION));

	VERIFY(m_hwndselall = GetDlgItem(hwnd, IDC_SELECTALL));
	VERIFY(m_hwnddeselall = GetDlgItem(hwnd, IDC_DESELECTALL));
	//Get handle for combo
	VERIFY(m_hwndcpcombo = GetDlgItem(hwnd, IDC_CHARSETCOMBO));
	

    SendMessage(m_hwndbloblimit, WM_SETTEXT, 0,(LPARAM)L"10");
    SendMessage(m_hwnddecimal, WM_SETTEXT, 0,(LPARAM)L"2");

	m_p->Add(hwnd, IDC_XML, "XML", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_HTML, "HTML", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CSV, "CSV", "1", CHECKBOX);
    m_p->Add(hwnd, IDC_EXCEL, "EXCEL", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_JSON, "JSON", "0", CHECKBOX);

	m_p->Add(hwnd, IDC_SQL, "SQL", "0", CHECKBOX);

	m_p->Add(hwnd, IDC_BOTH, "STRUCTUREDATA", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_VERSION, "INCVERSION", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_STRUCTUREONLY, "STRUCTUREONLY", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_DATAONLY, "DATAONLY", "0", CHECKBOX);

	m_p->Add(hwnd, IDC_ADDCOLUMNS, "ADDCOLUMNS", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_EXPORTFILENAME, "FILENAME", "", TEXTBOX);
	m_p->Add(hwnd, IDC_BLOB, "TEXTLIMIT", "50", TEXTBOX);
	m_p->Add(hwnd, IDC_DECIMAL, "DECIMALLIMIT", "2", TEXTBOX);

	m_p2->Add(hwnd, IDC_ADDCOLUMNS, "AddColumns", "0", CHECKBOX);

    DisableCSVOptions();
    DisableExcelOptions();
	DisableSQLOptions();

	if(SendMessage(m_hwndcsv, BM_GETCHECK, 0, 0) == BST_CHECKED)
        EnableCSVOptions();
	
    if(SendMessage(m_hwndexcel, BM_GETCHECK, 0, 0) == BST_CHECKED)
        EnableExcelOptions();

	//if(SendMessage(m_hwndsql, BM_GETCHECK, 0, 0) == BST_CHECKED)
    //    EnableSQLOptions();

	if(SendMessage(m_hwndsql, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        SendMessage(m_hwndcsv, BM_SETCHECK, 1, 0);
        SendMessage(m_hwndsql, BM_SETCHECK, 0, 0);
        EnableCSVOptions();
        ChangeFileExtension();
    }

	SetStaticText();
	FillColumnList();
	//InitExpCodePageCombo
	//Set it to default always!
	InitExpCodePageCombo(hwnd, IDC_CHARSETCOMBO);
	
    if(m_isenablesqlexport == wyFalse)
    {
        EnableWindow(m_hwndsql, FALSE);
    }

	GetClientRect(m_hwnd, &m_dlgrect);
	GetWindowRect(m_hwnd, &m_wndrect);
		
	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwnd, EXPORTAS_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();

    return;
}

void 
CExportResultSet::InitExpCodePageCombo(HWND hdlg, wyInt32 ctrl_id)
{
	HWND hcpcombo  = GetDlgItem(hdlg, ctrl_id);
	wyInt32 count = SendMessage(hcpcombo , CB_GETCOUNT, 0, 0), itemcount;

	// Removes the contents in the combo 
	for(itemcount = 0; itemcount < count; itemcount++)
		SendMessage(hcpcombo , CB_DELETESTRING, 0, 0);


	SendMessage(hcpcombo , CB_INSERTSTRING, 0,(LPARAM)L"ascii");
	SendMessage(hcpcombo, CB_SETITEMDATA, 0, CPI_ASCII);
	SendMessage(hcpcombo , CB_INSERTSTRING, 1,(LPARAM)L"big5");
	SendMessage(hcpcombo, CB_SETITEMDATA, 1, CPI_BIG5);
	SendMessage(hcpcombo , CB_INSERTSTRING, 2,(LPARAM)L"cp1250");
	SendMessage(hcpcombo, CB_SETITEMDATA, 2, CPI_CP1250);
	SendMessage(hcpcombo , CB_INSERTSTRING, 3,(LPARAM)L"cp1251");
	SendMessage(hcpcombo, CB_SETITEMDATA, 3, CPI_CP1251);
	SendMessage(hcpcombo , CB_INSERTSTRING, 4,(LPARAM)L"cp1256");
	SendMessage(hcpcombo, CB_SETITEMDATA, 4, CPI_CP1256);
	SendMessage(hcpcombo , CB_INSERTSTRING, 5,(LPARAM)L"cp1257");
	SendMessage(hcpcombo, CB_SETITEMDATA, 5, CPI_CP1257);
	SendMessage(hcpcombo , CB_INSERTSTRING, 6,(LPARAM)L"cp850");
	SendMessage(hcpcombo, CB_SETITEMDATA, 6, CPI_CP850);
	SendMessage(hcpcombo , CB_INSERTSTRING, 7,(LPARAM)L"cp852");
	SendMessage(hcpcombo, CB_SETITEMDATA, 7, CPI_CP852);
	SendMessage(hcpcombo , CB_INSERTSTRING, 8,(LPARAM)L"cp866");
	SendMessage(hcpcombo, CB_SETITEMDATA, 8, CPI_CP866);
	SendMessage(hcpcombo , CB_INSERTSTRING, 9,(LPARAM)L"cp932");
	SendMessage(hcpcombo, CB_SETITEMDATA, 9, CPI_CP932);
	SendMessage(hcpcombo , CB_INSERTSTRING, 10,(LPARAM)L"euckr");
	SendMessage(hcpcombo, CB_SETITEMDATA, 10, CPI_EUCKR);
	SendMessage(hcpcombo , CB_INSERTSTRING, 11,(LPARAM)L"gb2312");
	SendMessage(hcpcombo, CB_SETITEMDATA, 11, CPI_GB2312);
	SendMessage(hcpcombo , CB_INSERTSTRING, 12,(LPARAM)L"greek");
	SendMessage(hcpcombo, CB_SETITEMDATA, 12, CPI_GREEK);
	SendMessage(hcpcombo , CB_INSERTSTRING, 13,(LPARAM)L"hebrew");
	SendMessage(hcpcombo, CB_SETITEMDATA, 13, CPI_HEBREW);
	SendMessage(hcpcombo , CB_INSERTSTRING, 14,(LPARAM)L"keybcs2");
	SendMessage(hcpcombo, CB_SETITEMDATA, 14, CPI_KEYBCS2);
	SendMessage(hcpcombo , CB_INSERTSTRING, 15,(LPARAM)L"koi8r");
	SendMessage(hcpcombo, CB_SETITEMDATA, 15, CPI_KOI8R);
	SendMessage(hcpcombo , CB_INSERTSTRING, 16,(LPARAM)L"koi8u");
	SendMessage(hcpcombo, CB_SETITEMDATA, 16, CPI_KOI8U);
	SendMessage(hcpcombo , CB_INSERTSTRING, 17,(LPARAM)L"latin1");
	SendMessage(hcpcombo, CB_SETITEMDATA, 17, CPI_LATIN1);
	SendMessage(hcpcombo , CB_INSERTSTRING, 18,(LPARAM)L"latin2");
	SendMessage(hcpcombo, CB_SETITEMDATA, 18, CPI_LATIN2);
	SendMessage(hcpcombo , CB_INSERTSTRING, 19,(LPARAM)L"latin5");
	SendMessage(hcpcombo, CB_SETITEMDATA, 19, CPI_LATIN5);
	SendMessage(hcpcombo , CB_INSERTSTRING, 20,(LPARAM)L"latin7");
	SendMessage(hcpcombo, CB_SETITEMDATA, 20, CPI_LATIN7);
	SendMessage(hcpcombo , CB_INSERTSTRING, 21,(LPARAM)L"macce");
	SendMessage(hcpcombo, CB_SETITEMDATA, 21, CPI_MACCE);
	SendMessage(hcpcombo , CB_INSERTSTRING, 22,(LPARAM)L"macroman");
	SendMessage(hcpcombo, CB_SETITEMDATA, 22, CPI_MACROMAN);
	SendMessage(hcpcombo , CB_INSERTSTRING, 23,(LPARAM)L"sjis");
	SendMessage(hcpcombo, CB_SETITEMDATA, 23, CPI_SJIS);
	SendMessage(hcpcombo , CB_INSERTSTRING, 24,(LPARAM)L"swe7");
	SendMessage(hcpcombo, CB_SETITEMDATA, 24, CPI_SWE7);
	SendMessage(hcpcombo , CB_INSERTSTRING, 25,(LPARAM)L"tis620");//thai
	SendMessage(hcpcombo, CB_SETITEMDATA, 25, CPI_TIS620);
	SendMessage(hcpcombo , CB_INSERTSTRING, 26,(LPARAM)L"ujis");
	SendMessage(hcpcombo, CB_SETITEMDATA, 26, CPI_UJIS);
	SendMessage(hcpcombo , CB_INSERTSTRING, 27,(LPARAM)L"utf8");
	SendMessage(hcpcombo, CB_SETITEMDATA, 27, CPI_UTF8);
	SendMessage(hcpcombo , CB_INSERTSTRING, 28,(LPARAM)L"utf8 with BOM");
	SendMessage(hcpcombo, CB_SETITEMDATA, 28, CPI_UTF8B);
	//SendMessage(hcpcombo , CB_INSERTSTRING, 29,(LPARAM)L"utf16le");
	//SendMessage(hcpcombo, CB_SETITEMDATA, 29, CPI_UTF16LE);
	//SendMessage(hcpcombo , CB_INSERTSTRING, 30,(LPARAM)L"utf16be");
	//SendMessage(hcpcombo, CB_SETITEMDATA, 30, CPI_UTF16BE);
	SendMessage(hcpcombo , CB_SELECTSTRING, -1,(LPARAM)L"utf8");
}


void 
CExportResultSet::FillColumnList()
{
	wyInt32		count, index, width;
    wyString	text, namestr;
    wyString    query;
    MDIWindow   *wnd = GetActiveWin();
    MYSQL_ROW   myrow;
    
	VERIFY(m_hwndcolsel = GetDlgItem(m_hwnd, IDC_COLUMNLIST));
	
	// Sets the H-scroll for listbox
	//SendMessage(m_hwndcolsel, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)SIZE_512, (LPARAM)0);
	
	if(m_res)
    {
        for(count = 0 ; count < m_res->field_count; count++)
	    {
		    namestr.SetAs(m_res->fields[count].name);
		    SendMessage(m_hwndcolsel, LB_ADDSTRING, 0,(LPARAM)namestr.GetAsWideChar());
	    }
    }
    else
    {
        if(!wnd)
            return;

        query.Sprintf("show full fields from `%s`.`%s`", m_ptr->m_db.GetString(), m_ptr->m_table.GetString());
        m_res = ExecuteAndGetResult(wnd, m_tunnel, &m_ptr->m_pmdi->m_mysql, query);  

        if(!m_res)
        {	ShowMySQLError(m_hwnd, m_tunnel, &m_ptr->m_pmdi->m_mysql, query.GetString());
            return;
		}

        myrow = m_tunnel->mysql_fetch_row(m_res);

        while(myrow)
	    {
            index = GetFieldIndex(m_res, "field", m_tunnel, &m_ptr->m_pmdi->m_mysql);
            
            if(index == -1)
                return;
		    
            namestr.SetAs(myrow[index]);
		    SendMessage(m_hwndcolsel, LB_ADDSTRING, 0,(LPARAM)namestr.GetAsWideChar());

            myrow = m_tunnel->mysql_fetch_row(m_res);
	    }

        m_tunnel->mysql_free_result(m_res);
        m_res = NULL;
    }

    //sets the maximum width for the list box	
    width = SetListBoxWidth(m_hwndcolsel);
	
	// Sets the H-scroll for listbox
	SendMessage(m_hwndcolsel, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)width + 5, (LPARAM)0);

	SendMessage(m_hwndcolsel, LB_SETSEL, TRUE, -1);
}

wyBool 
CExportResultSet::AddSelColumn()
{
	wyInt32	itemcount = 0, temp = 0, flag = 0;

	m_selectedfields = (wyBool *)calloc(sizeof(wyBool) , m_res->field_count);

	// Now get the selected item and get its info 
	while(itemcount < m_res->field_count)
	{
		temp = SendMessage(m_hwndcolsel, LB_GETSEL, itemcount, 0);

		if(temp)
		{
			m_selectedfields[itemcount] = wyTrue;
			flag = 1;
		}
		else
			m_selectedfields[itemcount] = wyFalse;

		itemcount++;
	}
	return wyTrue;
}


wyBool 
CExportResultSet::OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{

    m_wparam = wparam;
    m_lparam = lparam;

    switch(LOWORD(wparam))
	{
	case IDC_EXPFILESELECT:
		SetExpFile();
		break;

	case IDC_XML:
		OnXMLCheck();
		break;

	case IDC_HTML:
		OnHTMLCheck();
		break;

	case IDC_CSV:
		OnCSVCheck();
		break;

    case IDC_EXCEL:
		OnExcelCheck();
		break;

	case IDC_SQL:
		if(Button_GetCheck(GetDlgItem(hwnd, IDC_SQL)) == BST_CHECKED)
		    OnSQLCheck(hwnd);
		break;
	case IDC_JSON:
		OnJSONCheck();
		break;

	case IDC_CHANGE:
		{
            wyBool ret;
			ret = m_cesv.GetEscapeCharacters(hwnd);
			if(ret == wyTrue)
				SetStaticText();
		}
		break;			

	case IDCANCEL:
		m_p->Cancel();
		m_p2->Cancel();
		m_cesv.m_esch.m_towrite = wyTrue;
		VERIFY(yog_enddialog(hwnd, 0));
		break;

	case IDOK:
        if(m_exporting == wyTrue)
            StopExporting();
        else
            if(StartExport() == wyFalse)
                EndDialog(hwnd, 0); 
		break;

	case IDC_SELECTALL:
		SendMessage(m_hwndcolsel, LB_SETSEL, TRUE, -1);
		break;

	case IDC_DESELECTALL:
		SendMessage(m_hwndcolsel, LB_SETSEL, FALSE, -1);
		break;

		
    }

   return  wyTrue;
}

VOID
CExportResultSet::OnSQLCheck(HWND hwnd)
{
    if(m_fromquerytab == wyFalse)
		PostMessage(hwnd, SHOWWARNING, 0, 0);

	EnableSQLOptions();
	DisableExcelOptions();
	DisableCSVOptions();
    ChangeFileExtension();
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);
}

VOID
CExportResultSet::OnExcelCheck()
{
	EnableExcelOptions();
	DisableCSVOptions();
	DisableSQLOptions();
    ChangeFileExtension();
}

VOID
CExportResultSet::OnXMLCheck()
{
	DisableExcelOptions();
	DisableCSVOptions();
	DisableSQLOptions();
    ChangeFileExtension();
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);
}
VOID 
	CExportResultSet::OnJSONCheck()
{

	DisableExcelOptions();
	DisableCSVOptions();
	DisableSQLOptions();
    ChangeFileExtension();
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);

}
VOID
CExportResultSet::OnHTMLCheck()
{
	DisableExcelOptions();
	DisableCSVOptions();
	DisableSQLOptions();
	ChangeFileExtension();
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);
}

VOID
CExportResultSet::OnCSVCheck()
{
	EnableCSVOptions();
	DisableExcelOptions();
	DisableSQLOptions();
    ChangeFileExtension();
}


VOID
CExportResultSet::ChangeFileExtension()
{
	wyWChar	 file[MAX_PATH];
	wyString filename;
	wyString origfilename;
	wyString fileext;
	wyWChar	 *ext;

	VERIFY(SendMessage(m_hwndedit, WM_GETTEXT, MAX_PATH - 1,(LPARAM)file));

	origfilename.SetAs(file);
	filename.SetAs(file);

	ext = wcstok(file, L".");
	
	while((ext = wcstok(NULL, L".")) != NULL)
		fileext.SetAs(ext);

	if(fileext.GetLength())
        filename.Strip(fileext.GetLength());
    else
        filename.Add(".");

    if((SendMessage(GetDlgItem(m_hwnd, IDC_EXCEL), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("xml");

	if((SendMessage(GetDlgItem(m_hwnd, IDC_XML), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("xml");
	
	if((SendMessage(GetDlgItem(m_hwnd, IDC_JSON), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("json");

	if((SendMessage(GetDlgItem(m_hwnd, IDC_CSV), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("csv");

	if((SendMessage(GetDlgItem(m_hwnd, IDC_HTML), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("htm");

	if((SendMessage(GetDlgItem(m_hwnd, IDC_SQL), BM_GETCHECK, 0, 0))== BST_CHECKED)
		filename.Add("sql");


	if(origfilename.GetLength())
		VERIFY(SendMessage(m_hwndedit, WM_SETTEXT, 0,(LPARAM)filename.GetAsWideChar()));

}


VOID
CExportResultSet::StopExporting()
{
    // Request ownership of the critical section.
    EnterCriticalSection(&m_cs);

     m_stopexporting = wyTrue;

      // Release ownership of the critical section.
    LeaveCriticalSection(&m_cs);

}

wyBool
CExportResultSet::EnableExcelOptions()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXCELOPTIONS), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_SINSHEET), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_BLOB), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_BLOBTEXT), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_DECIMAL), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_DECIMALTEXT), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), TRUE);
    return wyTrue;
}

wyBool
CExportResultSet::DisableSQLOptions()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_SQLGROUP), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_STRUCTUREONLY), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_DATAONLY), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BOTH), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_VERSION), FALSE);

	return wyTrue;
}

wyBool
CExportResultSet::EnableSQLOptions()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_SQLGROUP), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_STRUCTUREONLY), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_DATAONLY), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BOTH), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_VERSION), TRUE);

    return wyTrue;
}

wyBool
CExportResultSet::DisableExcelOptions()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXCELOPTIONS), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_BLOB), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_BLOBTEXT), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_DECIMAL), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_DECIMALTEXT), FALSE);
	//EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);
    return wyTrue;
}

wyBool
CExportResultSet::EnableCSVOptions()
{
    EnableWindow(m_hwndchange, TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_ADDCOLUMNS), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVOPTIONS), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVFIELDS), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVLINES), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSTITLE), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPETITLE), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_LINESTITLE), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_LINESCHAR), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CHANGE), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), TRUE);
    return wyTrue;
}

wyBool
CExportResultSet::DisableCSVOptions()
{
    EnableWindow(m_hwndchange, TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_ADDCOLUMNS), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVOPTIONS), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVFIELDS), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSVLINES), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSTITLE), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPETITLE), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_LINESTITLE), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_LINESCHAR), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CHANGE), FALSE);
	//EnableWindow(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), FALSE);
    return wyTrue;
}

wyBool
CExportResultSet::DisableOtherOptions()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_XML), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSV), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_HTML), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXCEL), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_SQL), FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_JSON), FALSE);
    
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXPORTFILENAME), FALSE);

    EnableWindow(GetDlgItem(m_hwnd, IDCANCEL), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXPFILESELECT), FALSE);
		
    EnableWindow(m_hwndcolsel, FALSE);
    EnableWindow(m_hwndselall, FALSE);
    EnableWindow(m_hwnddeselall, FALSE);
		
    return wyTrue;
}

wyBool
CExportResultSet::EnableOtherOptions()
        {
    EnableWindow(GetDlgItem(m_hwnd, IDC_XML), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CSV), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_HTML), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXCEL), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_SQL), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_JSON), TRUE);

    EnableWindow(GetDlgItem(m_hwnd, IDC_EXPORTFILENAME), TRUE);

    EnableWindow(GetDlgItem(m_hwnd, IDCANCEL), TRUE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_EXPFILESELECT), TRUE);

    EnableWindow(m_hwndcolsel, TRUE);
    EnableWindow(m_hwndselall, TRUE);
    EnableWindow(m_hwnddeselall, TRUE);

    return wyTrue;
		}

wyBool
CExportResultSet::EnableAll()
{
    EnableOtherOptions();

    if((SendMessage(GetDlgItem(m_hwnd, IDC_EXCEL), BM_GETCHECK, 0, 0))== BST_CHECKED)
        EnableExcelOptions();

    if((SendMessage(GetDlgItem(m_hwnd, IDC_CSV), BM_GETCHECK, 0, 0))== BST_CHECKED)
        EnableCSVOptions();
    
    if((SendMessage(GetDlgItem(m_hwnd, IDC_SQL), BM_GETCHECK, 0, 0))== BST_CHECKED)
        EnableSQLOptions();

    SetWindowText(GetDlgItem(m_hwnd, IDOK), _(L"&Export"));

    return wyTrue;
    }

wyBool
CExportResultSet::DisableAll()
{
    DisableOtherOptions();
    DisableExcelOptions();
    DisableCSVOptions();
    DisableSQLOptions();

	return wyTrue;
}

wyBool
CExportResultSet::GetTableData()
{
    wyString query, message;
	

    m_resultfromquery = wyTrue;

    query.Sprintf("select * from `%s`.`%s`", m_ptr->m_db.GetString(), m_ptr->m_table.GetString());

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    SetWindowText(m_hwndmessage, _(L"Fetching Row(s) :"));

    m_res = ExecuteAndUseResult(m_tunnel, &m_ptr->m_pmdi->m_mysql, query);

    if(m_res == NULL)
    {
        message.SetAs(m_tunnel->mysql_error(m_ptr->m_pmdi->m_mysql));
        MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), 0);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return wyFalse;
    }

    m_field = m_tunnel->mysql_fetch_field(m_res);
    
    if(!m_field)
    {
        message.SetAs(m_tunnel->mysql_error(m_ptr->m_pmdi->m_mysql));
        MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), 0);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return wyFalse;
    }
    
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    SetWindowText(m_hwndmessage, L"");

    if(!m_res || !m_field)
        return wyFalse;
    else
        return wyTrue;
}

wyBool
CExportResultSet::StartExport()
{
	unsigned		thdid;
	StopExport		stopevt;
	wyInt32			count;
	wyBool			flag = wyFalse,is_csv = wyFalse,is_blob = wyFalse;
    wyWChar         file[MAX_PATH + 1] = {0};
	wyString		msg;
	wyInt32     create;
	

    
    DisableAll();
    EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);

    if(SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, 0)== 0)
	{
		yog_message(m_hwnd, _(L"Please specify a file name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		m_exporting = wyFalse;
		EnableAll();
		SendMessage(m_hwndmessage, WM_SETTEXT, 0, (LPARAM)L"");
        EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
        return wyTrue;
	}

	// get the file name
	VERIFY(SendMessage(m_hwndedit, WM_GETTEXT, MAX_PATH,(LPARAM)file));

	if(!OverWriteFile(m_hwnd, file))
	{
		m_exporting = wyFalse;
		EnableAll();
		SendMessage(m_hwndmessage, WM_SETTEXT, 0, (LPARAM)L"");
        EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
		return wyTrue;
	}

    m_filename.SetAs(file);

    if(!m_res)
    {
        if(GetTableData() == wyFalse)
            return wyFalse;
    }

    AddSelColumn();

	for(count = 0; count < m_res->field_count; count++)
		if(m_selectedfields[count] == wyTrue)
			flag = wyTrue;

    if(flag == wyFalse)
	{
		MessageBox(m_hwnd, _(L"Please select at least one column"), pGlobals->m_appname.GetAsWideChar(), 
                MB_OK | MB_ICONINFORMATION );
        EnableAll();
		SendMessage(m_hwndmessage, WM_SETTEXT, 0, (LPARAM)L"");
        EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
		goto freeup;
	}

//------------------------------------Warning message for BLOB Type Fixed Length csv export---------------------------------------//
	if(SendMessage(m_hwndcsv, BM_GETCHECK, 0, 0) == BST_CHECKED)
		is_csv = wyTrue;
	else
		is_csv = wyFalse;
	
for(count = 0; count < m_res->field_count; count++)
	{
		if(m_selectedfields[count] == wyTrue && m_res->fields[count].type>= FIELD_TYPE_TINY_BLOB && m_res->fields[count].type<= FIELD_TYPE_BLOB)
			is_blob = wyTrue;
	}

	if(is_csv && m_cesv.m_esch.m_isfixed && is_blob){
		msg.SetAs("Fixed length CSV is not supported for BLOB/TEXT types. \n Do you still want to continue?");
		create = yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(),  MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);
		
		if(create != IDYES)
		{
		m_exporting = wyFalse;
		EnableAll();
		SendMessage(m_hwndmessage, WM_SETTEXT, 0, (LPARAM)L"");
        EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
		goto freeup;
		}
	}
//------------------------------------------------------------------------------------------------------------------------------------//

	stopevt.m_exportevent = CreateEvent(NULL, TRUE, FALSE, NULL);
    stopevt.m_exportresultset = this;

	m_stopexporting = wyFalse;
    m_exporting     = wyTrue;

    DisableAll();
    EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);

    m_exportthread = (HANDLE)_beginthreadex( NULL, 0, CExportResultSet::ExportData, &stopevt, 0, &thdid );

	if(!m_exportthread)
    	goto cleanup;

    HandleMsgs(stopevt.m_exportevent, wyFalse);

cleanup:
    if(stopevt.m_exportevent)
        VERIFY(CloseHandle(stopevt.m_exportevent));

    if(m_exportthread)
	    VERIFY(CloseHandle(m_exportthread));

	EnableAll();

	if(m_threadret == wyTrue)
		yog_enddialog(m_hwnd, 1);
freeup:
    if(m_resultfromquery == wyTrue && m_res)
    {
        m_resultfromquery = wyFalse;
        m_tunnel->mysql_free_result(m_res);
        m_res = NULL;
    }
   return  wyTrue;
}

wyBool 
CExportResultSet::SetStaticText()
{
	wyString	escape, fescape, lescape, enclosed, nullreplaceby;
	if(m_cesv.m_esch.m_isescaped == wyTrue)
	{
		if(strlen(m_cesv.m_esch.m_escape) == 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			escape.SetAs(m_cesv.m_esch.m_escape);
			SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)escape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_ESCAPECHAR), WM_SETTEXT, 0,(LPARAM)L"''");

	if(m_cesv.m_esch.m_isfieldsterm == wyTrue)
	{
		if(strlen(m_cesv.m_esch.m_fescape)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			fescape.SetAs(m_cesv.m_esch.m_fescape);
			SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)fescape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_FIELDSCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");

	if(m_cesv.m_esch.m_isenclosed == wyTrue)
	{
		if(m_cesv.m_esch.m_isoptionally == wyTrue)
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0,(LPARAM)_(L"Optionally enclosed by :"));
		else
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0,(LPARAM)_(L"Enclosed by :"));
		if(strlen(m_cesv.m_esch.m_enclosed)== 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			enclosed.SetAs(m_cesv.m_esch.m_enclosed);
			SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0,(LPARAM)enclosed.GetAsWideChar());
		}
	}
	else
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDTITLE), WM_SETTEXT, 0,(LPARAM)_(L"Enclosed by :"));
		SendMessage(GetDlgItem(m_hwnd, IDC_ENCLOSEDCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
	}

	if(m_cesv.m_esch.m_islineterminated == wyTrue)
	{
		if(strlen(m_cesv.m_esch.m_lescape) == 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0,(LPARAM)L"<NONE>");
		else
		{
			lescape.SetAs(m_cesv.m_esch.m_lescape);
			SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0,(LPARAM)lescape.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_LINESCHAR), WM_SETTEXT, 0,(LPARAM)L"''");
	
	if(m_cesv.m_esch.m_isnullreplaceby == wyTrue)
	{
		if(strlen(m_cesv.m_esch.m_nullchar) == 0)
			SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHARDISPLAY), WM_SETTEXT, 0,(LPARAM)L"");
		else
		{
			nullreplaceby.SetAs(m_cesv.m_esch.m_nullchar);
			SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHARDISPLAY), WM_SETTEXT, 0,(LPARAM)nullreplaceby.GetAsWideChar());
		}
	}
	else
		SendMessage(GetDlgItem(m_hwnd, IDC_NULLCHARDISPLAY), WM_SETTEXT, 0,(LPARAM)L"\"\"");
	

	return wyTrue;
}



wyBool 
CExportResultSet::SetExpFile()
{
	wyWChar         file[MAX_PATH+1] = {0};
	OPENFILENAME	openfilename;

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = m_hwnd;
	openfilename.hInstance         = pGlobals->m_pcmainwin->GetHinstance();

	// depending upon the selection of the user we change the filtering type.
	if(SendMessage(m_hwndxml, BM_GETCHECK, 0, 0)== BST_CHECKED)
	{
		openfilename.lpstrFilter       = XML;
		openfilename.lpstrDefExt       = L"xml";
	}
	else if(SendMessage(m_hwndhtml, BM_GETCHECK, 0, 0)== BST_CHECKED)
	{
		openfilename.lpstrFilter	   = HTML;
		openfilename.lpstrDefExt       = L"html";
	}
	else if(SendMessage(m_hwndjson, BM_GETCHECK, 0, 0)== BST_CHECKED)
	{
		openfilename.lpstrFilter	   = JSON;
		openfilename.lpstrDefExt       = L"json";
	}
	else if(SendMessage(m_hwndcsv, BM_GETCHECK, 0, 0)== BST_CHECKED)
	{
		openfilename.lpstrFilter	   = CSV;	
		openfilename.lpstrDefExt       = L"csv";
	}
    else if(SendMessage(m_hwndexcel, BM_GETCHECK, 0, 0)== BST_CHECKED)
	{
		openfilename.lpstrFilter	   = EXCEL;	
		openfilename.lpstrDefExt       = L"xml";
	}
    else
	{
		openfilename.lpstrFilter	   = SQL;	
		openfilename.lpstrDefExt       = L"sql";
	}

	openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = file;
	openfilename.nMaxFile          = sizeof(file);
	openfilename.lpstrTitle        = _(L"Export As");
	openfilename.Flags             = OFN_HIDEREADONLY;
	openfilename.lpfnHook          =(LPOFNHOOKPROC)(FARPROC)NULL;
	openfilename.lpTemplateName    =(LPWSTR)NULL;
	
	if(GetSaveFileName(&openfilename))
	{
		SendMessage(m_hwndedit, WM_SETTEXT, 0,(LPARAM)file);
		return wyFalse;
	}

	return wyTrue;
}


unsigned __stdcall
CExportResultSet::ExportData(LPVOID lpparam)
{
	wyBool      ret = wyTrue;
   	StopExport  *resultset = (StopExport *)lpparam;
	
	if(resultset->m_exportresultset->m_stopexporting == wyFalse)
        SetWindowText(GetDlgItem(resultset->m_exportresultset->m_hwnd, IDOK), _(L"&Stop"));

	// get a handle to it.
	resultset->m_exportresultset->m_hfile = CreateFile(resultset->m_exportresultset->m_filename.GetAsWideChar(), 
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(resultset->m_exportresultset->m_hfile == INVALID_HANDLE_VALUE)
	{
		DisplayErrorText(GetLastError(), _("Could not create file"));
		ret = wyFalse;
		goto End;
	}

	if(SendMessage(resultset->m_exportresultset->m_hwndxml, BM_GETCHECK, 0, 0)== BST_CHECKED)
		ret = resultset->m_exportresultset->StartXMLExport(resultset, 
		resultset->m_exportresultset->m_filename.GetAsWideChar());
	else if(SendMessage(resultset->m_exportresultset->m_hwndjson, BM_GETCHECK, 0, 0)== BST_CHECKED)
		ret = resultset->m_exportresultset->StartJSONExport(resultset, 
		resultset->m_exportresultset->m_filename.GetAsWideChar());

	else if(SendMessage(resultset->m_exportresultset->m_hwndhtml, BM_GETCHECK, 0, 0)== BST_CHECKED)
		ret = resultset->m_exportresultset->StartHTMLExport(resultset, 
                            resultset->m_exportresultset->m_filename.GetAsWideChar());

    else if(SendMessage(resultset->m_exportresultset->m_hwndcsv, BM_GETCHECK, 0, 0)== BST_CHECKED)
		ret = resultset->m_exportresultset->StartCSVExport(resultset);

    else if(SendMessage(resultset->m_exportresultset->m_hwndexcel, BM_GETCHECK, 0, 0)== BST_CHECKED)
		ret = resultset->m_exportresultset->StartExcelExport(resultset);
	
	else
		ret = resultset->m_exportresultset->StartSimpleSQLExport(resultset);


End:
	resultset->m_exportresultset->m_threadret = ret;
    resultset->m_exportresultset->m_exporting = wyFalse;
    /// Sets event
    SetEvent(resultset->m_exportevent);
	
	return ret;
	}

wyBool 
CExportResultSet::StartXMLExport(StopExport *resultset, wyWChar *file)
{
	wyBool ret;
	wyInt32 ret1 = 0;
	ret =  resultset->m_exportresultset->SaveDataAsXML(resultset->m_exportresultset->m_hfile); 
	
	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));

	if(ret == wyTrue)
    {
        ret1 = yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully. Would you like to open file?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION);
        if(ret1 == IDYES)
        {
            ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
        }
		//ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
    }
	return ret;
}
wyBool
	CExportResultSet::StartJSONExport(StopExport *resultset, wyWChar *file)
{
    wyBool ret;
	wyInt32 ret1 = 0;

	ret =  resultset->m_exportresultset->SaveDataAsJson(resultset->m_exportresultset->m_hfile);
	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));
	if(ret == wyTrue)
    {
        ret1 = yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully. Would you like to open file?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION);
        if(ret1 == IDYES)
        {
            ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
        }
    }

	return ret;
}

wyBool
CExportResultSet::StartHTMLExport(StopExport *resultset, wyWChar *file)
{
	wyBool ret;
    wyInt32 ret1 = 0;

	ret =  resultset->m_exportresultset->SaveDataAsHTML(resultset->m_exportresultset->m_hfile); 
	
	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));
	
	if(ret == wyTrue)
    {
        ret1 = yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully. Would you like to open file?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION);
        if(ret1 == IDYES)
        {
            ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
        }
	    //ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
    }
	
	return ret;
}

wyBool
CExportResultSet::StartCSVExport(StopExport *resultset)
{
	wyBool ret;
        wyInt32     ret1;
        wyWChar     *file = resultset->m_exportresultset->m_filename.GetAsWideChar();

	ret = resultset->m_exportresultset->SaveDataAsCSV(resultset->m_exportresultset->m_hfile);

	if(m_hwnd)
		FlashIfInactive(m_hwnd);

	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));
	
	if(ret)
        {
            ret1 = yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully. Would you like to open file?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION);
            if(ret1 == IDYES)
            {
                ShellExecute(NULL, TEXT("open"), file, NULL, NULL, SW_SHOWNORMAL);
            }
        }
	else if(resultset->m_exportresultset->m_stopexporting != wyTrue)
		yog_message(resultset->m_exportresultset->m_hwnd, _(L"Error while exporting"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
	
	return ret;

}

wyBool
CExportResultSet::StartExcelExport(StopExport *resultset)
{
	wyWChar     bloblimit[10];
    wyWChar     decimalplaces[10];
	wyBool		ret;
	wyBool      multisheet = wyFalse;
        wyInt32     ret1;
    wyWChar     *file = resultset->m_exportresultset->m_filename.GetAsWideChar();

	SendMessage(resultset->m_exportresultset->m_hwndbloblimit, WM_GETTEXT, 10, (LPARAM) bloblimit);
    SendMessage(resultset->m_exportresultset->m_hwnddecimal, WM_GETTEXT, 10, (LPARAM) decimalplaces);

    ret = resultset->m_exportresultset->SaveDataAsEXCEL(multisheet, bloblimit, decimalplaces);
	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));

	if(m_hwnd)
		FlashIfInactive(m_hwnd);

	if(ret)
    {
	ret1 = yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully. Would you like to open file?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION);
        if(ret1 == IDYES)
        {
            ShellExecute(NULL,TEXT("open"),file,NULL,NULL,SW_SHOWNORMAL);
        }
    }
	else if(resultset->m_exportresultset->m_stopexporting != wyTrue)
		yog_message(resultset->m_exportresultset->m_hwnd, _(L"Error while exporting"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);

	return ret;
}

//Adding data to a buffer of size 8K and then writing to a file
wyBool
CExportResultSet::StartSimpleSQLExport(StopExport *resultset)
{
	wyBool ret = wyTrue;
	
	if(SendMessage(resultset->m_exportresultset->m_hwndstructonly, BM_GETCHECK, 0, 0) == BST_CHECKED)
		m_structonly = wyTrue;
	if(SendMessage(resultset->m_exportresultset->m_hwnddataonly, BM_GETCHECK, 0, 0)== BST_CHECKED)
		m_dataonly = wyTrue;
	if(SendMessage(resultset->m_exportresultset->m_hwndstructdata, BM_GETCHECK, 0, 0)== BST_CHECKED)
		m_structdata = wyTrue;
	if(SendMessage(resultset->m_exportresultset->m_hwndversion, BM_GETCHECK, 0, 0)== BST_CHECKED)
		m_includeversion = wyTrue;
	else 
		m_includeversion =wyFalse;
	
	ret  = resultset->m_exportresultset->SaveDataAsSQL();
	VERIFY(CloseHandle(resultset->m_exportresultset->m_hfile));

	if(ret == wyTrue)
		yog_message(resultset->m_exportresultset->m_hwnd, _(L"Data exported successfully"), 
		pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
	else if(resultset->m_exportresultset->m_stopexporting != wyTrue)
		yog_message(resultset->m_exportresultset->m_hwnd, _(L"Error while exporting"), 
                                       pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);

	return ret;
}

//Adding data to a buffer of size 8K and then writing to a file
wyBool 
CExportResultSet::SaveDataAsEXCEL(wyBool multisheet, wyWChar *textlimit, wyWChar *decimalplaces)
{
    ExportToExcel   exp;
    ExportExcelData data;
    wyBool          retval;
    wyInt32			comboindex = -1;
    data.m_fields       = m_field;
    data.m_filename     = m_hfile;
    data.m_result       = m_res;
    data.m_tabrec       = m_ptr;
    data.m_tunnel       = m_tunnel;
    data.m_hwndmessage  = m_hwndmessage;
    data.m_hwnd         = m_hwnd;
    data.m_stopped      = &m_stopexporting;
	data.m_selcol		= m_selectedfields;
	data.m_resultfromquery = m_resultfromquery;

    data.m_textlimit.SetAs(textlimit);
    data.m_decimal.SetAs(decimalplaces);

	comboindex = SendMessage(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), CB_GETCURSEL, 0, 0);
	if(comboindex != CB_ERR)
		exp.m_charset = SendMessage(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), CB_GETITEMDATA, comboindex, 0);

    retval = exp.PrintToExcel(&data);

    return retval;
}


wyBool 
CExportResultSet::SaveDataAsSQL()
{
    ExportAsSimpleSQL exportdata;
    ExportSQLData	data;
    wyBool          retval;
    
    data.m_fields			= m_field;
    data.m_filename			= m_hfile;
    data.m_result			= m_res;
    data.m_tabrec			= m_ptr;
    data.m_tunnel			= m_tunnel;
    data.m_hwndmessage		= m_hwndmessage;
    data.m_hwnd				= m_hwnd;
    data.m_stopped			= &m_stopexporting;
	data.m_selcol			= m_selectedfields;
	data.m_structonly		= m_structonly;
	data.m_dataonly			= m_dataonly;
	data.m_structdata		= m_structdata;
	data.m_includeversion	= m_includeversion;

    data.m_tablename.SetAs(m_res->fields[0].table, m_ptr->m_pmdi->m_ismysql41);
    data.m_dbname.SetAs(m_res->fields[0].db ? m_res->fields[0].db : "", m_ptr->m_pmdi->m_ismysql41);

	exportdata.m_isdatafromquery = m_resultfromquery;

    retval = exportdata.StartSQLExport(&data);

	if(m_hwnd)
		FlashIfInactive(m_hwnd);

    return retval;
}

void CExportResultSet::FlashIfInactive(HWND hwnd)
{
	if( GetForegroundWindow() != hwnd)
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);
}

/*
User has selected to exported the data as XML.
Adding  data first to a buffer and then to a file.
It gets the resultset data from the selected tab in the tab control thru its LPARAM
formats the eintire set and write it in a file.
Each row has a tag ROW and each column has the tag of its column header.
*/
wyBool 
CExportResultSet::SaveDataAsXML(HANDLE hfile)
{
	wyUInt32		j,k;
	BOOL			ret;
	DWORD			dwbyteswritten;
    wyInt32         messagecount = 0;
    wyInt32         rowcount = 0, rowptr = 0;
    wyString        messbuff, myrowstr, buffer(SIZE_8K);
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);
	MYSQL_RES		*myres;
	MYSQL_ROW		myrow;
	MYSQL_FIELD		*myfield;
	MYSQL_ROWEX	    *tmp = NULL;
	MYSQL_ROWS		*rowswalker = NULL;

	myres			= m_res;
	myfield			= m_field;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	//Add to buffer then to file
	buffer.Add("<data>");

	m_tunnel->mysql_data_seek(myres,(my_ulonglong)0);

   
    while(1)
	{
		if(IsStopExport() == wyTrue)
        {
            SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }

		// now depending upon the user has started to edit or not we get the row accordingly
        if(!m_ptr->m_rowarray->GetLength())
        {
			if(m_resultfromquery == wyTrue)
			{	
				//If use mysql_useresult() then we should use this
				myrow = m_tunnel->mysql_fetch_row(myres);
			}
			else
			{
				SeekCurrentRowFromMySQLResult(myres, &rowswalker, m_tunnel, &myrow, NULL);
			}
			if(!myrow)
				break;
		}
        else 
        {
            if(rowptr == m_ptr->m_rowarray->GetLength())
                break;

            //no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            
            
            if(m_ptr->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
			{
                rowptr++;
                break;
            }
            if(m_ptr->m_modifiedrow >=0 && m_ptr->m_modifiedrow == rowptr && m_ptr->m_oldrow->IsNewRow() == wyFalse)
            {
                tmp = m_ptr->m_oldrow;
                myrow = m_ptr->m_oldrow->m_row;
            }
            else
            {
                tmp = m_ptr->m_rowarray->GetRowExAt(rowptr);
                myrow =tmp->m_row;
            }
            rowptr++;
		}

		buffer.Add("<row>");
		
		for(j=0; j < (myres->field_count); j++)
        {
			if(m_selectedfields[j] == wyFalse)
				continue;
			
			buffer.Add("<");
			
			// we output the fields but before we check if there is a space in between.
			k=0;

			while(myfield[j].name[k])
			{
				switch(myfield[j].name[k])
				{
				case C_SPACE:
					buffer.Add("_");
					break;

				default:
					buffer.AddSprintf("%c", *(myfield[j].name+k));
					break;
				}
				
				k++;
			}

			
			buffer.Add(">");
			
			if(myrow[j] != NULL)
			{
				myrowstr.SetAs(myrow[j], ismysql41);

				//function to add text to buffer in an xml file handling characters.
				AddXMLToBuffer(&buffer, myrowstr.GetString(), wyFalse);
			}
			else
				buffer.Add(STRING_NULL);
				
			buffer.Add("</");

			// we output the fields but before we check if there is a space in between.
			k=0;
			
			while(myfield[j].name[k])
			{
			switch(myfield[j].name[k])
				{
				case C_SPACE:
					buffer.Add("_");
					break;

				default:
					buffer.AddSprintf("%c", *(myfield[j].name+k));
					break;
				}
				
				k++;
			}
			
			buffer.Add(">");
		}

		buffer.Add("</row>");
		
		buffer.Add("\r\n");
	
		//Write to file if buffer size is more
		if(buffer.GetLength() >= SIZE_8K)
		{
			ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);
			buffer.Clear();
			
			if(ret == FALSE)
			{
				DisplayErrorText(GetLastError(), _("Error writing in file."));
				return wyFalse;
			}
		}

		//Display number of rows exported
		messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(m_hwndmessage, WM_SETTEXT, 0,(LPARAM) messbuff.GetAsWideChar());
        rowcount++;
	}

	buffer.Add("</data>");
	
	ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);

	if(m_hwnd)
		FlashIfInactive(m_hwnd);
	
	if(ret == FALSE)
	{
		DisplayErrorText(GetLastError(), _("Error writing in file."));
		return wyFalse;
	}
	
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}
//this function export the data as Json in the file given as parameter
//format for josn is [{row 1}{row 2}....]
wyBool 
	CExportResultSet::SaveDataAsJson(HANDLE hfile)
{
	wyUInt32		j,count;
	BOOL			ret;
	DWORD			dwbyteswritten;
    wyInt32         messagecount = 0;
    wyInt32         rowcount = 0, rowptr = 0;
    wyString        messbuff, myrowstr, buffer(SIZE_8K);
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);
	MYSQL_RES		*myres;
	MYSQL_ROW		myrow;
	MYSQL_FIELD		*myfield;
	MYSQL_ROWEX	    *tmp = NULL;
	MYSQL_ROWS		*rowswalker = NULL;
	wyString		temp;
	myres			= m_res;
	myfield			= m_field;
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	//Add to buffer then to file
	buffer.Add("[ \r\n");

	m_tunnel->mysql_data_seek(myres,(my_ulonglong)0);

   
    while(1)
	{
		if(IsStopExport() == wyTrue)
        {
            SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }

		// now depending upon the user has started to edit or not we get the row accordingly
        if(!m_ptr->m_rowarray->GetLength())
        {
			if(m_resultfromquery == wyTrue)
			{	
				//If use mysql_useresult() then we should use this
				myrow = m_tunnel->mysql_fetch_row(myres);
			}
			else
			{
				SeekCurrentRowFromMySQLResult(myres, &rowswalker, m_tunnel, &myrow, NULL);
			}
			if(!myrow)
				break;
		}
        else 
        {
            if(rowptr == m_ptr->m_rowarray->GetLength())
                break;

            //no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            
            
            if(m_ptr->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
			{
                rowptr++;
                break;
            }
            if(m_ptr->m_modifiedrow >=0 && m_ptr->m_modifiedrow == rowptr && m_ptr->m_oldrow->IsNewRow() == wyFalse)
            {
                tmp = m_ptr->m_oldrow;
                myrow = m_ptr->m_oldrow->m_row;
            }
            else
            {
                tmp = m_ptr->m_rowarray->GetRowExAt(rowptr);
                myrow =tmp->m_row;
            }
            rowptr++;
		}

		if(rowcount != 0)
			buffer.Add(",\r\n");

		buffer.Add("{ \r\n");
		count = 0;
		for(j=0; j < (myres->field_count); j++)
        {
			if(m_selectedfields[j] == wyFalse)
				continue;

			if(count!=0)
			buffer.Add(",\r\n");

			buffer.Add("\"");
			temp.SetAs(myfield[j].name);
			temp.JsonEscape();
			buffer.Add(temp.GetString());
			buffer.Add("\":");
			
			if(myrow[j] != NULL)
			{
				myrowstr.SetAs(myrow[j], ismysql41);

				//Now add the value. If it number than use as it is other wise use double quotes refer-http://json.org/
				if(myfield[j].type==MYSQL_TYPE_DECIMAL ||myfield[j].type==MYSQL_TYPE_FLOAT ||myfield[j].type==MYSQL_TYPE_INT24
					||myfield[j].type==MYSQL_TYPE_LONG||myfield[j].type==MYSQL_TYPE_LONGLONG||myfield[j].type==MYSQL_TYPE_NEWDECIMAL
					||myfield[j].type==MYSQL_TYPE_SHORT || myfield[j].type==MYSQL_TYPE_JSON)
					buffer.Add(myrowstr.GetString());
				 else
				
			    {
						buffer.Add("\"");
						temp.SetAs(myrowstr.GetString());
						temp.JsonEscape();
						buffer.Add(temp.GetString());
						buffer.Add("\"");
				}
			}
			else
				buffer.Add("null");
		count++;
		}
		
			 buffer.Add("\r\n}");
			 
		
	
		//Write to file if buffer size is more
		if(buffer.GetLength() >= SIZE_8K)
		{
			ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);
			buffer.Clear();
			
			if(ret == FALSE)
			{
				DisplayErrorText(GetLastError(), _("Error writing in file."));
				return wyFalse;
			}
		}

		//Display number of rows exported
		messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(m_hwndmessage, WM_SETTEXT, 0,(LPARAM) messbuff.GetAsWideChar());
        rowcount++;
	}
	//remove last ',' 
//	buffer.SetCharAt(buffer.GetLength()-3,' ');
		buffer.Add("]");
	
	ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);
	
	
	if(m_hwnd)
		FlashIfInactive(m_hwnd);


	if(ret == FALSE)
	{
		DisplayErrorText(GetLastError(), _("Error writing in file."));
		return wyFalse;
	}
	
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

wyBool 
CExportResultSet::IsStopExport() 
{
    wyBool ret;
    // Request ownership of the critical section.
    EnterCriticalSection(&m_cs);

	if(m_stopexporting == wyTrue)
		ret = wyTrue;
	else
		ret = wyFalse;
    
    // Release ownership of the critical section.
    LeaveCriticalSection(&m_cs);

    return ret;
}

/* It gets the resultset data from the selected tab in the tab control thru its LPARAM
 formats the entire set and writes it in a file.
 Adding data to a buffer of size 8K and then writing to a file
 */
 wyBool 
CExportResultSet::SaveDataAsCSV(HANDLE hfile)
{
	BOOL			ret;
	wyChar			fterm, lterm, encl;
	wyBool          isescaped, isfterm, islterm, isencl;
	unsigned long   *l = NULL;
	wyUInt32        numfields, i;
	wyULong			*length = NULL;
    DWORD           dwbytesread;
	MYSQL_ROWEX	    *tmp = NULL;
	MYSQL_ROWS		*rowswalker = NULL;
    wyInt32         messagecount = 0, rescount = 0, rowptr = 0, charset = 0, comboindex = -1,count;
	wyString        messbuff, myrowstr, buffer(SIZE_8K), msg;
	wyInt32			lenptr = 0;
	MYSQL_RES		*myres;
	MYSQL_FIELD		*fields;
	MYSQL_ROW		myrow;
	wyChar			*encbuffer;
	//wyWChar		    *wencbuffer;
	//wyUInt32		wlenptr = 0;
	VERIFY(myres	= m_res);
	wyBool			iswritten = wyFalse;
	unsigned char Header[3]; //unicode text file header
	Header[0] = 0xEF;
	Header[1] = 0xBB;
	Header[2] = 0xBF;

	//unsigned char Headerle[2]; //unicode text file header
	//Headerle[0] = 0xFF;
	//Headerle[1] = 0xFE;


	//unsigned char Headerbe[2]; //unicode text file header
	//Headerbe[0] = 0xFE;
	//Headerbe[1] = 0xFF;


	//wyUInt32		widelen = 0;
	// process the escaping characters.
	m_cesv.ProcessEscChar(m_cesv.m_esch.m_enclosed);
	m_cesv.ProcessEscChar(m_cesv.m_esch.m_escape);
	m_cesv.ProcessEscChar(m_cesv.m_esch.m_fescape);
	m_cesv.ProcessEscChar(m_cesv.m_esch.m_lescape);

	m_cesv.GetDefEscChar(&fterm, &isfterm, &lterm, &islterm, &encl, &isencl);

	isescaped   = m_cesv.m_esch.m_isescaped;
	
	fields		= m_tunnel->mysql_fetch_fields(myres);	
	numfields	= m_tunnel->mysql_num_fields(myres);
	l			= new unsigned long [numfields+1];

	m_tunnel->mysql_data_seek(myres, 0);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// first add columns.
	if((SendMessage(GetDlgItem(m_hwnd, IDC_ADDCOLUMNS), BM_GETCHECK, 0, 0)) == BST_CHECKED)
		AddColumnName(&buffer, m_cesv.m_esch.m_fescape, m_cesv.m_esch.m_lescape);

	//Get the charset encoding
	comboindex = SendMessage(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), CB_GETCURSEL, 0, 0);
	if(comboindex != CB_ERR)
		charset = SendMessage(GetDlgItem(m_hwnd, IDC_CHARSETCOMBO), CB_GETITEMDATA, comboindex, 0);
    /*if(m_ptr->m_rowarray)
		tmp = m_ptr->m_rowarray[0]; */
		
	while(1)
	{
        if(IsStopExport() == wyTrue)
        {
            SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }
		// now depending upon the user has started to edit or not we get the row accordingly
        if(!m_ptr->m_rowarray->GetLength())
        {
			if(m_resultfromquery == wyFalse)
			{
				SeekCurrentRowFromMySQLResult(myres, &rowswalker, m_tunnel, &myrow, &length); 
			}
			else
			{
				//If use mysql_useresult() then we should use this
				myrow = m_tunnel->mysql_fetch_row(myres);
			}
			if(!myrow)
				break;

		} 
        else 
        {
            if(rowptr == m_ptr->m_rowarray->GetLength())
                break;

            //no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            
            
            if(m_ptr->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
			{
                rowptr++;
                break;
            }
            if(m_ptr->m_modifiedrow >=0 && m_ptr->m_modifiedrow == rowptr && m_ptr->m_oldrow->IsNewRow() == wyFalse)
            {
                tmp = m_ptr->m_oldrow;
                myrow = m_ptr->m_oldrow->m_row;
            }
            else
            {
                tmp = m_ptr->m_rowarray->GetRowExAt(rowptr);
                myrow = tmp->m_row;
            }
            rowptr++;

			//Gets row_lengths
			GetColLengthArray(myrow, numfields, l);
			length = l;
		}

        if(!m_ptr->m_rowarray->GetLength() && m_resultfromquery == wyTrue)
		{
			length = m_tunnel->mysql_fetch_lengths(myres);
		}
		/*else 
        {
            GetColLengthArray(myrow, numfields, l);
			length = (wyUInt32*)l;
		}*/
		
		//First data is added to buffer and then written to file
		count = 0;
		for(i=0; i<numfields; i++)
		{
			if(m_selectedfields[i] == wyFalse)
				continue;

			if(count != 0)
				buffer.Add(m_cesv.m_esch.m_fescape);

            if(m_cesv.m_esch.m_isfixed)
			{
				//Adds data of fixed length to the buffer.
				ret = m_cesv.WriteFixed(&buffer, myrow[i], &fields[i], length[i], isescaped, 
										fterm, isfterm, lterm, islterm, encl, isencl);
			}
			else
			{
				//Adds data of variable length to the buffer.	
				ret = m_cesv.WriteVarible(&buffer, myrow[i], &fields[i], length[i], isescaped, 
										fterm, isfterm, lterm, islterm, encl, isencl);
				
				// if only add the separator if its not the last field.
					
			}
			count++; 
		}
		//Freeing the row_lengths buffer 
        if(!m_ptr->m_rowarray->GetLength() && m_resultfromquery == wyFalse && length)
		{
			free(length);
			length = NULL;
		}

		//Adds to buffer
		buffer.Add(m_cesv.m_esch.m_lescape);

		//Check buffer size and write in file
		if(buffer.GetLength() >= SIZE_8K)
		{
			//convert buffer encoding to users selected encoding
			//write converted string to the file
			if(iswritten == wyFalse && (charset == CPI_UTF8B) )
			{
				if(charset == CPI_UTF8B)
					ret = WriteFile(hfile, Header, 3, &dwbytesread, NULL);
				/*if(charset == CPI_UTF16LE)
					ret = WriteFile(hfile, Headerle, 2, &dwbytesread, NULL);
				if(charset == CPI_UTF16BE)
					ret = WriteFile(hfile, Headerbe, 2, &dwbytesread, NULL);*/
				iswritten = wyTrue;
			}
		
		if(charset != CPI_UTF8 && charset != CPI_UTF8B )
		{
				encbuffer =  buffer.GetAsEncoding(charset, &lenptr);
				ret = WriteFile(hfile, encbuffer, lenptr, &dwbytesread, NULL);
		}
		/*else
		if(charset == CPI_UTF16LE || charset == CPI_UTF16BE)
		{
			wencbuffer =  buffer.GetAsWideChar(&wlenptr);
			ret = WriteFile(hfile, wencbuffer, wlenptr * sizeof(wyWChar), &dwbytesread, NULL);
		}*/
		else
		{
				ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbytesread, NULL);
		}
		buffer.Clear();

		if(!ret)
		{
			DisplayErrorText(GetLastError(), _("Could not write into file !"));
			return wyFalse;
		}
		}		
		
		messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(m_hwndmessage, WM_SETTEXT, 0, (LPARAM) messbuff.GetAsWideChar());
        rescount++;
	}

	//Freeing the row_lengths buffer 
    if(!m_ptr->m_rowarray->GetLength() && m_resultfromquery == wyFalse && length)
	{
		free(length);
		length = NULL;
	}

	//If the buffer is not empty, then write the data to file
	if(buffer.GetLength() != 0)
	{
		if(iswritten == wyFalse && (charset == CPI_UTF8B ) )
			{
				//if(charset == CPI_UTF8B)
					ret = WriteFile(hfile, Header, 3, &dwbytesread, NULL);
				/*if(charset == CPI_UTF16LE)
					ret = WriteFile(hfile, Headerle, 2, &dwbytesread, NULL);
				if(charset == CPI_UTF16BE)
					ret = WriteFile(hfile, Headerbe, 2, &dwbytesread, NULL);*/
				iswritten = wyTrue;
			}
		
		if(charset != CPI_UTF8 && charset != CPI_UTF8B )
		{
				encbuffer =  buffer.GetAsEncoding(charset, &lenptr);
				ret = WriteFile(hfile, encbuffer, lenptr, &dwbytesread, NULL);
		}
		/*else
		if(charset == CPI_UTF16LE || charset == CPI_UTF16BE)
		{
			wencbuffer =  buffer.GetAsWideChar(&wlenptr);
			ret = WriteFile(hfile, wencbuffer, wlenptr * sizeof(wyWChar), &dwbytesread, NULL);
		}*/
		else
		{
				ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbytesread, NULL);
		}
		buffer.Clear();

		if(!ret)
		{
			DisplayErrorText(GetLastError(), _("Could not write into file !"));
			return wyFalse;
		}
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	delete[] l;

	return wyTrue;
}


wyBool
CExportResultSet::AddColumnName(wyString * buffer, wyChar *fescape, wyChar *lescape)
{
	wyUInt32    fieldcount=0, i=0;
    wyString    query, myfieldstr;
	
	fieldcount	= m_tunnel->mysql_num_fields(m_res);

	for(i=0; i < fieldcount; i++)
	{
		if(m_selectedfields[i] == wyFalse)
			continue;
		else
			query.AddSprintf("%s%s", m_field[i].name, fescape);
	}

	// remove the comma.
    query.Strip(strlen(fescape));
	query.AddSprintf("%s", lescape);
	
	//add to buffer
	buffer->Add(query.GetString());
	
	return wyTrue;
}

//add the file contents to buffer then writing to file
wyBool 
CExportResultSet::SaveDataAsHTML(HANDLE hfile)
{
	wyUInt32        j, messagecount = 0, rowcount = 0, rowptr = 0;
	wyString        messbuff, buffer(SIZE_8K);
	BOOL			ret;
	DWORD			dwbyteswritten;
	MYSQL_RES		*myres;
	MYSQL_ROW		myrow;
	MYSQL_FIELD		*myfield;
	MYSQL_ROWS		*rowswalker = NULL;

	myres		=	 m_res;
	myfield		=	 m_field;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// Start adding HTML to a buffer.
	buffer.Add("<html>\r\n<head>\r\n");

	buffer.Add("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");

	buffer.Add("<title>query data</title>\r\n");

	buffer.Add("<style type=\"text/css\"> <!--\r\n.normal {  font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 12px; font-weight: normal; color: #000000}\r\n.medium {  font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 15px; font-weight: bold; color: #000000; text-decoration: none}\r\n--></style>\r\n</head>\r\n");

	buffer.Add("<body>\r\n<h3>query result</h3>");
	
	buffer.Add("<table border=1>\r\n<tr>\r\n");

	// Write the top row thats the title bar.
	for(j = 0; j < (myres->field_count); j++)
    {
		if(m_selectedfields[j] == wyFalse)
			continue;
		
		buffer.Add("<td bgcolor=silver class='medium'>");
		
		buffer.Add(myfield[j].name);
		
		buffer.Add("</td>");
	}

	buffer.Add("</tr>\r\n");
	
	m_tunnel->mysql_data_seek(myres,(my_ulonglong)0);

	/*if(m_ptr->m_data)
		tmp = m_ptr->m_data->m_data; */

	while(1)
	{

        if(IsStopExport() == wyTrue)
        {
            SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }

		// now depending upon the user has started to edit or not we get the row accordingly
        if(!m_ptr->m_rowarray->GetLength())
        {
			if(m_resultfromquery == wyTrue)
			{
				//If use mysql_useresult() then we should use this
				myrow = m_tunnel->mysql_fetch_row(myres);
			}

			else
			{
				SeekCurrentRowFromMySQLResult(myres, &rowswalker, m_tunnel, &myrow, NULL);			
			}

			if(!myrow)
				break;
		} 
        else 
        {	
			//no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            if(rowptr == m_ptr->m_rowarray->GetLength())
                break;
            if(m_ptr->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
            {
                rowptr++;
				continue;
            }
            if(m_ptr->m_modifiedrow >=0 && m_ptr->m_modifiedrow == rowptr && m_ptr->m_oldrow->IsNewRow() == wyFalse)
            {
                myrow = m_ptr->m_oldrow->m_row;
            }
            else
            {
                myrow = m_ptr->m_rowarray->GetRowExAt(rowptr)->m_row;
            }
            rowptr++;
		}

		buffer.Add("<tr>\r\n");
		
		for(j = 0; j < (myres->field_count); j++)
		{
			if(m_selectedfields[j] == wyFalse)
				continue;

			buffer.Add("<td class='normal' valign='top'>");
			
			if(myrow[j] != NULL)
			{
				// we check for each character so that we can replace some specific characters
				// like < or > with its original value which the browser understands.

				if(myrow[j][0] == 0)
					buffer.Add("&nbsp;");
				else
				{
					//function to add text to buffer in an xml file handling characters.
					AddXMLToBuffer(&buffer, myrow[j], wyTrue);
				}
			}
			else
				buffer.Add(STRING_NULL);
				
			buffer.Add("</td>\r\n");
		}

		buffer.Add("</tr>\r\n");
		
		//If buffer size is more then write to file
		if(buffer.GetLength() >= SIZE_8K)
		{
			ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);
			buffer.Clear();
			
			if(ret == FALSE)
			{
				DisplayErrorText(GetLastError(), _("Error writing in file."));
				return wyFalse;
			}
		}

		//To display number of rows exported
        messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(m_hwndmessage, WM_SETTEXT, 0,(LPARAM) messbuff.GetAsWideChar());
        rowcount++;
	}

	buffer.Add("</table>\r\n</body></html>");

	ret = WriteFile(hfile, buffer.GetString(), buffer.GetLength(), &dwbyteswritten, NULL);
	
	buffer.Clear();

	
	if(m_hwnd)
		FlashIfInactive(m_hwnd);

	if(ret == FALSE)
	{
		DisplayErrorText(GetLastError(), _("Error writing in file."));
		return wyFalse;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

void
CExportResultSet::GetColLengthArray(MYSQL_ROW row, wyInt32 num_col, unsigned long *len)
{
	VERIFY ( len );

	unsigned long		*to=NULL, *prev_length = 0;
	unsigned int		field_count;
	byte				*start=0;
	MYSQL_ROW			column = row;
	MYSQL_ROW			end;
	
	if ( !column )
		return;		// something is wrong

	to			= len;
	field_count = num_col;

	for (end=column + field_count + 1 ; column != end ; column++, to++)
	{
		if (!*column)
		{
			*to= 0;					// NULL
			continue;
		}
		
		if (start)					// Found end of prev string 
			*prev_length = (unsigned long) ((byte*)*column-start-1);
		
		start= (byte*)*column;
		prev_length=to;
	}

	return;

}

void
CExportResultSet::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_HORZBAR1, 1, 0,
        //IDC_CSV, 0, 0,
		IDC_JSON,0,0,
        IDC_HTML, 0, 0,
        IDC_XML, 0, 0,
        IDC_EXCEL, 0, 0,
        IDC_SQL, 0, 0,
        IDC_SELFLDS, 1, 1,
        IDC_COLUMNLIST, 1, 1,
        IDC_SELECTALL, 0, 0,
        IDC_DESELECTALL, 0, 0,
        IDC_TEXT1, 0, 0,
        IDC_EXPORTFILENAME, 1, 0,
        IDC_EXPFILESELECT, 0, 0,
        IDC_HORZBAR2, 1, 0,
		IDC_HORZBAR3, 1, 0,
        IDOK, 0, 0,
        IDCANCEL, 0, 0,
        IDC_MESSAGE, 0, 0,
		IDC__EXPORTAS_GRIP, 0, 0,
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
CExportResultSet::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;

    RECT        rect;

    HDWP        hdefwp;
		
	GetClientRect(m_hwnd, &rect);

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
				case IDC_HTML:
					x = (rect.right - rect.left)/3 +20;
					break;

				case IDC_XML:
					x = (rect.right - rect.left)/2+25;
					break;

				case IDC_EXCEL:
					x = (rect.right - rect.left)*2/3+25;
					break;
               case IDC_JSON:
				   x = (rect.right - rect.left)/5;
				   break;
				case IDC_SQL:
				case IDC_EXPFILESELECT:
				case IDC_DESELECTALL:
				case IDOK:
				case IDCANCEL:
					x = rect.right - rightpadding - width;
					break;
				
				case IDC__EXPORTAS_GRIP:
					x = rect.right - width;
					break;

				default:
					x = leftpadding;
			}
        }
        else
        {
			x = leftpadding;
			width = rect.right - leftpadding - rightpadding;
        }
	    switch(pdlgctrl->m_id)
        {
			case IDOK:
			case IDC_HORZBAR2:
			case IDC_HORZBAR3:
			case IDCANCEL:
			case IDC_TEXT1:
			case IDC_EXPORTFILENAME:
			case IDC_EXPFILESELECT:
			case IDC_MESSAGE:
			case IDC_SELECTALL:
			case IDC_DESELECTALL:
                y = rect.bottom - bottompadding - height;
                break;

			case IDC__EXPORTAS_GRIP:
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
CExportResultSet::ExpDatResize(HWND hwnd){

	PositionCtrls();
}

void
CExportResultSet::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

void
CExportResultSet::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC__EXPORTAS_GRIP);
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