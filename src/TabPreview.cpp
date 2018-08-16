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

#include "TabPreview.h"
#include "MDIWindow.h"
#include "Global.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabFields.h"
#include "TabAdvancedProperties.h"
#include "TabForeignKeys.h"
#include "TabIndexes.h"
#include "EditorFont.h"

#ifndef COMMUNITY
#include "SCIFormatter.h"
#endif

extern PGLOBALS		pGlobals;

TabPreview::TabPreview(HWND hwndparent, TableTabInterfaceTabMgmt *ptabmgmt)
{
    m_hwnd              =   hwndparent;
    m_hwndpreview       =   NULL;
    m_mdiwnd            =   GetActiveWin();
    m_ptabmgmt          =   ptabmgmt;
    m_renamequeryrowno  =   -1;
    m_istabempty        =   wyTrue;

    /// flag to avoid processing EN_CHANGE message in the common window procedure
    m_settingpreviewcontent = wyFalse;
}

void
TabPreview::Create()
{
    wyUInt32	exstyles    =  0;
	wyUInt32	styles      =  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;
    
    //..SetScintillaModes func clears all values and sends EN_CHANGE to the parent window. To avoid handling that message, we have set this flag to wyTrue;
    m_settingpreviewcontent = wyTrue;

    /// Creating Scintilla window
    VERIFY(m_hwndpreview	= ::CreateWindowEx(exstyles, L"Scintilla", L"Source", styles | WS_TABSTOP, 0, 0, 0, 0,
                        m_hwnd, (HMENU)IDC_QUERYEDIT, pGlobals->m_pcmainwin->GetHinstance(), this));

    m_wporigproc =(WNDPROC)SetWindowLongPtr(m_hwndpreview, GWLP_WNDPROC,(LONG_PTR)TabPreview::WndProc);
	SetWindowLongPtr(m_hwndpreview, GWLP_USERDATA,(LONG_PTR)this);
    
    //Set scintilla properties
	SetScintillaModes(m_hwndpreview, m_mdiwnd->m_keywordstring, m_mdiwnd->m_functionstring, wyFalse, wyTrue, "MySQL");
	
    /// disabling the default context menu
	SendMessage(m_hwndpreview, SCI_USEPOPUP, 0, 0);

	SendMessage(m_hwndpreview, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
    SetFont();
    EditorFont::SetColor(m_hwndpreview, wyTrue);
    SendMessage(m_hwndpreview, SCI_SETSCROLLWIDTH, 20, 0);
    SendMessage(m_hwndpreview, SCI_SETSCROLLWIDTHTRACKING, (WPARAM)1, 0);
    UpdateWindow(m_hwndpreview);
    ShowWindow(m_hwndpreview, SW_HIDE);
    m_settingpreviewcontent = wyFalse;
}

void
TabPreview::SetFont()
{
    EditorFont::SetFont(m_hwndpreview, "HistoryFont", wyTrue);
}

//Function enables and disables copy in history menu 
wyBool 
TabPreview::ChangeMenuItem(HMENU hmenu)
{
	if(SendMessage(m_hwndpreview, SCI_GETSELECTIONSTART, 0, 0) != SendMessage(m_hwndpreview, SCI_GETSELECTIONEND, 0, 0))
    {
		EnableMenuItem(hmenu, ID_OPEN_COPY, MF_ENABLED);
    }
    else
        EnableMenuItem(hmenu, ID_OPEN_COPY, MF_DISABLED);
	
	return wyTrue;
}

// Function to perform operations when the user right clicks on the window.
wyInt32
TabPreview::OnContextMenu(LPARAM lParam)
{
	HMENU   hmenu, htrackmenu;
	POINT   pnt;
	wyInt32 pos;
    RECT    rect;

    /// Calculating the context-menu position
	if(lParam == -1)//If the context menu is generated from the keyboard then lParam = -1
	{		
		pos = SendMessage(m_hwndpreview, SCI_GETCURRENTPOS, 0, 0);
		pnt.x = SendMessage(m_hwndpreview, SCI_POINTXFROMPOSITION, 0, pos) ; 
		pnt.y = SendMessage(m_hwndpreview, SCI_POINTYFROMPOSITION, 0, pos); 
		VERIFY(ClientToScreen(m_hwndpreview, &pnt));
	}
	else
	{
		pnt.x   =   (LONG)LOWORD(lParam);
		pnt.y   =   (LONG)HIWORD(lParam);
	}

    GetClientRect(m_hwndpreview, &rect);
    MapWindowPoints(m_hwndpreview, NULL, (LPPOINT)&rect, 2);

    if(!PtInRect(&rect, pnt))
    {
        return -1;
    }

	hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_PREVIEWMENU));
    LocalizeMenu(hmenu);
	ChangeMenuItem(hmenu);

	htrackmenu =	GetSubMenu(hmenu, 0);
	m_menu = htrackmenu;
	wyTheme::SetMenuItemOwnerDraw(m_menu);
	TrackPopupMenu(m_menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, m_hwndpreview, NULL);

	FreeMenuOwnerDrawItem(m_menu);

	DestroyMenu(m_menu);

	return 1;
}


LRESULT	CALLBACK 
TabPreview::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TabPreview	*tabpreview	=(TabPreview*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
	switch(message)
	{
    case WM_HELP:
        {
            ShowHelp("http://sqlyogkb.webyog.com/article/97-preview");
        }
        return 1;
        break;
        
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case ID_OPEN_COPY:
                /// copying selected text to clipboard
                CopyStyledTextToClipBoard(hwnd);
                break;

            case ID_OPEN_SELECTALL:
                /// Selecting all text
                SendMessage(hwnd, SCI_SELECTALL, 0, 0);
                break;

            case ID_OPEN_COPYALL:
                {
                    /// Copying all text to clipboard
                    int pos, spos, epos;
                    spos = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
		            epos = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
		            pos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		            SendMessage(hwnd, SCI_SELECTALL, 0, 0);
		            CopyStyledTextToClipBoard(hwnd);
		            SendMessage(hwnd, SCI_SETSELECTION,(WPARAM)epos, (LPARAM)spos);
                }
                break;
            }
        }
        break;
        
    case WM_CONTEXTMENU:
        /// Show context-menu
        if(tabpreview->OnContextMenu(lParam) == 1)
        {
            return 1;
        }
        break;

    case WM_KEYUP:
        if(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog == wyFalse)
        {
            PostMessage(tabpreview->m_mdiwnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
        }

        break;

    case WM_SETFOCUS:
        if(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog == wyFalse)
        {
            PostMessage(tabpreview->m_mdiwnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
        }

        break;

    case WM_KILLFOCUS:
        if(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog == wyFalse)
        {
            PostMessage(tabpreview->m_mdiwnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
        }

        break;

    case WM_LBUTTONUP:
        if(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog == wyFalse)
        {
            PostMessage(tabpreview->m_mdiwnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
        }

        break;

    case WM_KEYDOWN:
        if(wParam == VK_ESCAPE)
        {
            /// If tab is open in dialogbox(SD), then send UM_CLOSEDLG message to the tabbed interface wndproc when user presses Esc
            if(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
            {
                PostMessage(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, UM_CLOSEDLG, wParam, lParam);
                return 0;
            }
        }
        if(wParam == VK_TAB)
        {
            SetFocus(GetNextDlgTabItem(tabpreview->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, GetKeyState(VK_SHIFT) & SHIFTED ? TRUE : FALSE));
            return 1;
        }

        if(HandleScintillaStyledCopy(hwnd, wParam))
        {
            return 1;
        }
        break;

    case WM_SYSKEYDOWN:
        return tabpreview->OnWMSysKeyDown(wParam, lParam);

    case WM_MEASUREITEM:
        return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lParam, tabpreview->m_menu);

	case WM_DRAWITEM:		
        return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lParam);	
    }
    
    return CallWindowProc(tabpreview->m_wporigproc, hwnd, message, wParam, lParam);
}

LRESULT
TabPreview::OnWMSysKeyDown(WPARAM wParam, LPARAM lParam)
{
    if(!m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        return 1;

    return m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(0, wParam, lParam);
}

wyInt32
TabPreview::OnWMChar(HWND hwnd, WPARAM wparam)
{
    wyInt32 state = 0;

	state = GetKeyState(VK_CONTROL);
	if(state & 0x8000)
	{
		state = GetKeyState('C');

		if(state & 0x8000) //COPY
		{
			CopyStyledTextToClipBoard(hwnd);
			return 0;
		}
	}		
    return -1;
}

void 
TabPreview::Resize()
{
    RECT			rcmain;
	
    GetClientRect(m_hwnd, &rcmain);

    if(m_hwndpreview)
    {
        if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
            MoveWindow(m_hwndpreview, rcmain.left + 2 , rcmain.top, rcmain.right - 3, rcmain.bottom - 2 , TRUE);
        else
            MoveWindow(m_hwndpreview, rcmain.left + 1 , rcmain.top, rcmain.right - 2, rcmain.bottom - 1 , TRUE);
    }
}

void
TabPreview::SetScintillaValues(HWND hwndedit)
{
    /* XPM */
	static  wyChar *keyword_xpm[] = KEYWORD_XPM;
    static  wyChar *function_xpm[] = FUNCTION_XPM;
    static  wyChar *database_xpm[] = DATABASE_XPM;
    static  wyChar *table_xpm[] = TABLE_XPM;
    static  wyChar *field_xpm[] = FIELD_XPM;
    static  wyChar *sp_xpm[] = SP_XPM;
    static  wyChar *func_xpm[] = FUNC_XPM;
	static  wyChar *alias_xpm[] = ALIAS_XPM;

    SendMessage(m_hwndpreview, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"MySQL");
	SendMessage(m_hwndpreview, SCI_USEPOPUP, 0, 0);
    SendMessage(hwndedit, SCI_CLEARREGISTEREDIMAGES, 0, 0);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 8, (LPARAM)alias_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 7, (LPARAM)func_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 6, (LPARAM)sp_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 5, (LPARAM)field_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 4, (LPARAM)table_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 3, (LPARAM)database_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 2, (LPARAM)function_xpm);
	SendMessage(hwndedit, SCI_REGISTERIMAGE, 1, (LPARAM)keyword_xpm);

    if(IsInsertSpacesForTab() == wyTrue)
        SendMessage(hwndedit,SCI_SETUSETABS, FALSE, 0);
    else
        SendMessage(hwndedit,SCI_SETUSETABS, TRUE, 0);

    SendMessage(hwndedit, SCI_SETTABWIDTH, GetTabSize(), 0 );
	SendMessage(hwndedit, SCI_AUTOCSETIGNORECASE, TRUE, 0);
	SendMessage(hwndedit, SCI_AUTOCSETSEPARATOR, (long)'\n', 0);
	SendMessage(hwndedit, SCI_AUTOCSETAUTOHIDE, 0, 0);
	SendMessage(hwndedit, SCI_AUTOCSTOPS, 0, (LONG)" ~`!@#$%^&*()+|\\=-?><,/\":;'{}[]");
	
	SendMessage(hwndedit, SCI_CALLTIPSETBACK, RGB(255, 255, 225), 0);
	SendMessage(hwndedit, SCI_CALLTIPSETFORE, RGB(0, 0, 0), 0);
	SendMessage(hwndedit, SCI_CALLTIPSETFOREHLT, RGB(10, 10, 200), 0);
	SendMessage(hwndedit, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
    
    //Increase the Default Autocomplete Window Height
	SendMessage(hwndedit, SCI_AUTOCSETMAXHEIGHT, 10, 0);
    SetParanthesisHighlighting(hwndedit);
}

void
TabPreview::CancelChanges()
{
    m_renamequeryrowno = -1;
}

void
TabPreview::UpdateTableName(wyString &newname)
{
    wyString    rename("");
    wyString    dbname(""), oldname("");
    wyInt32     endpos, startpos;
    wyString    tmpstr(""), strnoquery;
	wyChar*		backtick;

	strnoquery.Sprintf("/* %s */", NO_PENDING_QUERIES);

    m_ptabmgmt->m_tabinterfaceptr->GetComboboxValue(m_ptabmgmt->m_tabinterfaceptr->m_hcmbdbname, dbname);
    oldname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);

    /// Setting Preview window editable
    SendMessage(m_hwndpreview, SCI_SETREADONLY, false, 0);

    dbname.FindAndReplace("`", "``");
    oldname.FindAndReplace("`", "``");
    newname.FindAndReplace("`", "``");

	//from  .ini file
	backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    if(!m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)      /// for create table only
    {
        if(!newname.GetLength())
        {
            wyString    query;
            GenerateQuery(query);

            if(!query.GetLength())
            {
                m_istabempty = wyTrue;
                SetPreviewContent((wyChar*)strnoquery.GetString());
                SendMessage(m_hwndpreview, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
                SendMessage(m_hwndpreview, SCI_SETSELECTIONEND, (WPARAM)0, 0);

                //..Setting Preview window Read-Only
                SendMessage(m_hwndpreview, SCI_SETREADONLY, true, 0);
                return;
            }
        }
        
        /// First line's end position (index of the last character in the line)
        endpos = SendMessage(m_hwndpreview, SCI_GETLINEENDPOSITION, (WPARAM)0, 0);

        /// Forming a query
        tmpstr.Sprintf("%s%s%s.%s%s%s (", backtick, dbname.GetString(), backtick, 
			backtick, newname.GetString(), backtick);

        //..If table name is missing, then add error message in the query
        if(!newname.GetLength())
        {
            newname.SetAs(TABLE_NAME_MISSING);
            tmpstr.AddSprintf("\t\t/* %s */", newname.GetString());
            newname.Clear();
        }

        //..Selecting and Replacing the Text
        SendMessage(m_hwndpreview, SCI_SETSEL, (WPARAM)13, endpos);
        SendMessage(m_hwndpreview, SCI_REPLACESEL, 0, (LPARAM)tmpstr.GetString());
        
    }
    else        //..for alter table only
    {
        //..Generating rename query
        rename.Sprintf("Rename table %s%s%s.%s%s%s to %s%s%s.%s%s%s", 
			backtick, dbname.GetString(), backtick,
			backtick, oldname.GetString(), backtick,
			backtick, dbname.GetString(), backtick,
			backtick, newname.GetString(), backtick);

        //..If table name is missing, then add error message in the query
        if(!newname.GetLength())
        {
            newname.SetAs(TABLE_NAME_MISSING);
            rename.AddSprintf("\t\t/* %s */", newname.GetString());
            newname.Clear();
        }
        rename.Add(";");

        //..rename query is not present
        if(m_renamequeryrowno == -1)
        {
            //..If original table name and new table name are different
            if(oldname.Compare(newname) != 0)
            {
                m_renamequeryrowno = SendMessage(m_hwndpreview, SCI_GETLINECOUNT, 0, 0);                      //..Getting the last line number
                
                if(m_renamequeryrowno == 1)     //  No query present in the tab
                    m_renamequeryrowno--;

                if(m_renamequeryrowno > 1)      //  add new-line character if any query is there on the tab
                    SendMessage(m_hwndpreview, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");

                /// Appending rename query in the preview-tab
                SendMessage(m_hwndpreview, SCI_APPENDTEXT, rename.GetLength(), (LPARAM)rename.GetString());
                SendMessage(m_hwndpreview, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");
                SendMessage(m_hwndpreview, SCI_GOTOPOS, (WPARAM)m_renamequeryrowno, 0);

                startpos = SendMessage(m_hwndpreview, SCI_POSITIONFROMLINE, (WPARAM)m_renamequeryrowno, 0);     //..Find the Start position of the last line
                SendMessage(m_hwndpreview, SCI_SETSEL, (WPARAM)startpos, startpos);       //..Select the "Rename table" query
            }
            else
            {
                wyString query;
                GenerateQuery(query);

                if(!query.GetLength())
                {
                    m_istabempty = wyTrue;
                    SetPreviewContent((wyChar*)strnoquery.GetString());
                }
            }
        }
        //..If rename query is present
        else
        {
            //..If original table name and new table name are different
            if(oldname.Compare(newname) != 0)
            {
                //..Select the Rename Query and Replace with new rename query.
                endpos = SendMessage(m_hwndpreview, SCI_GETLINEENDPOSITION, (WPARAM)m_renamequeryrowno, 0);     //..Getting the end position of the last line
                startpos = SendMessage(m_hwndpreview, SCI_POSITIONFROMLINE, (WPARAM)m_renamequeryrowno, 0);     //..Find the Start position of the last line

                SendMessage(m_hwndpreview, SCI_SETSEL, (WPARAM)startpos, endpos);       //..Select the "Rename table" query
                SendMessage(m_hwndpreview, SCI_REPLACESEL, 0, (LPARAM)rename.GetString());
            }
            else
            {
                //..Delete the rename query from the preview window.
                startpos = SendMessage(m_hwndpreview, SCI_POSITIONFROMLINE, (WPARAM)m_renamequeryrowno, 0);     //..Find the Start position of the last line
                SendMessage(m_hwndpreview, SCI_SETCURRENTPOS, (WPARAM) startpos, 0);
                SendMessage(m_hwndpreview, SCI_LINEDELETE, (WPARAM)NULL, (LONG)NULL);       //..Select the "Rename table" query

                /// deleting the previous line(Empty line)
                startpos = SendMessage(m_hwndpreview, SCI_POSITIONFROMLINE, (WPARAM)m_renamequeryrowno-1, 0);     //..Find the Start position of the last line
                SendMessage(m_hwndpreview, SCI_SETCURRENTPOS, (WPARAM) startpos, 0);
                SendMessage(m_hwndpreview, SCI_LINEDELETE, (WPARAM)NULL, (LONG)NULL);       //..Select the "Rename table" query

                /// Resetting the the variable
                m_renamequeryrowno = -1;

                wyString query;
                GenerateQuery(query);

                if(!query.GetLength())
                {
                    m_istabempty = wyTrue;
                    SetPreviewContent((wyChar*)strnoquery.GetString());
                }
            }
        }
    }
    /// resetting the selection in the tab-preview
    SendMessage(m_hwndpreview, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
    SendMessage(m_hwndpreview, SCI_SETSELECTIONEND, (WPARAM)0, 0);

    //..Setting Preview window Read-Only
    SendMessage(m_hwndpreview, SCI_SETREADONLY, true, 0);
}

void
TabPreview::GenerateQuery(wyString &query)
{
    wyString    renamequery(""), tempstr(""), escapestr("");
    wyString    strnoquery;
    
    query.Clear();
    strnoquery.Sprintf("/* %s */", NO_PENDING_QUERIES);

    /// Generating query
    m_ptabmgmt->m_tabinterfaceptr->GenerateQuery(query);
    
    query.RTrim();
    if(query.GetLength() && query.Compare(strnoquery) != 0)
        query.Add(";");

    /// For alter table, generate Drop & Recreate FK query separately and add it to the main query
    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable && m_ptabmgmt->m_tabinterfaceptr->m_isfksupported)
    {
        wyString    **dropcreate = new wyString*[2];
        wyInt32     cntfkgridrows   = 0;
        wyBool      fkretval        = wyFalse;
		wyChar*		backtick;

		//from  .ini file
		backtick = AppendBackQuotes() == wyTrue ? "`" : "";

        dropcreate[0] = new wyString();
        dropcreate[1] = new wyString();

        cntfkgridrows = CustomGrid_GetRowCount(m_ptabmgmt->m_tabfk->m_hgridfk);

        for(int i=0; i<cntfkgridrows; i++)
        {
            fkretval = m_ptabmgmt->m_tabfk->GenerateFKDropRecreateQuery(dropcreate, i);

            if(!fkretval)
                continue;
            if(query.GetLength() || tempstr.GetLength())
                tempstr.Add("\r\n\r\n");

            escapestr.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);
            escapestr.FindAndReplace("`", "``");
            
            tempstr.AddSprintf("Alter table %s%s%s.%s%s%s %s;", 
				backtick, m_ptabmgmt->m_tabinterfaceptr->m_dbname.GetString(), backtick,
				backtick, escapestr.GetString(), backtick,
				dropcreate[0]->GetString());
            
            if(dropcreate[1]->GetLength())
                tempstr.AddSprintf("\r\n\r\nAlter table %s%s%s.%s%s%s %s;", 
					backtick, m_ptabmgmt->m_tabinterfaceptr->m_dbname.GetString(), backtick,
					backtick, escapestr.GetString(), backtick,
					dropcreate[1]->GetString());
        }

        delete dropcreate[0];
        delete dropcreate[1];
        delete dropcreate;
        
        if(tempstr.GetLength())
        {
            query.AddSprintf("%s\r\n\r\n", tempstr.GetString());
        }
    }
    query.RTrim();

    if(query.GetLength())
        query.Add("\r\n");
}

void
TabPreview::GenerateAndSetPreviewContent()
{
    wyString    query(""), renamequery;
    wyString    strnoquery;

    /// Generating query
    GenerateQuery(query);
    strnoquery.Sprintf("/* %s */", NO_PENDING_QUERIES);
    SetPreviewContent((wyChar*)query.GetString());

    query.RTrim();

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
        m_ptabmgmt->m_tabinterfaceptr->GetRenameQuery(renamequery);
    if(renamequery.GetLength())
    {
        m_renamequeryrowno = SendMessage(m_hwndpreview, SCI_GETLINECOUNT, 0, 0);                      //..Getting the last line number

        //..True when no query is there on the scintilla
        if(m_renamequeryrowno == 1)
            m_renamequeryrowno--;

        SendMessage(m_hwndpreview, SCI_SETREADONLY, false, 0);
        
        if(m_renamequeryrowno>1)
            SendMessage(m_hwndpreview, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");

        SendMessage(m_hwndpreview, SCI_APPENDTEXT, renamequery.GetLength(), (LPARAM)renamequery.GetString());
        SendMessage(m_hwndpreview, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");

        SendMessage(m_hwndpreview, SCI_SETSEL, -1, -1);
	    SendMessage(m_hwndpreview, SCI_SETREADONLY, true, 0);
        SendMessage(m_hwndpreview, SCI_GOTOPOS, (WPARAM)0, 0);
        
        SendMessage(m_hwndpreview, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
        SendMessage(m_hwndpreview, SCI_SETSELECTIONEND, (WPARAM)0, 0);
        
    }
    else
        m_renamequeryrowno = -1;

    if((!query.GetLength() || (query.GetLength() && query.Compare(strnoquery) == 0)) && !renamequery.GetLength())
    {
        m_istabempty = wyTrue;
        SetPreviewContent((wyChar*)strnoquery.GetString());
    }
    else
        m_istabempty = wyFalse;
}

wyInt32
TabPreview::OnTabSelChange()
{
    HWND   hwndarr[10] = {m_hwndpreview, NULL};
    
    EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);

    GenerateAndSetPreviewContent();

    ShowWindow(m_hwndpreview, SW_SHOW);
    EnableWindow(m_hwndpreview, TRUE);

    SetFocus(m_hwndpreview);

    return 1;
}

void
TabPreview::SetPreviewContent(wyChar* content)
{
    //..To avoid EN_CHANGE message processing in the TableTabInterfaceTabMgmt.
    m_settingpreviewcontent = wyTrue;

    SendMessage(m_hwndpreview, SCI_SETREADONLY, false, 0);
    SendMessage(m_hwndpreview, SCI_CLEARALL, false, 0);

	//Format query
#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(GetActiveWin(),
		m_hwndpreview, content, ALL_QUERY);
#else
	if (content)
		SendMessage(m_hwndpreview, SCI_SETTEXT, strlen(content), (LPARAM)content);
	else
		SendMessage(m_hwndpreview, SCI_SETTEXT, 0, (LPARAM)"");
	SendMessage(m_hwndpreview, SCI_SETSEL, -1, -1);
	SendMessage(m_hwndpreview, SCI_GOTOPOS, (WPARAM)0, 0);

	Format(m_hwndpreview, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_ALL_QUERY, GetIndentation());

	SendMessage(m_hwndpreview, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
	SendMessage(m_hwndpreview, SCI_SETSELECTIONEND, (WPARAM)0, 0);
#endif

	SendMessage(m_hwndpreview, SCI_SETREADONLY, true, 0);

    m_settingpreviewcontent = wyFalse;
}
