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


#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "GUIHelper.h"
#include "CommonHelper.h"

extern	PGLOBALS	pGlobals;

#define	RELDLGWD	500
#define	RELDLGHT    350

/*! \fn wyBool InitializeShortKeys(HWND hwnd)
    \brief Initilizes the shortcutkeys dialog procedure
    \returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool InitializeShortKeys(HWND hwnd);
wyBool LoadFileFromPgmResource(HWND hwnd, wyString& str);

INT_PTR CALLBACK	
KeyShortCutsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	wyBool ret;	

	switch(message)
	{
	case WM_INITDIALOG:
        LocalizeWindow(hwnd);
		ret = InitializeShortKeys(hwnd);
		if(ret == wyFalse)
			PostMessage(hwnd, UM_CLOSE, 0, 0);
		break;

	case WM_COMMAND:
		{
			switch LOWORD(wParam)
			{	
			case IDCANCEL:
			case IDOK:
				PostMessage(hwnd, UM_CLOSE, 0, 0);
				break;
			}
		}
		break;

	case WM_SIZE:
		//resize keyboard shortcut dialog
		KeyShortCutResize(hwnd);
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
		break;

	case WM_DESTROY:
		//StoreDialogPersist(hwnd, KEYSHORTCUT_SECTION);
		SaveInitPos(hwnd, KEYSHORTCUT_SECTION);
		break;

	case UM_CLOSE:
		VERIFY(yog_enddialog(hwnd,  1));
		break;

	default:
		break;
	}

	return 0;
}

wyBool 
InitializeShortKeys(HWND hwnd)
{
	HICON		    hicon;
    wyString        str, key, command;
    const wyChar    *substr, *tabstr;
    wyInt32         i, x = 0, tabpos;
    HWND            hwndedit;

	//Set icon for dialog	
	hicon = CreateIcon(IDI_KEYSHORT);
	SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
    DestroyIcon(hicon);
	
	//SetDialogPos(hwnd, KEYSHORTCUT_SECTION);
	SetInitPos(hwnd, KEYSHORTCUT_SECTION);
	
	//Resize dialog
	KeyShortCutResize(hwnd);
    LoadFileFromPgmResource(hwnd, str);
    hwndedit = GetDlgItem(hwnd, IDC_KEYSHORTS);

    for(i = str.Find("\r\n", x), substr = str.GetString(); substr; i = str.Find("\r\n", x))
    {
        command.Clear();

        if(i < 0)
        {
            key.Sprintf("%s", substr + x);
        }
        else
        {
            key.Sprintf("%.*s", i - x, substr + x);
        }
        
        if((tabpos = key.FindChar('\t', 0)) != -1)
        {
            tabstr = key.GetString();
            for(; *(tabstr + tabpos) == '\t'; ++tabpos);
            command.SetAs(tabstr + tabpos);
            key.Sprintf("%.*s", tabpos, substr + x);
        }

        SendMessage(hwndedit, EM_REPLACESEL, 0, (LPARAM)(command.GetLength() ? key.GetAsWideChar() : _(key.GetAsWideChar())));
        SendMessage(hwndedit, EM_REPLACESEL, 0, (LPARAM)_(command.GetAsWideChar()));

        if(i < 0)
        {
            break;
        }
        else
        {
            SendMessage(hwndedit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
            x = i + 2;
        }
    }

    return wyTrue;
}

wyBool 
LoadFileFromPgmResource(HWND hwnd, wyString& str)
{
	HRSRC		hfound;
	LPVOID		lpvdata;
	HGLOBAL		hreadme;
	
	if(!(hfound = FindResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_README), L"README")))
    {
        return wyFalse;
    }

	hreadme = LoadResource(NULL, hfound);
    lpvdata = LockResource(hreadme);
    str.SetAsDirect((wyChar*)lpvdata, SizeofResource(GetModuleHandle(0), hfound));
    UnlockResource(hreadme);
    FreeResource(hreadme);
	return wyTrue;
}
