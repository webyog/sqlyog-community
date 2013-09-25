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

#include "PreferenceCommunity.h"
#include "EditorFont.h"
#include "CommonHelper.h"

extern PGLOBALS		pGlobals;

/* Applies all the changes or current values to the .ini file */
void
PreferencesCommunity::Apply()
{
	if(m_ispreferenceapplied)
		return;
    SaveGeneralPreferences(m_hwnd, GENERALPREF_PAGE);

    SaveFontPreferences(m_hwnd, FONT_PAGE); 

	SaveOthersPreferences(m_hwnd, OTHERS_PAGE);

	EnumChildWindows(pGlobals->m_hwndclient, EnumChildProc, (LPARAM)this);

	m_ispreferenceapplied = wyTrue;
	return;
}

void
PreferencesCommunity::RestoreAllDefaults()
{
	m_isrestorealldefaults = wyTrue;//user clicked RestoreAllTabs button

	SetGenPrefDefaultAllTabValues(m_hwnd, GENERALPREF_PAGE); //Setting General pref dialogue with default values
	SetFontPrefDefAllTabValues(m_hwnd, FONT_PAGE);//Setting Font pref dialogue with default values
	SetOthersPrefDefaultAllTabValues(m_hwnd, OTHERS_PAGE);//Setting Others pref dialogue with default values

	return;
}

//Disabling all the Enterprise-Powertools tab controls
void
PreferencesCommunity::EnterprisePrefHandleWmInitDialog(HWND hwnd) 
{
	HWND		hwndacgrp = NULL;
	wyInt32     publicid[] = { IDC_PTGROUP, IDC_ACGROUP, IDC_DEFTABGROUP, IDC_AUTOCOMPLETE, 
								IDC_AUTOCOMPLETEHELP, IDC_AUTOCOMPLETEREBUILD,
								IDC_AUTOCOMPLETESHOWTOOLTIP, IDC_TAGDIRGROUP, 
								IDC_AUTOCOMPLETETAGSDIR, IDC_DIRSEL, IDC_CONFIRMONSDCLOSE,	
								IDC_POWERTOOLSRESTOREALL, IDC_POWERTOOLSRESTORETAB, IDC_PQA, 
								IDC_PQAEXPLAIN, IDC_PQAEXPLAINEXT, IDC_PQAPROF, 
								IDC_PQAADVISOR, IDC_PQASTATUS};

    wyInt32     count = sizeof(publicid)/ sizeof(publicid[0]);

	if(m_startpage == AC_PAGE)
        HandlerToSetWindowPos(hwnd);
	EnableOrDisable(hwnd, publicid, count , wyFalse);	

	//Power Tools Gropbox handle
	hwndacgrp = GetDlgItem(hwnd, IDC_PTGROUP);
	
	//Sets the Groupbox options
	SendMessage(hwndacgrp, WM_SETTEXT, 0, (LPARAM)COMMMUNITY_POWERTOOLS);	
}

void
PreferencesCommunity::EntPrefHandleWmCommand(HWND hwnd, WPARAM wParam)
{
}
void 
PreferencesCommunity::FormatterPrefHandleWmNotify(HWND hwnd, LPARAM lParam)
{
    LPNMHDR lpnm = (LPNMHDR)lParam;
	wyString dirstr;
    switch (lpnm->code)
    {
	case PSN_SETACTIVE:
		dirstr.SetAs(m_directory);
		m_hwnd = hwnd;
		pGlobals->m_prefpersist=FORMATTER_PAGE;
		wyIni::IniWriteInt(GENERALPREFA, "PrefPersist", FORMATTER_PAGE, dirstr.GetString());
		break;
	case PSN_APPLY: //user pressed the OK button.		
		Apply();
		break;

	}	
}
void 
PreferencesCommunity::ACPrefHandleWmNotify(HWND hwnd, LPARAM lParam)
{
    LPNMHDR lpnm = (LPNMHDR)lParam;
	wyString dirstr;
    switch (lpnm->code)
    {
	case PSN_SETACTIVE:
		dirstr.SetAs(m_directory);
		m_hwnd = hwnd;
		pGlobals->m_prefpersist=AC_PAGE;
		wyIni::IniWriteInt(GENERALPREFA, "PrefPersist", AC_PAGE, dirstr.GetString());
		break;
	case PSN_APPLY: //user pressed the OK button.		
		Apply();
		break;

	}	
}
//Disabling all Enterprise-Formtter tab controls
void
PreferencesCommunity::FormatterPrefHandleWmInitDialog(HWND hwnd)
{
	HWND		hwndacgrp = NULL;
	HWND		hwndpreview = NULL;
	wyInt32     publicid[] = { IDC_OPTIONSGROUP, IDC_TABCOLLIST, IDC_LINEBRK,
								IDC_PREVIEWGROUP, IDC_STACKED, IDC_BEFORECOMMA, 
								IDC_AFTERCOMMA, IDC_NOTSTACKED,  
								IDC_INDENT, IDC_INDENTATION, IDC_SPACES, IDC_FORMATTERPREVIEW, 
								IDC_FORMATTERRESTOREALL, IDC_FORMATTERRESTORETAB };

	wyInt32     count = sizeof(publicid)/ sizeof(publicid[0]);
	if(m_startpage == FORMATTER_PAGE)
		HandlerToSetWindowPos(hwnd);

	hwndpreview = GetDlgItem(hwnd, IDC_FORMATTERPREVIEW);

	//setting editor properties
	SetPreviewEditor(hwnd);
	//Setting a query to preview window
	SendMessage(hwndpreview, SCI_SETTEXT, 0, (LPARAM)FORMATTER_PREVIEW);

	EnableOrDisable(hwnd, publicid, count , wyFalse);

	//Formatter Groupbox	
	hwndacgrp = GetDlgItem(hwnd, IDC_OPTIONSGROUP);

	//Sets the Groupbox caption
	SendMessage(hwndacgrp, WM_SETTEXT, 0, (LPARAM)COMMUNITY_FORMATTER);
}

void        
PreferencesCommunity::FormatterPrefHandleWmCommand(HWND hwnd, WPARAM wParam)
{
}