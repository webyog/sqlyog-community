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


#include "AutoCompleteInterface.h"

#ifndef COMMUNITY
#include "AutoCompleteEnt.h"
#endif

AutoCompleteInterface::AutoCompleteInterface()
{
    m_autocomplete = NULL;
}

AutoCompleteInterface::~AutoCompleteInterface()
{
#ifndef COMMUNITY
    if(m_autocomplete)
        delete m_autocomplete;

    m_autocomplete = NULL;
#endif
}

wyBool 
AutoCompleteInterface::HandlerInitAutoComplete(MDIWindow * wnd)
{
#ifndef COMMUNITY
    if(IsAutoComplete() == wyTrue)
    {
        m_autocomplete = InitAutoComplete(wnd);

        if(m_autocomplete)
            return wyTrue;
    }
#endif
    return wyFalse;
}

void	            
AutoCompleteInterface::HandlerBuildTags(MDIWindow *wnd)
{
#ifndef COMMUNITY
    if(pGlobals->m_isautocomplete == wyTrue)
		if(wnd->m_psqlite->m_refcount == 1)
            HandlerStoreObjects(wnd, wyFalse);
#endif
}

void 
AutoCompleteInterface::HandlerStoreObjects(MDIWindow *wnd, wyBool rebuild)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        StoreObjects(wnd, m_autocomplete, rebuild);
    }
#endif
}

wyInt32 
AutoCompleteInterface::HandlerOnWMChar(HWND hwnd, EditorBase *eb, WPARAM wparam)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        return m_autocomplete->OnWMChar(hwnd, wparam, NULL);
    }
#endif
	 eb->SetAutoIndentation(hwnd, wparam);
	 return 0;
}

wyInt32 
AutoCompleteInterface::HandlerOnWMKeyDown(HWND hwnd, EditorBase *eb, WPARAM wparam)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        return m_autocomplete->OnWMKeyDown(hwnd, eb, wparam);
    }
#endif
    return 0;
}

wyInt32 
AutoCompleteInterface::HandlerOnWMKeyUp(HWND hwnd, EditorBase *eb, WPARAM wparam)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        return m_autocomplete->OnWMKeyUp(hwnd, eb, wparam);
    }
#endif
    return 0;
}

wyBool 
AutoCompleteInterface::HandlerAddToMessageQueue(MDIWindow*	wnd, wyChar *query)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        return m_autocomplete->HandlerAddToMessageQueue(wnd, query);
    }
#endif
    return wyFalse;
}

void 
AutoCompleteInterface::HandlerProcessMessageQueue(MDIWindow *wnd)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        m_autocomplete->ProcessMessageQueue(wnd);
    }
#endif
}


void
AutoCompleteInterface::HandlerOnAutoThreadExit(MDIWindow *wnd, LPARAM lparam)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        return m_autocomplete->OnAutoThreadExit(wnd, lparam);
    }
#endif
}

//Sets the "AutocompleteTagbuilded" to '1' once the Build tags process is completed
VOID
AutoCompleteInterface::UpdateTagBuildFlag()
{
	wyString	dirstr;
	wyInt32		ret = 0, value = 0;;
	wyWChar		directory[MAX_PATH] = {0}, *lpfileport = 0;
	ConnectionBase *conbase = pGlobals->m_pcmainwin->m_connection;

	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH - 1, directory, &lpfileport);
	if(ret == 0)
		return;

	dirstr.SetAs(directory);

	//write to .ini the 'Building' tag file is completed successful(AutocompleteTagbuilded = 1)
	if(conbase)
	{
		value = conbase->m_isbuiltactagfile == wyTrue ? 1 : 0;
        
		wyIni::IniWriteInt(conbase->m_consectionname.GetString(), 
			"AutocompleteTagbuilded", value, dirstr.GetString());		
	}
}

wyBool 
AutoCompleteInterface::OnACNotification(WPARAM wparam, LPARAM lparam)
{
#ifndef COMMUNITY
    if(m_autocomplete)
    {
        if(((LPNMHDR)lparam)->code == SCN_AUTOCSELECTION)
        {
            CAutoComplete::OnACSelection(wparam, lparam);
            return wyTrue;
        }
        else if(((LPNMHDR)lparam)->code == SCN_CALLTIPCLICK)
        {
            m_autocomplete->OnCallTipClick(((LPNMHDR)lparam)->hwndFrom, wparam, lparam);
            return wyTrue;
        }
        else if(((LPNMHDR)lparam)->code == SCN_CHARADDED)
        {
            m_autocomplete->OnWMChar(((LPNMHDR)lparam)->hwndFrom, wparam, lparam);
            return wyFalse;
        }
        else if(((LPNMHDR)lparam)->code == SCN_UPDATEUI)
        {
            m_autocomplete->OnUpdateUI(wparam, lparam);
            return wyFalse;
        }
    }
#endif
    return wyFalse;
}
