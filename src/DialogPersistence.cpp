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


#include "DialogPersistence.h"
#include "CommonHelper.h"

DialogPersistence::DialogPersistence(HWND parent, wyInt32 id, const wyChar *atrribute, const wyChar *defvalue, const wyChar *section, PersistWinType t)
{
	m_hwndparent= parent;
	m_id        = id;
	m_type      = t;

	m_attribute.SetAs(atrribute);
	m_defvalue.SetAs(defvalue);
	m_section.SetAs(section);

	ReadFromFile();
}

DialogPersistence::~DialogPersistence()
{
}

// Writes a parameters value from ini file and sets the text to the window.

void
DialogPersistence::ReadFromFile()
{
	wyWChar		directory[MAX_PATH+1];
	wyWChar		*lpfileport=0;
	HWND		hwnd;
	wyString	dirstr, lastvalstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	wyIni::IniGetString(m_section.GetString(), m_attribute.GetString(), m_defvalue.GetString(), &lastvalstr, dirstr.GetString());

	switch(m_type)
	{
	case TEXTBOX:
		VERIFY(hwnd = GetDlgItem(m_hwndparent, m_id));
		VERIFY(SetWindowText(hwnd, lastvalstr.GetAsWideChar()));
		//GetWindowText(hwnd, lastvalue, sizeof(lastvalue)- 1);
		break;

	case CHECKBOX:
        if(lastvalstr.Compare("1") == 0)
			SendMessage(GetDlgItem(m_hwndparent, m_id), BM_SETCHECK, BST_CHECKED, 0);
		else 
			SendMessage(GetDlgItem(m_hwndparent, m_id), BM_SETCHECK, BST_UNCHECKED, 0);
		break;

	case COMBOBOX_P:
		{
			wyInt32 search;
			//lastvalstr.SetAs(lastvalue);
			search  = SendMessage(GetDlgItem(m_hwndparent, m_id), CB_FINDSTRINGEXACT, -1, (LPARAM)lastvalstr.GetAsWideChar());

			if(search == CB_ERR)
				search = 0;				// select the first item.

			SendMessage(GetDlgItem(m_hwndparent, m_id), CB_SETCURSEL, search, 0); 
		}
		break;
	}
}


void
DialogPersistence::WriteToFile()
{
	wyWChar		directory[MAX_PATH + 1] = {0};
	wyWChar		lastvalue[SIZE_512] = {0};
	wyWChar		*lpfileport;
	wyUInt32	ret;
	wyString	lastvalstr, dirstr;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	if(ret == 0)
		return ;

	// get the text first.
	switch(m_type)
	{
	case COMBOBOX_P:
	case TEXTBOX:
		GetWindowText(GetDlgItem(m_hwndparent, m_id), lastvalue, SIZE_512 - 1);
		break;

	case CHECKBOX:
		{
			wyInt32 checked;

			checked = SendMessage(GetDlgItem(m_hwndparent, m_id), BM_GETCHECK, 0, 0);

			if(checked)
				wcscpy(lastvalue, L"1");
			else
				wcscpy(lastvalue, L"0");
		}
		break;
	}

	lastvalstr.SetAs(lastvalue);
	dirstr.SetAs(directory);
	VERIFY(wyIni::IniWriteString(m_section.GetString(), m_attribute.GetString(), lastvalstr.GetString(), dirstr.GetString()));
}

Persist::Persist()
{
	m_cancel = wyFalse;
}

Persist::~Persist()
{
	// we write back the details and free all the buffer.
	DialogPersistence		*elem;

	elem = (DialogPersistence*)m_list.GetFirst();
	while(elem)
	{
		DialogPersistence	*next;
		
		VERIFY(elem =(DialogPersistence*)m_list.GetFirst());

		m_list.Remove(elem);

		next =(DialogPersistence*)elem->m_next;

		if(m_cancel == wyFalse)
			elem->WriteToFile();
		
		delete elem;

        elem = next;
	}
}

void
Persist::Create(wyChar *sectionname)
{
	m_section.SetAs(sectionname);
}

void
Persist::Add(HWND hwnd, wyInt32 id, wyChar *attribute, wyChar *defvalue, PersistWinType isbool)
{
	DialogPersistence		*elem = new DialogPersistence(hwnd, id, attribute, defvalue, m_section.GetString(), isbool);

	m_list.Insert(elem);
}

void
Persist::DeleteFromSection(wyWChar * attribute)
{
	wyWChar		directory[MAX_PATH+1] = {0};
	wyWChar		*lpfileport=0;
	wyInt32		ret;
	wyString	attributestr(attribute), dirstr;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	if(ret == 0)
		return ;
	
	dirstr.SetAs(directory);
	//WritePrivateProfileStringA(m_section.GetString(), attributestr.GetString(), NULL, dirstr.GetString());
	wyIni::IniDeleteKey(m_section.GetString(), (wyChar *)attributestr.GetString(), dirstr.GetString());
}

void  
Persist::Cancel()
{
    m_cancel = wyTrue;
}