/* Copyright (C) 2013 Webyog Inc.

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

/*********************************************

Author: Vishal P.R

*********************************************/

#ifndef _L10N_H_
#define _L10N_H_

#include "CommonHelper.h"

#define LANGUAGE_DBFILE "l10n.db"

class L10n
{
    public:
        L10n(HWND hwnd, const wyChar* dbname);
        ~L10n();
        static INT_PTR CALLBACK    DlgProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);
        void Show();
        wyBool InitDialog(HWND hwnd);
        wyInt32 OnOK(HWND hwnd);
        wyInt32 MatchLocale(wyString* plocale, HWND hwndcombo);

    private:
        Language* m_plang;
        const wyChar* m_dbname;
        wyInt32 m_langcount;
        HWND m_hwndparent;
};

// Helper functions to apply localization------------------------------------------ 

///Helper function to set the language for the session
void SetLanguage(HWND hwndparent, wyBool isautomated);

///Function localizes the menu, i.e converting each string into localized version
void LocalizeMenu(HMENU hmenu);

///Function localizes the window, i.e converting each string into localized version
void LocalizeWindow(HWND hwnd, void (_cdecl *rearrange)(HWND hwnd, wyBool beforetranslating, LPARAM& lparam) = NULL);

///Sample function which rearranges the controls.
void RearrangeControls(HWND hwnd, wyBool beforetranslating, LPARAM& lparam);

///The function loads the string from resource and give the translated version. 
///Be careful that it should only be used in places where you can supply a resource id. If the translation of the string is not available the function returns MAKEINTRESOURCE(id)
wyWChar* LocalizeStringResource(wyInt32 id, HINSTANCE hinstance, wyBool defaulttoid = wyTrue);

#endif