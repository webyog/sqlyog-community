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

#include "TabFields.h"
#include "MDIWindow.h"
#include "CustGrid.h"
#include "Global.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "TableTabInterface.h"
#include "TabForeignKeys.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabIndexes.h"
#include<iostream>

extern PGLOBALS		pGlobals;

IndexedBy::IndexedBy(IndexesStructWrapper* value)
{
    m_pindexwrap = value;
}

ReferencedBy::ReferencedBy(FKStructWrapper* value)
{
    m_pfkwrap = value;
}

FieldStructWrapper::FieldStructWrapper(FIELDATTRIBS *value, wyBool isnew)
{
    if(isnew)
    {
        m_oldval = NULL;
        m_newval = value;
    }
    else
    {
        m_oldval = m_newval = value;
    }
    m_ischanged = wyFalse;
    m_errmsg.Clear();
}

FieldStructWrapper::~FieldStructWrapper()
{
    if(m_newval == m_oldval)
    {
        delete m_newval;
    }
    else
    {
        delete m_newval;
        delete m_oldval;
    }

    //..Deleting List-elements of Index-Working copy
    IndexedBy *indexby = (IndexedBy *)m_listindexesworkingcopy.GetFirst();
    IndexedBy *tmpindby = NULL;
    while(indexby)
    {
        tmpindby = indexby;
        indexby = (IndexedBy *)m_listindexesworkingcopy.Remove(indexby);
        delete tmpindby;
    }

    //..Deleting List-elements of Index-backup copy
    indexby = (IndexedBy *)m_listindexesbackupcopy.GetFirst();
    tmpindby = NULL;
    while(indexby)
    {
        tmpindby = indexby;
        indexby = (IndexedBy *)m_listindexesbackupcopy.Remove(indexby);
        delete tmpindby;
    }

    //..Deleting List-elements of FK-working copy
    ReferencedBy    *refby = (ReferencedBy*)m_listfkworkingcopy.GetFirst();
    ReferencedBy    *tmprefby = NULL;

    while(refby)
    {
        tmprefby = refby;
        refby = (ReferencedBy*)m_listfkworkingcopy.Remove(refby);
        delete tmprefby;
    }

    //..Deleting List-elements of FK-backup copy
    refby = (ReferencedBy*)m_listfkbackupcopy.GetFirst();
    tmprefby = NULL;
    while(refby)
    {
        tmprefby = refby;
        refby = (ReferencedBy*)m_listfkbackupcopy.Remove(refby);
        delete tmprefby;
    }
}

TabFields::TabFields(HWND hwnd, TableTabInterfaceTabMgmt* ptabmgmt)
{
    m_hwnd              =   hwnd;
    m_ptabmgmt          =   ptabmgmt;
    m_hgridfields       =   NULL;
    m_hchkboxhidelanguageoptions    =   NULL;
    m_wporighidelangopt =   NULL;
    m_charsetres        =   NULL;
	
    m_lastclick         =   -1;
    m_autoincrowid      =   -1;
    m_autoinccol.Clear();

    m_mdiwnd            =   GetActiveWin();
    m_ismysql41         =   IsMySQL41(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	m_ismariadb52       =   IsMariaDB52(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
    m_ismysql57			=   IsMySQL57(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
	m_ismysql578        =   IsMySQL578(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
	m_ismariadb10309	=	IsMariaDB10309(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
    m_p = new Persist;
	m_p->Create("TabbedInterface");
	m_isalter = wyFalse;
}

TabFields::~TabFields()
{
    ClearAllMemory();
}

wyInt32
TabFields::GetToolButtonsSize()
{
    DWORD   dword;
    wyInt32 size = 0;

    dword = SendMessage(m_ptabmgmt->m_hwndtool, TB_GETBUTTONSIZE, 0, 0);
    size += HIWORD(dword);

    size *= 5;
    return size;
}

VOID
TabFields::Resize()
{
    RECT			rcmain, rctoolbar;
	wyInt32			hpos, vpos, height, toolwidth = 0, hidelangwidth;
	wyString		tmp;

    VERIFY(GetWindowRect(m_hwnd, &rcmain));
    VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcmain, 2));

    VERIFY(GetWindowRect(m_ptabmgmt->m_hwndtool, &rctoolbar));
    VERIFY(MapWindowPoints(NULL, m_ptabmgmt->m_hwndtool, (LPPOINT)&rctoolbar, 2));
    toolwidth  = rctoolbar.right - rctoolbar.left - 2;


	hidelangwidth = GetTextSize(_(L"Hide language options"), m_hchkboxhidelanguageoptions, (HFONT)GetStockObject(DEFAULT_GUI_FONT)).right;


    if(toolwidth <= (GetToolButtonsSize() + hidelangwidth + 25))
    {
        MoveWindow(m_hchkboxhidelanguageoptions, GetToolButtonsSize() + 20, 2, hidelangwidth + 25, 20, TRUE);
    }
    else
	    MoveWindow(m_hchkboxhidelanguageoptions, rctoolbar.right - (hidelangwidth + 25), 2 , hidelangwidth + 25, 20, TRUE);

    //..Moving Grid
    hpos    = m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog? 2 : 1;
    vpos    = rctoolbar.bottom;  //..25 for "Hide Language Options" checkbox , and toolbar height to be subtracted.
    height  = rcmain.bottom - rcmain.top - rctoolbar.bottom + rctoolbar.top - 2;    
        if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        MoveWindow(m_hgridfields, hpos, vpos, rcmain.right - 3, height, TRUE);
    else
        MoveWindow(m_hgridfields, hpos, vpos, rcmain.right - 2, height, TRUE);
}

wyBool
TabFields::DropColumn(wyUInt32 row)
{
	wyInt32             ret = IDNO, ret2 = IDNO;
    wyString            fieldname, datatypestr;
    IndexedBy           *indexedby = NULL;
    ReferencedBy        *refby = NULL;
    FieldStructWrapper  *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hgridfields, row);

    if(cwrapobj && cwrapobj->m_newval)
    {
        GetGridCellData(m_hgridfields, row, CNAME, fieldname);
        GetGridCellData(m_hgridfields, row, DATATYPE, datatypestr);

        if(fieldname.GetLength() || datatypestr.GetLength() || cwrapobj->m_oldval)
        {
            ret = MessageBox(m_hgridfields, _(L"Do you want to drop this column?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            
            SetFocus(m_hgridfields);
            if(ret != IDYES)
                return wyFalse;
        }
        
        indexedby = (IndexedBy*)cwrapobj->m_listindexesworkingcopy.GetFirst();
        refby = (ReferencedBy*) cwrapobj->m_listfkworkingcopy.GetFirst();

        if(indexedby && !refby)
        {
            ret2 = MessageBox(m_hgridfields, _(L"The selected column is referenced by one or more indexes. Dropping this column will also modify the corresponding indexes.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	        if(ret2 != IDYES)
		        return wyTrue;
        }
        else if(!indexedby && refby)
        {
            ret2 = MessageBox(m_hgridfields, _(L"The selected column is referenced by one or more foreign keys. Dropping this column will also modify the corresponding foreign keys.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	        if(ret2 != IDYES)
		        return wyTrue;
        }
        else if(indexedby && refby)
        {
	        ret2 = MessageBox(m_hgridfields, _(L"The selected column is referenced by one or more indexes and foreign keys. Dropping this column will also modify the corresponding indexes and foreign keys.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	        if(ret2 != IDYES)
		        return wyTrue;
        }

        if(indexedby)
            HandleIndexesOnFieldDelete(cwrapobj);

        if(refby)
            HandleFKsOnFieldDelete(cwrapobj);
        
        ChangeListOnDelete(cwrapobj);
    }
    
    if(m_autoincrowid != -1)
    {
        if(row < m_autoincrowid)
        {
            m_autoincrowid--;
        }
        else if(row == m_autoincrowid)
        {
            m_autoincrowid = -1;
            m_autoinccol.Clear();
        }
    }
    
    CustomGrid_DeleteRow(m_hgridfields, row);

    if(m_lastclick != -1)
        m_lastclick = -1;

    if(CustomGrid_GetRowCount(m_hgridfields))
    {
        if(row == 0)    // if the selected row was first then we just select the first row and column
        {
	        CustomGrid_SetCurSelection(m_hgridfields, 0, 0);
        }
        else
        {
	        CustomGrid_SetCurSelection(m_hgridfields, --row, 0);
        }
    }

    if(ret == IDYES && m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
    {
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    }
    
	return wyTrue;
}

void
TabFields::ChangeListOnDelete(FieldStructWrapper* cwrapobj)
{
    if(!cwrapobj)
        return;

    if(cwrapobj->m_oldval)  //..Existing field
    {
        if(cwrapobj->m_oldval != cwrapobj->m_newval)
        {
            delete cwrapobj->m_newval;
        }
        cwrapobj->m_newval = NULL;
    }
    else
    {
        m_listwrapperstruct.Remove(cwrapobj);
        delete cwrapobj;
    }
}

wyBool	
TabFields::ProcessInsert()
{
    wyUInt32        ret=0;
    wyUInt32        curselrow=0;

    CustomGrid_ApplyChanges(m_hgridfields, wyTrue);

    // First get the current selection
	ret = CustomGrid_GetCurSelection(m_hgridfields);
    
    curselrow = LOWORD(ret);

    if(!curselrow)
        ret=CustomGrid_InsertRowInBetween(m_hgridfields, 0);
    else
	    ret=CustomGrid_InsertRowInBetween(m_hgridfields, curselrow);

    if(ret <= m_autoincrowid)
        m_autoincrowid++;

    CustomGrid_SetCurSelection(m_hgridfields, ret, 0);
    ::SetFocus(m_hgridfields);

    //m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    
	return wyTrue;
}

wyBool	
TabFields::ProcessDelete()
{
    // we delete the selected row but if there are no rows selected we just return.
	wyInt32		selrow, newrow = -1;
    
	// First get the current selection
    selrow = CustomGrid_GetCurSelRow(m_hgridfields);
    
	if(selrow == -1)
		return wyFalse;

	CustomGrid_ApplyChanges(m_hgridfields);
    
    if(DropSelectedColumns() == wyFalse)
    {
        DropColumn(selrow);
    }

    if(CustomGrid_GetRowCount(m_hgridfields) == 0)
        newrow = CustomGrid_InsertRow(m_hgridfields, wyTrue);
    
    selrow = CustomGrid_GetCurSelRow(m_hgridfields);
    
    OnGVNEndRowChange(selrow);
    
    return wyTrue;
}

wyBool
TabFields::DropSelectedColumns()
{
    wyInt32         count, row, ret = IDNO, ret2 = IDNO;
    wyBool          checkrowfound = wyFalse;
    IndexedBy       *indexedby = NULL;
    ReferencedBy    *refby = NULL;

    FieldStructWrapper *cwrapobj = NULL;
    count = CustomGrid_GetRowCount(m_hgridfields);

    for(row=0; row<count; row++)
    {
        if(CustomGrid_GetRowCheckState(m_hgridfields, row))
        {
            cwrapobj = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hgridfields, row);
            checkrowfound = wyTrue;

            if(!cwrapobj || !cwrapobj->m_newval)
            {
                if(m_autoincrowid != -1)
                {
                    if(row < m_autoincrowid)
                    {
                        m_autoincrowid--;
                    }
                    else if(row == m_autoincrowid)
                    {
                        m_autoincrowid = -1;
                        m_autoinccol.Clear();
                    }
                }
                CustomGrid_DeleteRow(m_hgridfields, row, wyTrue);
                
                row--;
                count--;
                continue;
            }
            else if(ret == IDNO)
            {
                ret = MessageBox(m_hgridfields, _(L"Do you want to drop the selected column(s)?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                SetFocus(m_hgridfields);
                if(ret != IDYES)
                    return wyTrue;
            }
            if(ret == IDYES)
            {
                indexedby = (IndexedBy*)cwrapobj->m_listindexesworkingcopy.GetFirst();
                refby = (ReferencedBy*) cwrapobj->m_listfkworkingcopy.GetFirst();
                
                if(ret2 == IDNO && (indexedby && !refby))
                {
                    ret2 = MessageBox(m_hgridfields, _(L"The selected columns are referenced by one or more indexes. Dropping these columns will also modify the corresponding indexes.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if(ret2 != IDYES)
                        return wyTrue;
                }
                else if(ret2 == IDNO && (!indexedby && refby))
                {
                    ret2 = MessageBox(m_hgridfields, _(L"The selected columns are referenced by one or more foreign keys. Dropping these columns will also modify the corresponding foreign keys.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if(ret2 != IDYES)
                        return wyTrue;
                }
                else if(ret2 == IDNO && (indexedby && refby))
                {
                    ret2 = MessageBox(m_hgridfields, _(L"The selected columns are referenced by one or more indexes and foreign keys. Dropping these columns will also modify the corresponding indexes and foreign keys.\r\n\r\nDo you want to continue?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if(ret2 != IDYES)
                        return wyTrue;
                }
                
                if(m_autoincrowid != -1)
                {
                    if(row < m_autoincrowid)
                    {
                        m_autoincrowid--;
                    }
                    else if(row == m_autoincrowid)
                    {
                        m_autoincrowid = -1;
                        m_autoinccol.Clear();
                    }
                }
                
                if(indexedby)
                    HandleIndexesOnFieldDelete(cwrapobj);
                
                if(refby)
		            HandleFKsOnFieldDelete(cwrapobj);
                
                ChangeListOnDelete(cwrapobj);
                
                CustomGrid_DeleteRow(m_hgridfields, row);
                
                row--;
                count--;
            }
        }
    }
    
    if(ret == IDYES)
    {
        if(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_dirtytab != wyTrue)
        {
            m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
        }
    }

    if(checkrowfound && m_lastclick != -1)
        m_lastclick = -1;

    return checkrowfound;
}

void
TabFields::HandleFKsOnFieldRename(FieldStructWrapper* fieldswrapobj)
{
    ReferencedBy *refby = NULL;

    refby = (ReferencedBy*) fieldswrapobj->m_listfkworkingcopy.GetFirst();

    while(refby)
    {
        m_ptabmgmt->m_tabfk->HandleFKsOnFieldRename(refby->m_pfkwrap, fieldswrapobj);
        refby = (ReferencedBy*)refby->m_next;
    }
}

void
TabFields::HandleIndexesOnFieldRename(FieldStructWrapper* fieldswrapobj)
{
    IndexedBy              *indexedby = NULL;

    indexedby = (IndexedBy*)fieldswrapobj->m_listindexesworkingcopy.GetFirst();

    while(indexedby)
    {
        m_ptabmgmt->m_tabindexes->HandleIndexesOnFieldRename(indexedby->m_pindexwrap, fieldswrapobj);
        indexedby = (IndexedBy*)indexedby->m_next;
    }
}

void
TabFields::HandleFKsOnFieldDelete(FieldStructWrapper* fieldswrapobj)
{
    ReferencedBy    *refby = NULL, *tmprefby = NULL;
    
    refby = (ReferencedBy*) fieldswrapobj->m_listfkworkingcopy.GetFirst();

    while(refby)
    {
        m_ptabmgmt->m_tabfk->HandleFKsOnFieldDelete(refby->m_pfkwrap, fieldswrapobj);
        tmprefby = refby;
        refby = (ReferencedBy*)fieldswrapobj->m_listfkworkingcopy.Remove(refby);
        delete tmprefby;
    }
}

void
TabFields::HandleIndexesOnFieldDelete(FieldStructWrapper* fieldswrapobj)
{
    IndexedBy              *indexedby = NULL, *tmpindexby = NULL;

    indexedby = (IndexedBy*)fieldswrapobj->m_listindexesworkingcopy.GetFirst();

    while(indexedby)
    {
        m_ptabmgmt->m_tabindexes->HandleIndexesOnFieldDelete(indexedby->m_pindexwrap, fieldswrapobj);
        tmpindexby = indexedby;
        indexedby = (IndexedBy*) fieldswrapobj->m_listindexesworkingcopy.Remove(indexedby);
        delete tmpindexby;
    }
}

wyBool
TabFields::Create()
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;

    //..Create Checkbox "Hide Language Options"
    if(m_ismysql41)
    {
        m_hchkboxhidelanguageoptions = CreateWindowEx(0, WC_BUTTON, _(TEXT("Hide language options")), 
                                            style | BS_AUTOCHECKBOX | WS_GROUP | WS_TABSTOP, 0, 0, 0, 0, m_ptabmgmt->m_hwndtool, (HMENU) IDC_HIDECOLUMNS,
										    (HINSTANCE)pGlobals->m_hinstance, NULL);
        
        if(m_hchkboxhidelanguageoptions)
        {
            ShowWindow(m_hchkboxhidelanguageoptions, SW_SHOW);
            UpdateWindow(m_hchkboxhidelanguageoptions);
            SendMessage(m_hchkboxhidelanguageoptions, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        }
        if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        {
            m_wporighidelangopt = (WNDPROC)SetWindowLongPtr(m_hchkboxhidelanguageoptions, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
            SetWindowLongPtr(m_hchkboxhidelanguageoptions, GWLP_USERDATA, (LONG_PTR)m_ptabmgmt->m_tabinterfaceptr);
        }
        m_p->Add(m_ptabmgmt->m_hwndtool, IDC_HIDECOLUMNS, "HideCharsetAndCollation", "1", CHECKBOX);
    }

    //..Create Grid
    CreateGrid(m_hwnd);

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        /// Fetching and setting column values
        if(!SetInitValues())
            return wyFalse;
    }

    return wyTrue;
}

wyInt32
TabFields::OnTabSelChanging()
{
    /// App;ying grid changes 
    CustomGrid_ApplyChanges(m_hgridfields, wyTrue);
    
    /// Validating all grid values
    return ValidateFields(wyFalse);
}

wyInt32 
TabFields::OnTabSelChange()
{
    /// An array that contains the handles that only should be visible
    HWND   hwndarr[10] = {m_hgridfields, m_ptabmgmt->m_hwndtool, m_hchkboxhidelanguageoptions, NULL};

    /// Making windows visible
    EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);

    SetFocus(m_hgridfields);
    
    /// Unhiding toolbar buttons (Up/Down and Seperator)
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDM_SEPARATOR, (LPARAM)MAKELONG(FALSE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(FALSE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(FALSE,0));

    return 1;
}

void
TabFields::HandleDefaults(MYSQL_ROW myfieldrow, MYSQL_RES *myfieldres, wyString &datatype, wyInt32 &ret, wyString& defaultstr)
{
    wyInt32 ind = -1;

	if((ind = GetFieldIndex(myfieldres, "default", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))) != -1 && myfieldrow[ind])
	{
		defaultstr.SetAs(myfieldrow[ind], m_ismysql41);
        if(defaultstr.GetLength() == 0)
        {
            if(myfieldrow[GetFieldIndex(myfieldres, "null", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))])
		    {
                if(strstr(datatype.GetString(), "varchar") != NULL || strstr(datatype.GetString(), "char") != NULL || 
                    strstr(datatype.GetString(), "varbinary") != NULL || strstr(datatype.GetString(), "set") != NULL 
                    || strstr(datatype.GetString(), "enum") != NULL || strstr(datatype.GetString(), "binary") != NULL)
                {
                    defaultstr.SetAs("''");
                }
            }
		}
	}
}

wyBool 
TabFields::TraverseEachFieldRow(MYSQL_RES *myfieldres,wyString createtable)
{
    wyInt32			ret, tempcount, count, fieldno = 0, index = 0, addcol = 0;
    wyInt32         typeindex, fieldindex, charindex;
    wyInt32         i = -1;
    FIELDATTRIBS    *fieldattr = NULL;
    FieldStructWrapper *cwrapobj = NULL,*cwrapobj2 = NULL;
    wyChar          *tempstr = NULL,*currentrowstr=NULL,*wholecreatestring=NULL;
	wyString        strcreate, query, expressionvalue, checkexpression;
    MYSQL_ROW		myfieldrow;
	wyString		rowname, datatype;
	wyString		myrowstr, isnullstr, defaultstr;
	wyString		mykeyrowstr;
	wyString		mykeyrow1str;
	wyString		myfieldrowstr;
	
    wyBool          flag = wyTrue;

    if(m_ismysql41)
		index = 1;
    else
        addcol = 1;
	if(m_ismariadb52||m_ismysql57)
	{
	wholecreatestring = (wyChar*)strdup(createtable.GetString());
	if(wholecreatestring)
	currentrowstr=strtok(wholecreatestring,"\n");
	if(currentrowstr)
	currentrowstr=strtok(NULL,"\n");
	}
	/// Extracting and storing all column values
	while(myfieldrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myfieldres))
	{
        fieldattr = new FIELDATTRIBS;
        InitFieldAttribs(fieldattr);
        
        if(myfieldrow[0])
            fieldattr->m_name.SetAs(myfieldrow[0], m_ismysql41);
        
        typeindex = GetFieldIndex(myfieldres, "type", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
		
		// now get the datatype but first alloc the buffer.
		if(typeindex != -1 && myfieldrow[typeindex])
		{
			myfieldrowstr.SetAs(myfieldrow[typeindex], m_ismysql41);
			VERIFY(tempstr =(wyChar*)calloc(sizeof(wyChar), myfieldrowstr.GetLength() + 2));
		}

        /// Setting datatype value
		count = 0;
		while(myfieldrow[typeindex][count] && myfieldrow[typeindex][count] != '(' && myfieldrow[typeindex][count] != ' ')
		{
			tempstr[count] = *(myfieldrow[1] + count);
			count++;
		}
		tempstr[count] = NULL;
        fieldattr->m_datatype.SetAs(tempstr);
        datatype.SetAs(fieldattr->m_datatype);

        // for 6.06
        HandleDefaults(myfieldrow, myfieldres, datatype, ret, fieldattr->m_default);
        /*
		if((stricmp(datatype.GetString(), "timestamp")== 0)&& (CheckForOnUpdate(strcreate, fieldno)))
			fieldattr->m_onupdate = wyTrue;
		else
			fieldattr->m_onupdate = wyFalse;
            */
		if(index)
		{
			fieldindex = GetFieldIndex(myfieldres, "collation", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
			if(fieldindex != -1 && myfieldrow[fieldindex] && stricmp(myfieldrow[fieldindex], "NULL") != 0)
			{
				myrowstr.SetAs(myfieldrow[fieldindex], m_ismysql41);

                fieldattr->m_collation.SetAs(myrowstr.GetString());

                //we extract the charset by collation info of the table
                //for eg, x_y is the collation then before "_" is the charset
                charindex = myrowstr.Find("_", 0);
                myrowstr.Strip(myrowstr.GetLength() - charindex);
                fieldattr->m_charset.SetAs(myrowstr);
			}
		}
		
		myrowstr.SetAs(myfieldrow[1], m_ismysql41);

		if (strstr(myrowstr.GetString(), "enum"))
		{
			fieldattr->m_unsigned = wyFalse;
		}
		else if(strstr(myrowstr.GetString(), "unsigned"))
            fieldattr->m_unsigned = wyTrue;

		if(m_ismysql41 == wyFalse)
			if(strstr(myrowstr.GetString(), "binary"))
                fieldattr->m_binary = wyTrue;
		
        /// Zerofill value
		if(strstr(myrowstr.GetString(), "zerofill"))
            fieldattr->m_zerofill = wyTrue;
		
        /// Comment value
		if(m_ismysql41 && (i = GetFieldIndex(myfieldres, "comment", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))) != -1 &&
            myfieldrow[i])
		{
            fieldattr->m_comment.SetAs(myfieldrow[i], m_ismysql41);
		}

        /// NULL
		if((i = GetFieldIndex(myfieldres, "null", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))) != -1 && myfieldrow[i])
		{
			myrowstr.SetAs(myfieldrow[i], m_ismysql41);
			if(!(strstr(myrowstr.GetString(), "YES")))
                fieldattr->m_notnull = wyTrue;
		}

        /// Auto_Incr value
		if((i = GetFieldIndex(myfieldres, "extra", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))) != -1 && myfieldrow[i])
		{
			myrowstr.SetAs(myfieldrow[i], m_ismysql41);
			if(strstr(myrowstr.GetString(), "auto_increment"))
			{
                fieldattr->m_autoincr = wyTrue;
				m_autoinccheck = wyTrue;
				m_autoinccol.SetAs(myfieldrow[CNAME], m_ismysql41);
			}
			//adding option for virtaul/persistent columns
			if(m_ismariadb52)
			{
			if(strstr(myrowstr.GetString(), "VIRTUAL"))
			{
				fieldattr->m_virtuality.SetAs("VIRTUAL");
				//for expression value we have to parse createtable string
				if(expressionvalue.GetLength()!=0)
					expressionvalue.Clear();
				if(GetExpressionValue(currentrowstr,&expressionvalue))
				{
					fieldattr->m_expression.SetAs(expressionvalue.GetString());
				
				}
			}
			else if(strstr(myrowstr.GetString(), "STORED"))
			{
				fieldattr->m_virtuality.SetAs("STORED");
				if(expressionvalue.GetLength()!=0)
					expressionvalue.Clear();
				if(GetExpressionValue(currentrowstr,&expressionvalue))
				{
					fieldattr->m_expression.SetAs(expressionvalue.GetString());
				
				}
			}
			else
			{
			fieldattr->m_virtuality.SetAs("(none)");
			}
			}

			//adding option for virtaul/persistent columns
			else if(m_ismysql57)
			{
			if(strstr(myrowstr.GetString(), "VIRTUAL"))
			{
				fieldattr->m_mysqlvirtuality.SetAs("VIRTUAL");
				//for expression value we have to parse createtable string
				if(expressionvalue.GetLength()!=0)
					expressionvalue.Clear();
				if(GetExpressionValue(currentrowstr,&expressionvalue))
				{
					fieldattr->m_mysqlexpression.SetAs(expressionvalue.GetString());
				
				}
			}
			else if(strstr(myrowstr.GetString(), "STORED"))
			{
				fieldattr->m_mysqlvirtuality.SetAs("STORED");
				if(expressionvalue.GetLength()!=0)
					expressionvalue.Clear();
				if(GetExpressionValue(currentrowstr,&expressionvalue))
				{
					fieldattr->m_mysqlexpression.SetAs(expressionvalue.GetString());
				
				}
			}
			else
			{
			fieldattr->m_mysqlvirtuality.SetAs("(none)");
			}
			}
			//also check for datetime - 5.6.5
			// on update cannont be true for other datatypes!
            //if((datatype.CompareI("timestamp")== 0 || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql))) && myrowstr.FindI("on update CURRENT_TIMESTAMP") != -1)
            if((datatype.CompareI("timestamp")== 0 || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql))) && myrowstr.FindI("on update CURRENT_TIMESTAMP") != -1)
			{
                fieldattr->m_onupdate = wyTrue;
            }
		}
		// we have to parse createtable string for check constraint expression
		//if (m_ismariadb52)
		//{
			if (checkexpression.GetLength() != 0)
				checkexpression.Clear();
			if (GetCheckConstraintValue(currentrowstr, &checkexpression))
			{
				fieldattr->m_mycheckexpression.SetAs(checkexpression.GetString());

			}
		//}
		
        /// Extracting the len value
        tempstr[0] = NULL;
        if(myfieldrow[typeindex][count] != NULL)
        {
            if(myfieldrow[typeindex][count] == '(')
            {
                count++;
                tempcount = 0;
                while(myfieldrow[typeindex][count])
                {
                    if(myfieldrow[typeindex][count] == ')' && flag == wyTrue)
                        break;

                    tempstr[tempcount] = *(myfieldrow[1] + count);

                    if(*(myfieldrow[1] + count) == '\'')
                    {
                        if(flag == wyTrue)
                            flag = wyFalse;
                        else
                            flag = wyTrue;
                    }
                    tempcount++;
                    count++;
                }
                tempstr[tempcount] = NULL;
                if(tempcount && tempstr)
                    fieldattr->m_len.SetAs(tempstr, m_ismysql41);
            }
        }

		// dealloc
		if(tempstr)
			free(tempstr);
		
		tempstr = NULL;
		fieldno++;

        /// Creating the wrapper instance and storing it the the class-member variable
        cwrapobj = new FieldStructWrapper(fieldattr, wyFalse);
		cwrapobj2 = new FieldStructWrapper(fieldattr, wyFalse);
        m_listwrapperstruct.Insert(cwrapobj);
		m_listwrapperstruct_2.Insert(cwrapobj2);
		if((m_ismysql57||m_ismariadb52) && currentrowstr)
		currentrowstr=strtok(NULL,"\n");
	}
    return wyTrue;
}
//function to get expression value from create table statement added in 12.05
//void
//TabFields::setExpression(wyString *atrrinbute, wyString *createstring)
//{
//wyInt32			regexret = 0;
//SQLFormatter    formatter;
//wyString		pattern, res,expression;
////pattern to find out statring of expression
//pattern.SetAs("AS[\\s]*\\(");
////pattern.SetAs("AS");
//
////this step will five the substring string from pattern
//regexret = formatter.MatchStringAndGetResult(createstring->GetString(),
//					(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
//					wyTrue, wyTrue);
////now we will get the current expression of virtual/persistent cloumn
//if(regexret == -1)
//		return;
//
//else
//{
//int i=0;
//wyChar value;
////copy everything after AS and befrore VIRTUAL/PERSISTENT in attribute.
//while(((value=res.GetCharAt(i))!='V')&&(res.GetCharAt(i)!='P'))
//{
//expression.SetCharAt(i,value);
//
//i++;
//}
//atrrinbute->SetAs(expression);
////now we don't need current expression, we will look into remaining part of create table statement
//createstring->SetAs(res.GetString()+i);
//}    
//
//
//}
wyBool
TabFields::FetchInitData()
{
	MYSQL_RES		*myfieldres,*myfieldresmariadb,*myfieldresmysql;
	MYSQL_ROW		myfieldrow;
	wyString        strcreate, query;
	wyString        tblname(""), dbname("");

    tblname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);
    tblname.FindAndReplace("`", "``");
    
    dbname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_dbname);
    dbname.FindAndReplace("`", "``");
    
	// get the field information.
    query.Sprintf("show full fields from `%s`.`%s`", dbname.GetString(), tblname.GetString());
	
    myfieldres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
	
    if(!myfieldres)
    {
	    ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
    }
	//getting expression for virtual/persistent coloumn
	if(m_ismariadb52)
	{
		query.Clear();
		query.Sprintf("show create table `%s`.`%s`",dbname.GetString(), tblname.GetString());
		myfieldresmariadb = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
	
    if(!myfieldresmariadb)
    {
	    ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
    }
	else
	{
	myfieldrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myfieldresmariadb);

	TraverseEachFieldRow(myfieldres,myfieldrow[1]);	
	 m_mdiwnd->m_tunnel->mysql_free_result(myfieldresmariadb);
	}
	
	
	
	}

	else if(m_ismysql57)
	{
		query.Clear();
		query.Sprintf("show create table `%s`.`%s`",dbname.GetString(), tblname.GetString());
		myfieldresmysql = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
	
    if(!myfieldresmysql)
    {
	    ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
    }
	else
	{
	myfieldrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myfieldresmysql);

	TraverseEachFieldRow(myfieldres,myfieldrow[1]);	
	 m_mdiwnd->m_tunnel->mysql_free_result(myfieldresmysql);
	}
	
	
	
	}
    /// Extracting all field values
	else
	{
    TraverseEachFieldRow(myfieldres,"");
	}
	m_mdiwnd->m_tunnel->mysql_free_result(myfieldres);
   
	return wyTrue;
}

void 
TabFields::InitFieldAttribs(FIELDATTRIBS* value)
{
    /// Initializing all attribute values
    value->m_pk = wyFalse;
    value->m_binary = wyFalse;
    value->m_notnull = wyFalse;
    value->m_unsigned = wyFalse;
    value->m_autoincr = wyFalse;
    value->m_zerofill = wyFalse;
    value->m_onupdate =   wyFalse;

    value->m_next = NULL;
}

wyBool
TabFields::SetInitValues()
{
    /// Fetching init values
    if(FetchInitData() == wyFalse)
        return wyFalse;

    /// Filling fetched column values
    FillInitData();

    return wyTrue;
}

wyBool
TabFields::FillInitData()
{
    FIELDATTRIBS	*temp = NULL;
    wyInt32         rowno = -1, index = 0, addcol = 0;
    FieldStructWrapper* cwrapobj = NULL;

    if(m_ismysql41)
		index = 1;
    else
        addcol = 1;

    cwrapobj = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();

    /// Rotating through all wrappers and filling all values
    while(cwrapobj)
    {
        temp = cwrapobj->m_oldval;

        rowno = CustomGrid_InsertRow(m_hgridfields, wyTrue);
        CustomGrid_SetText(m_hgridfields, rowno, CNAME, temp->m_name.GetString());

        CustomGrid_SetText(m_hgridfields, rowno, DATATYPE, temp->m_datatype.GetString());
        SetValidation(rowno, (wyChar*) temp->m_datatype.GetString());

        CustomGrid_SetText(m_hgridfields, rowno, LENGTH, temp->m_len.GetString());
        CustomGrid_SetText(m_hgridfields, rowno, DEFVALUE, temp->m_default.GetString());
		CustomGrid_SetText(m_hgridfields, rowno, CHECKCONSTRAINT, temp->m_mycheckexpression.GetString());

        if(index)
        {
            CustomGrid_SetText(m_hgridfields, rowno, COLLATION, temp->m_collation.GetString());
            CustomGrid_SetText(m_hgridfields, rowno, CHARSET, temp->m_charset.GetString());
        }
		//filling virtual/Persistent combo box.
		if(m_ismariadb52)
		{
		CustomGrid_SetText(m_hgridfields, rowno, VIRTUALITY , temp->m_virtuality.GetString());
        SetValidation(rowno, (wyChar*) temp->m_virtuality.GetString());
		CustomGrid_SetText(m_hgridfields, rowno, EXPRESSION , temp->m_expression.GetString());
		
		}
		//filling virtual/Stored combo box.
		else if(m_ismysql57)
		{
		CustomGrid_SetText(m_hgridfields, rowno, VIRTUALITY , temp->m_mysqlvirtuality.GetString());
        SetValidation(rowno, (wyChar*) temp->m_mysqlvirtuality.GetString());
		CustomGrid_SetText(m_hgridfields, rowno, EXPRESSION , temp->m_mysqlexpression.GetString());
		}
        CustomGrid_SetBoolValue(m_hgridfields, rowno, UNSIGNED + addcol, temp->m_unsigned ? GV_TRUE : GV_FALSE);
        CustomGrid_SetBoolValue(m_hgridfields, rowno, BINARY, temp->m_binary ? GV_TRUE : GV_FALSE);
        CustomGrid_SetBoolValue(m_hgridfields, rowno, ZEROFILL + addcol, temp->m_zerofill ? GV_TRUE : GV_FALSE);
        CustomGrid_SetText(m_hgridfields, rowno, COMMENT_, temp->m_comment.GetString());
        CustomGrid_SetBoolValue(m_hgridfields, rowno, NOTNULL + addcol, temp->m_notnull ? GV_TRUE : GV_FALSE);
        CustomGrid_SetBoolValue(m_hgridfields, rowno, AUTOINCR + addcol, temp->m_autoincr ? GV_TRUE : GV_FALSE);
		CustomGrid_SetBoolValue(m_hgridfields, rowno, ONUPDATECT, temp->m_onupdate ? GV_TRUE : GV_FALSE);

        if(temp->m_autoincr)
        {
            m_autoincrowid = rowno;
            m_autoinccol.SetAs(temp->m_name);
        }

        CustomGrid_SetBoolValue(m_hgridfields, rowno, PRIMARY, temp->m_pk ? GV_TRUE : GV_FALSE);
        CustomGrid_SetColumnReadOnly(m_hgridfields, rowno, NOTNULL + addcol, temp->m_pk);
        CustomGrid_SetRowLongData(m_hgridfields, rowno, (LPARAM)cwrapobj);
        cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
    }

    CustomGrid_InsertRow(m_hgridfields, wyTrue);    //..We shall add new row with NO "ROWLONGDATA"
    CustomGrid_SetCurSelection(m_hgridfields, 0, 0, wyTrue);
    return wyTrue;
}

void
TabFields::CreateGrid(HWND hwnd)
{
    /// Creating grid
    m_hgridfields = CreateCustomGridEx(hwnd, 0, 0, 0, 0, (GVWNDPROC)GridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);
    
    /// Initializing grid
    InitGrid();
    
    /// showing/hiding language options
    ShowHideCharsetAndCollation();

    ShowWindow(m_hgridfields, SW_SHOW);
    UpdateWindow(m_hgridfields);

    return;
}

wyBool
TabFields::InitGrid()
{
    wyInt32			counter;		// normal counter
	wyInt32			num_cols;		// number of columns
	GVCOLUMN		gvcol;			// structure used to create columns for grid
	//wyWChar	 		(*type)[33][20]; 
	wyWChar			type[33][20] =	{	L"tinyint", L"smallint", L"mediumint", L"int", L"bigint", L"real", L"bit", L"bool", L"boolean",
										L"float", L"double", L"decimal", L"date", L"datetime", L"timestamp", L"numeric",  L"time", L"year", L"char", 
										L"varchar", L"tinyblob", L"tinytext", L"text",L"blob" ,L"mediumblob", L"mediumtext", L"longblob", L"longtext", 
										L"enum", L"set", L"binary", L"varbinary" };
	if(m_ismysql578)
	{
		type[32][0]='j';
		type[32][1]='s';
		type[32][2]='o';
		type[32][3]='n';
	}

	
	wyWChar virtuallity[3][20]= {  L"(none)", L"VIRTUAL",NULL};
	if (m_ismariadb10309)
		mbstowcs(virtuallity[2], "STORED", strlen("STORED") + 1);
	else if(m_ismariadb52)
		mbstowcs (virtuallity[2], "PERSISTENT", strlen("PERSISTENT")+1);
	else 
		mbstowcs (virtuallity[2], "STORED", strlen("STORED")+1);

	wyChar		    *heading[] = {_("Column Name"), _("Data Type"), _("Length"), _("Default"),  _("PK?"), _("Binary?"), _("Not Null?"), 
		                          _("Unsigned?"), _("Auto Incr?"), _("Zerofill?"), _("Charset"), _("Collation"), _("On Update"), _("Comment"),  _("Virtuality"),_("Expression"),_("Check Constraint")};
	
	wyInt32			mask[] = {GVIF_TEXT, GVIF_LIST, GVIF_TEXT, GVIF_TEXT,  GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, 
		                      GVIF_BOOL, GVIF_BOOL, GVIF_LIST, GVIF_LIST, GVIF_BOOL, GVIF_TEXT,GVIF_LIST,GVIF_TEXT,GVIF_TEXT };
	
    wyInt32			cx[] = { 125, 95, 40, 80, 40, 60, 70, 70, 75, 70,  90, 140, 80, 150,20,95,140 };//default/min col width

	wyInt32			format[] = { GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, 
		                         GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_LEFT, GVIF_CENTER,GVIF_LEFT,GVIF_LEFT,GVIF_LEFT };

	wyInt32			width = 0;
	wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_table");
	HFONT			hfont;
    m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hgridfields);
	hfont = CustomGrid_GetColumnFont(m_hgridfields);

	cx[1] = GetTextSize(L"mediumblob ", m_hgridfields, hfont).right + 15; //default min size for datatype col
	num_cols = sizeof (heading)/sizeof(heading[0]);
	
	VOID		    *listtype[] = {NULL, (VOID*)type, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (void*)virtuallity,NULL,NULL};
	wyInt32			elemsize[] = {  0, sizeof(type[0]), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,sizeof(virtuallity[0]),0,0};
	wyInt32			elemcount[] = {0, sizeof(type)/sizeof(type[0]), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, sizeof(virtuallity)/sizeof(virtuallity[0]),0,0};
	
	if(! m_ismysql578)   // to avoid extra counting of the new datatype json
		elemcount[1]=elemcount[1]-1;
	
	for (counter=0; counter < num_cols ; counter++ )
    {
        	//for getting the retained column width
			colname.SetAs(heading[counter]);
            /*if(counter>= 4 && counter<=9)*/
			width = GetTextSize(colname.GetAsWideChar(), m_hgridfields, hfont).right + 5;
           /* else
                width = 0;*/
			//width = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		
		memset(&gvcol, 0,sizeof(gvcol));
		
		gvcol.mask			= mask[counter];		// Set the mask for the sturcture  i.e. what kind of column in the grid.
		gvcol.fmt			= format[counter];		// Alignment
		gvcol.pszList		= listtype[counter];	// set the kind list in between
		gvcol.cx			= (width < cx[counter])? cx[counter]:width;
		gvcol.text			= heading[counter];
		gvcol.nElemSize		= elemsize[counter];
		gvcol.nListCount	= elemcount[counter];
		gvcol.cchTextMax	= strlen(heading[counter]);
        //gvcol.uIsButtonVis

		// if the column being entered is the collation column and the mysql version is < 4.1, then
		// we skip this column as column level collation is not present in v < 4.1
		if ((counter == COLLATION+1 || counter == CHARSET+1) && !m_ismysql41)
        {
			continue;
        }
		if((counter ==  VIRTUALITY +1 || counter == EXPRESSION+1) && !m_ismariadb52 && !m_ismysql57)
		{
		continue;
		}
		// if the column being entered is last and the mysql version is < 4.1, then
		// we exit as the last column is for column level comments which is only supported
		// in v4.1
		if((counter==(num_cols-1)) && (m_ismysql41) == 0)
        {
			break;
        }

		if((m_ismysql41 == wyTrue) && (stricmp(heading[counter] , _("Binary?")) == 0))
        {
			continue;
        }
		CustomGrid_InsertColumn(m_hgridfields, &gvcol);
    
    }
	if(!((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_isaltertable)
    {
	    CreateInitRows();
    }
	
	return wyTrue;
}

void
TabFields::CreateInitRows()
{
	wyUInt32		i;					// just a counter
	wyUInt32		rowperpage;			// how many rows can be displayed in the page

	rowperpage = CustomGrid_RowPerPage(m_hgridfields);

	for(i=0; i< DEFAULTNUMROWS; i++)
    {
		CustomGrid_InsertRow(m_hgridfields);
    }
}

wyBool
TabFields::GenerateQuery(wyString& query)
{
    wyString    str("");
    wyBool      retval = wyTrue;

	// We always read here so that we can get current value not cached one
	//Read the .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    /// Generating query
    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        /// Generating Alter table query
        retval = GenerateAlterQuery(str);
    }
    else
    {
        /// Generating Crate table query
        retval = GenerateCreateQuery(str);
    }

    if((!m_ptabmgmt->m_tabinterfaceptr->m_isaltertable) && !str.GetLength())
    {
        return wyFalse;
    }

    if(str.GetLength())
        query.AddSprintf(" %s", str.GetString());
    return wyTrue;    
}

wyBool
TabFields::GenerateCreateQuery(wyString& str)
{
    FIELDATTRIBS*   pfieldattribs = NULL;
    wyString        query;
    wyString        lasterrormsg("");
    TableTabInterface*  tabint;
    FieldStructWrapper* cwrapobj = NULL;

    tabint = (TableTabInterface*) m_ptabmgmt->m_tabinterfaceptr;
    
    cwrapobj = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();

    if(!cwrapobj)
        return wyFalse;

    while(cwrapobj)
    {
        query.Add("\r\n  ");
        pfieldattribs = cwrapobj->m_newval;
        lasterrormsg.Clear();
        
        /// Concatinating column name and datatype into query
        GetColumnAndDataType(query, pfieldattribs);
        
        if(m_ismysql41)
        {
            /// Concatinating charset and collation into query
            GetCharsetAndCollationValue(query, pfieldattribs);
        }

        /// concatinating extra and default values
        GetExtraValues(query, pfieldattribs);
        GetDefaultValue(query, cwrapobj);
		GetVirtualOrPersistentValue( query, cwrapobj);
		

        if(m_ismysql41)
        {
            GetCommentValue(query,pfieldattribs);
        }
		GetCheckValue(query, pfieldattribs);
        query.RTrim();
        query.Add(",");

        if(cwrapobj->m_errmsg.GetLength())
        {
            /// concatinating error message into query
            lasterrormsg.SetAs(cwrapobj->m_errmsg);
            query.AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg.GetString());
        }
        
        cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
    }
	query.RTrim();
    
    /// Checking if any column is auto-increment and not-indexed? If yes, add as a Key
    if(m_autoincrowid != -1)
    {
        cwrapobj = GetWrapperObjectPointer(m_autoinccol);
        if(cwrapobj)
        {
            IndexedBy* indby = NULL;
            IndexColumn *indcol = NULL;

            indby = (IndexedBy*)cwrapobj->m_listindexesworkingcopy.GetFirst();

            /// Check whether the column is indexed or not.
            if(indby)
            {
                if(indby->m_pindexwrap->m_newval->m_listcolumns)
                    indcol = (IndexColumn *)indby->m_pindexwrap->m_newval->m_listcolumns->GetFirst();
                
                /// If not the first column of the index, add a new index
                if(indcol->m_pcwrapobj != cwrapobj)
                    query.AddSprintf("\r\n   Key(%s%s%s),", m_backtick, m_autoinccol.GetString(), m_backtick);
            }
            else
                query.AddSprintf("\r\n   Key(%s%s%s),", m_backtick, m_autoinccol.GetString(),m_backtick);
        }
    }

	str.Add(query.GetString());
    return wyTrue;
}

wyInt32
TabFields::GetRowNoForWrapper(FieldStructWrapper* pwrapobj)
{
    wyUInt32 row, count = CustomGrid_GetRowCount(m_hgridfields);

    for(row =0; row < count; row++)
    {
        if(pwrapobj == ((FieldStructWrapper*)CustomGrid_GetRowLongData(m_hgridfields, row)))
            return row;
    }
    return -1;
}

wyBool
TabFields::GenerateAlterQuery(wyString& query)
{
    wyString		modifyquery, completequery, dropcols;
	wyString		autoincrquery;
    wyString        dbname, tablename;
	IndexedBy       *indby = NULL;
	wyInt32			retvalue;
    IndexColumn     *icols = NULL;
    wyBool          indexedfirst = wyFalse;

	//To avoid extra parenthesis around check constraint extraint
	m_isalter = wyTrue;

    dbname.SetAs(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_dbname);
    tablename.SetAs(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_origtblname);

    /// Generating query for new and modified columns
    GetNewAndModifiedColumns(modifyquery);
    
    /// Generating query for dropped columns
	retvalue = DropColumnQuery(dropcols);
	if(retvalue == -1)
		return wyFalse;

    if(!(modifyquery.GetLength() || dropcols.GetLength()))
        return wyTrue;
    
	// prepare the query.
    completequery.Add((dropcols.GetLength())?(dropcols.GetString()):(""));
    completequery.Add((modifyquery.GetLength())?(modifyquery.GetString()):(""));
	
    completequery.RTrim();

    if(m_autoincrowid != -1)
    {
        FieldStructWrapper *cwrapobj = NULL;

        if(m_autoinccol.GetLength())
        {
            /// Checking whether auto-increment requires index(key) or not
            cwrapobj = (FieldStructWrapper*) GetWrapperObjectPointer(m_autoinccol);
            if(cwrapobj)
            {
                if((!cwrapobj->m_oldval && cwrapobj->m_newval->m_autoincr) || (cwrapobj->m_oldval && cwrapobj->m_newval && (!cwrapobj->m_oldval->m_autoincr) && (cwrapobj->m_newval->m_autoincr)))
                {
                    indby = (IndexedBy *)cwrapobj->m_listindexesworkingcopy.GetFirst();
                    while(indby)
                    {
                        icols = (IndexColumn*) indby->m_pindexwrap->m_newval->m_listcolumns->GetFirst();
                        if(icols->m_pcwrapobj == cwrapobj)
                        {
                            indexedfirst = wyTrue;
                            break;
                        }
                        indby = (IndexedBy *)indby->m_next;
                    }
                    if(!indexedfirst)
                        completequery.AddSprintf("\r\n\tadd Key(%s%s%s),", m_backtick, m_autoinccol.GetString(), m_backtick);
                }
            }
        }
    }

	query.SetAs(completequery);
	return wyTrue;
}

// create the drop column query.
wyInt32
TabFields::DropColumnQuery(wyString &dropcolumns)
{
    FieldStructWrapper* cwrapobj = NULL;
    dropcolumns.Clear();

    cwrapobj = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        if(cwrapobj->m_oldval && !(cwrapobj->m_newval))
        {
            dropcolumns.AddSprintf("\r\n\tdrop column %s%s%s, ", m_backtick, cwrapobj->m_oldval->m_name.GetString(), m_backtick);
        }
        cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
    }

    return 0;
}

void
TabFields::GetNewAndModifiedColumns(wyString &columnsstr)
{
    FieldStructWrapper* cwrapobj = NULL, *prevoldcwrap = NULL, *prevcwrap = NULL, *cwrapobjtemp = NULL;
    FieldStructWrapper* past_ele = NULL,*past_ele_prev = NULL,*present_ele_prev = NULL;
    wyUInt32        count, row;
	wyInt32			row_temp;
    wyString        defvalue("");
    wyString        coldef("");
    wyString        reorderstr(""), lasterrmsg("");
	wyBool			isremove = wyTrue;
    count = CustomGrid_GetRowCount(m_hgridfields);
	//if a column was removed then remove it from past list
	past_ele = (FieldStructWrapper*) m_listwrapperstruct_2.GetFirst();
	while(past_ele != NULL)
	{
		isremove = wyTrue;
		for(row = 0; row < count; row++)
		{
			cwrapobj = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);
			if(!cwrapobj)
				continue;

			if(!cwrapobj->m_newval)
				continue;

			if(cwrapobj->m_oldval == past_ele->m_oldval)
			{
				isremove = wyFalse;
				break;
			}
		}
		if(isremove)
			m_listwrapperstruct_2.Remove(past_ele);

		past_ele = (FieldStructWrapper*)past_ele->m_next;
	}
    past_ele = NULL;

    for(row = 0; row < count; row++)
    {
        cwrapobj = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);

        if(!cwrapobj)
            continue;

        if(!cwrapobj->m_newval)
            continue;

		//after first iteration move to  the next element in the list
		if(cwrapobj->m_oldval)
		{
			if( past_ele == NULL)
				past_ele = (FieldStructWrapper*) m_listwrapperstruct_2.GetFirst();
			else
				past_ele = (FieldStructWrapper*)past_ele->m_next;
		}


        reorderstr.Clear();

        if(!cwrapobj->m_oldval)     //..if field is newly added
        {
            GetColumnDefinition(cwrapobj, columnsstr, wyTrue);
            
            //..Finds position of the new field
            if(row == 0 || !prevcwrap)
            {
                columnsstr.Add(" first");
            }
            else
                columnsstr.AddSprintf(" after %s%s%s", m_backtick, prevcwrap->m_newval->m_name.GetString(), m_backtick);
            columnsstr.Add(",");

            lasterrmsg.SetAs(cwrapobj->m_errmsg);

            if(cwrapobj->m_errmsg.GetLength())
                columnsstr.AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg.GetString());
        }
        else if(cwrapobj->m_newval)
        {
            coldef.Clear();
            
            /// Getting the column defintion
			
            GetColumnDefinition(cwrapobj, coldef, wyFalse);

            /// Checking whether the column is re-ordered?
            if(!prevoldcwrap && prevcwrap)  //..New Field is added at the top of all existing fields
            {
                if(prevcwrap->m_oldval && prevcwrap->m_newval)
                    reorderstr.Sprintf(" after %s%s%s", m_backtick, prevcwrap->m_newval->m_name.GetString(), m_backtick);
            }
            else if(!prevoldcwrap && cwrapobj->m_prev)  //..if any existing field other than the first field is shifted to the first position in the Grid
            {
                cwrapobjtemp = (FieldStructWrapper*)cwrapobj->m_prev;
                
                while(cwrapobjtemp)
                {
                    if(cwrapobjtemp->m_newval != NULL)
                        break;
                    cwrapobjtemp = (FieldStructWrapper*)cwrapobjtemp ->m_prev;
                }
                if(cwrapobjtemp)
                    reorderstr.SetAs(" first");
            }
			//need to reorder only when the element in the current position is not the same one before
            else if(prevcwrap != NULL && cwrapobj->m_ischanged == wyTrue ) 
            {
				if (cwrapobj->m_oldval != past_ele->m_oldval)
                    reorderstr.Sprintf(" after %s%s%s", m_backtick, prevcwrap->m_newval->m_name.GetString(), m_backtick);
				else
				{
					row_temp =row;
					cwrapobj->m_ischanged = wyFalse;
					past_ele_prev = (FieldStructWrapper*)past_ele->m_prev;
					//present_ele_prev = (FieldStructWrapper*)cwrapobj->m_prev;
					while(row_temp > 0)
					{
						row_temp = row_temp - 1;
						present_ele_prev = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row_temp);
						if(present_ele_prev)
							break;
					}
					while(past_ele_prev !=NULL && present_ele_prev !=NULL)
					{
						if(past_ele_prev->m_newval != present_ele_prev->m_newval)
						{
							if(past_ele_prev->m_newval && present_ele_prev->m_newval)
							{
								if(past_ele_prev->m_newval->m_name.Compare(present_ele_prev->m_newval->m_name) != 0)
								{
									reorderstr.Sprintf(" after %s%s%s", m_backtick, prevcwrap->m_newval->m_name.GetString(), m_backtick);
									cwrapobj->m_ischanged = wyTrue;
									break;
								}
							}

						}
						past_ele_prev = (FieldStructWrapper*)past_ele_prev->m_prev;
						//find the valid row from the grid
						row_temp = row_temp - 1;
						if(row_temp < 0)
							present_ele_prev = NULL;
						while(row_temp >= 0)
                {
						present_ele_prev = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row_temp);
						if(present_ele_prev)
                        break;
						row_temp = row_temp - 1;
						}


                }
					if(cwrapobj->m_ischanged  == wyFalse)
					{
						if(!( past_ele_prev ==NULL && present_ele_prev == NULL))
						{
                    reorderstr.Sprintf(" after %s%s%s", m_backtick, prevcwrap->m_newval->m_name.GetString(), m_backtick);
							cwrapobj->m_ischanged = wyTrue;
						}
					}
					
				}
            }

            if(reorderstr.GetLength())
            {
                coldef.AddSprintf(" %s", reorderstr.GetString());
            }
            
            if((cwrapobj->m_oldval != cwrapobj->m_newval) || reorderstr.GetLength())
            {
                lasterrmsg.SetAs(cwrapobj->m_errmsg);
                columnsstr.AddSprintf("%s,", coldef.GetString());

                if(cwrapobj->m_errmsg.GetLength())
                    columnsstr.AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg.GetString());
            }

            prevoldcwrap = (FieldStructWrapper*)cwrapobj;
        }
        prevcwrap = cwrapobj;
        
        columnsstr.RTrim();
    }

    columnsstr.RTrim();

    if(lasterrmsg.GetLength())
    {
        columnsstr.Strip(lasterrmsg.GetLength() + 6);
    }

    columnsstr.RTrim();

    /// Setting the error message in the query
    if(lasterrmsg.GetLength())
    {
        columnsstr.AddSprintf("\t\t/* %s */", lasterrmsg.GetString());
    }
}

void
	TabFields::GetColumnDefinition(FieldStructWrapper* cwrapobj, wyString &coldef, wyBool isnew)
{
	wyString        newcolumns("");
	wyString        colnamestr;
	FIELDATTRIBS    *fieldattr = NULL;
	wyString        defvalue(""),computedfield(""), onupdate(""),onvirtualpersitentcolumns("");
	wyString        _comment("");
	wyBool skip=wyFalse, dropandrecreate=wyFalse, skipforcheck=wyFalse, dropandrecreatecheck=wyFalse;
	//we can not add other attributes if we are adding VIRTUAL/PERSIATENT 
	if(m_ismariadb52 && ((cwrapobj->m_newval->m_virtuality.GetLength()>5)&&(cwrapobj->m_newval->m_expression.GetLength()!=0)))
		skip=wyTrue;

	else if(m_ismysql57 && ((cwrapobj->m_newval->m_mysqlvirtuality.Compare("(none)"))&&(cwrapobj->m_newval->m_mysqlexpression.GetLength()!=0)))
		skip=wyTrue;
	else if ((cwrapobj->m_newval->m_mycheckexpression.GetLength() != 0))// virtuality and column level check constraint are not allowed on the same column
	{
		skipforcheck = wyTrue;
	}
	//skipforcheck = wyTrue;
	//Altering a Virtual/persuatent column is not allowed we must drop and recreate 
	dropandrecreate=IsDropAndRecreateRequired(cwrapobj,isnew);

	//Alter a CHECK constraint is not allowed. We must drop and recreate
	//dropandrecreatecheck= IsDropAndRecreateCheckRequired(cwrapobj, isnew);
	fieldattr = cwrapobj->m_newval;
	if(!fieldattr)
		return;

	/// Checking which clause to use for column-definition
	if(isnew || dropandrecreate)// || dropandrecreatecheck)
	{ 
		if(dropandrecreate)//|| dropandrecreatecheck)
		{
			newcolumns.AddSprintf("\r\n\tDROP COLUMN %s%s%s ,", m_backtick, cwrapobj->m_oldval->m_name.GetString(), m_backtick );//modified the column needed to drop in case we modify the name of virtual column
			newcolumns.Add("\r\n\tadd column ");
		}
		else
		{
			newcolumns.Add("\r\n\tadd column ");
		}
	}
	else
	{
		if(!cwrapobj->m_oldval)
			return;
		colnamestr.SetAs(cwrapobj->m_oldval->m_name);
		colnamestr.FindAndReplace("`", "``");
		newcolumns.AddSprintf("\r\n\tchange %s%s%s ", m_backtick, colnamestr.GetString(), m_backtick);
	}

	colnamestr.SetAs(fieldattr->m_name);
	colnamestr.FindAndReplace("`", "``");

	newcolumns.AddSprintf("%s%s%s %s", m_backtick, colnamestr.GetString(), m_backtick, fieldattr->m_datatype.GetString());

	if(fieldattr->m_len.GetLength())
		newcolumns.AddSprintf("(%s) ", fieldattr->m_len.GetString());
	else
		newcolumns.Add(" ");

	if(m_ismysql41 && !skip)
	{
		/// adding charset and collation if the user has specified
		if(fieldattr->m_charset.GetLength() && fieldattr->m_charset.CompareI(STR_DEFAULT) != 0)
			newcolumns.AddSprintf("CHARSET %s ", fieldattr->m_charset.GetString());

		if(fieldattr->m_collation.GetLength() && fieldattr->m_collation.CompareI(STR_DEFAULT) != 0)
			newcolumns.AddSprintf("COLLATE %s ", fieldattr->m_collation.GetString());
	}

	// first get the default value.
	if((cwrapobj->m_oldval) && (fieldattr->m_datatype.CompareI("timestamp")== 0) && (fieldattr->m_onupdate))
		onupdate.SetAs(" on update CURRENT_TIMESTAMP ");

	GetDefaultValue(defvalue, cwrapobj);

	/*
	if(fieldattr->m_default.GetLength())
	{
	if(fieldattr->m_notnull && (fieldattr->m_default.CompareI("NULL") == 0))
	{
	defvalue.Sprintf("");
	}

	//If the datatype is timestamp and default string is 0, NULL or CURRENT_TIMESTAMP, then no need to add quotes
	else if((fieldattr->m_default.CompareI(CURRENT_TIMESTAMP) == 0
	|| (((fieldattr->m_datatype.CompareI("timestamp")) == 0) && ((fieldattr->m_default.CompareI("0")) == 0)))
	)
	{
	defvalue.Sprintf("DEFAULT %s ", fieldattr->m_default.GetString());
	}
	/// add default value without quote if the datatype is numeric
	else if(IsDatatypeNumeric(fieldattr->m_datatype))
	{
	defvalue.Sprintf("DEFAULT %s ", fieldattr->m_default.GetString());
	}
	else
	{
	if(fieldattr->m_default.Compare("''") != 0)
	{
	// First Search for `` pattern
	if(fieldattr->m_default.GetCharAt(0) == '`' && fieldattr->m_default.GetCharAt(fieldattr->m_default.GetLength() - 1) == '`' 
	&& fieldattr->m_default.GetLength() == MAXLENWITHBACKTICKS)
	{
	if(fieldattr->m_default.Find("''", 1))
	fieldattr->m_default.SetAs("''");
	}
	wyChar *tbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), fieldattr->m_default.GetString());
	defvalue.Sprintf("default '%s'%s", tbuff, onupdate.GetString());
	}
	else
	defvalue.Sprintf("default ''%s", onupdate.GetString());
	}
	}
	*/
	newcolumns.AddSprintf("%s%s%s", 
		((fieldattr->m_unsigned)?("UNSIGNED "):("")),
		((fieldattr->m_zerofill)?("ZEROFILL "):("")),
		((fieldattr->m_binary) && (!m_ismysql41)?("BINARY "):(""))
		);

	if(fieldattr->m_default.GetLength() || fieldattr->m_onupdate )
		newcolumns.AddSprintf("%s", defvalue.GetString());

	if(!skip)
	{
		newcolumns.AddSprintf("%s%s", 
			((fieldattr->m_notnull)?("NOT NULL "):("NULL ")),
			((fieldattr->m_autoincr)?("Auto_increment ") :(""))
			);
	}
	if(skip)
	{
		if(m_ismariadb52 || m_ismysql57){
			GetVirtualOrPersistentValue(computedfield,cwrapobj);
			newcolumns.AddSprintf("%s", computedfield.GetString());
		}

	}
	
	GetCommentValue(_comment, fieldattr);

	if(_comment.GetLength())
		newcolumns.AddSprintf("%s", _comment.GetString());

	newcolumns.RTrim();


	if (skipforcheck)
	{
		//if (m_ismariadb52)
		//{
		GetCheckValue(computedfield, cwrapobj->m_newval);
		newcolumns.AddSprintf(" %s", computedfield.GetString());
		//}

	}
	coldef.Add(newcolumns.GetString());
}
wyBool     TabFields::IsDropAndRecreateCheckRequired(FieldStructWrapper* cwrapobj, wyBool isnew)
{

	if (!isnew )//&& m_ismariadb52)
	{
		if ((cwrapobj->m_oldval->m_mycheckexpression.CompareI("")==0))
		{
			return wyFalse;
		}
		if ((cwrapobj->m_oldval->m_mycheckexpression.CompareI(cwrapobj->m_newval->m_mycheckexpression.GetString()) != 0))
			return wyTrue;
		else if ((cwrapobj->m_oldval->m_mycheckexpression.CompareI(cwrapobj->m_newval->m_mycheckexpression.GetString()) == 0) && (cwrapobj->m_newval->m_mycheckexpression.GetLength() != 0))
			return wyTrue;

		else
			return wyFalse;


	}
	else
	{
		return wyFalse;
	}
}
wyBool     TabFields::IsDropAndRecreateRequired(FieldStructWrapper* cwrapobj, wyBool isnew)
{

if(!isnew && m_ismariadb52)
{
 if((cwrapobj->m_oldval->m_virtuality.CompareI(cwrapobj->m_newval->m_virtuality.GetString())!=0))
	 return wyTrue;
 else if((cwrapobj->m_oldval->m_virtuality.CompareI(cwrapobj->m_newval->m_virtuality.GetString())==0)&&(cwrapobj->m_newval->m_expression.GetLength()!=0))
	 return wyTrue;
 
 else
     return wyFalse;


}
else if(!isnew && m_ismysql57)
{
 if((cwrapobj->m_oldval->m_mysqlvirtuality.CompareI(cwrapobj->m_newval->m_mysqlvirtuality.GetString())!=0))
	 return wyTrue;
 else if((cwrapobj->m_oldval->m_mysqlvirtuality.CompareI(cwrapobj->m_newval->m_mysqlvirtuality.GetString())==0)&&(cwrapobj->m_newval->m_mysqlexpression.GetLength()!=0))
		 return wyTrue;
 else if((cwrapobj->m_oldval->m_mysqlvirtuality.CompareI(cwrapobj->m_newval->m_mysqlvirtuality.GetString())!=0))
	 return wyTrue;
 else
     return wyFalse;

}
else
{
	return wyFalse;
}


}
void        
	TabFields::GetCommentValue(wyString& query, FIELDATTRIBS*   pfieldattribs)
{
	wyString		dbdefaultvalstr;

	// get comments but we will need to real_escape as somebody can put ' in a comment
	if(pfieldattribs->m_comment.GetLength())
	{
		wyChar * commentbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), pfieldattribs->m_comment.GetString());
		query.AddSprintf("COMMENT '%s'", commentbuff);
		free(commentbuff);
		return;
	}
}

void
TabFields::GetCheckValue(wyString& query, FIELDATTRIBS*   pfieldattribs)
{
	if (pfieldattribs->m_mycheckexpression.GetLength())
	{
		//wyChar * checkexpressbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), pfieldattribs->m_mycheckexpression.GetString());

		// to avoid extra pair of parenthesis
		if(m_isalter)
		{ 
			query.AddSprintf("CHECK %s", pfieldattribs->m_mycheckexpression.GetString());
		}
		else
		{
			query.AddSprintf("CHECK (%s)", pfieldattribs->m_mycheckexpression.GetString());
		}
		
		//free(checkexpressbuff);
		return;
	}
}

void        
	TabFields::GetExtraValues(wyString& query, FIELDATTRIBS*   pfieldattribs)
{

	// Add the extra values like NOT NULL, AUTOINCREMENT etc.
	// Customgrid sends BOOL value as GV_TRU
	wyString		dbdefaultvalstr;

	if(pfieldattribs->m_unsigned)
		query.Add("UNSIGNED ");

	if(pfieldattribs->m_zerofill)
		query.Add("ZEROFILL ");

	if(pfieldattribs->m_binary)
		query.Add("BINARY ");

	if(pfieldattribs->m_notnull)
		query.Add("NOT NULL ");

	if(pfieldattribs->m_autoincr)
	{
		query.Add("AUTO_INCREMENT ");
	}
}

void
TabFields::GetDefaultValue(wyString& query, FieldStructWrapper* cwrapobj)
{
    FIELDATTRIBS    *fieldattr = NULL;
    wyString	defval, datatype, onupdate,len;
    wyChar      *tbuff =  NULL;
    
    if(!cwrapobj || !cwrapobj->m_newval)
        return;

    fieldattr = cwrapobj->m_newval;

    defval.SetAs(fieldattr->m_default);
    datatype.SetAs(fieldattr->m_datatype);
	len.SetAs(fieldattr->m_len);
	//onupdate.SetAs(fieldattr->m_onupdatestr);

    if(defval.GetLength() != 0)
	{
			//return;

		// first get the default value.
		//if((cwrapobj->m_oldval) && (datatype.CompareI("timestamp")== 0) && (fieldattr->m_onupdate))
			//onupdate.SetAs("on update CURRENT_TIMESTAMP");



		query.Add("DEFAULT ");

		/// If datatype is numeric
		if(IsDatatypeNumeric(datatype))
		{
			query.AddSprintf("%s ", defval.GetString());
		}
		/// If datatype is timestamp
		else if((datatype.CompareI("timestamp") == 0 || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql) )) && 
				(defval.FindI("CURRENT_TIMESTAMP") != -1 || defval.FindI("NOW") != -1))
		{
			query.AddSprintf("%s ", defval.GetString());
		}
		else if(defval.Compare("''") == 0)
		{
			query.Add("'' ");
		}
		else
		{
			if(defval.GetCharAt(0) == '`' && defval.GetCharAt(defval.GetLength() - 1) == '`' 
				&& defval.GetLength() == MAXLENWITHBACKTICKS)
			{
				if(defval.Find("''", 1))
					defval.SetAs("''");
			}
			tbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), defval.GetString());
			defval.Sprintf("'%s'", tbuff);
        
			query.AddSprintf("%s ", defval.GetString());
		}
	}


	//onupdate

	if(fieldattr->m_onupdate)
	{
		// on update cannont be true for other datatypes!
		//if(((datatype.CompareI("timestamp")== 0) || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql) )))
			//onupdate.SetAs("ON UPDATE CURRENT_TIMESTAMP");
		//bud fix-if datatype is timestamp or datetime consider length field also
		
		if((datatype.CompareI("timestamp")== 0) || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql) ))
			
		{
			if(len.GetLength()!=0)
				query.AddSprintf("%s(%s)","ON UPDATE CURRENT_TIMESTAMP ",len.GetString());
			else 
			query.AddSprintf("ON UPDATE CURRENT_TIMESTAMP ");
		
		}
		
			
		
	}
}
//function for adding expression and VIRTUAL/PERSIATENT in the query
void TabFields::GetVirtualOrPersistentValue(wyString& query, FieldStructWrapper* cwrapobj)
{
   FIELDATTRIBS    *fieldattr = NULL;
    wyString	virtuality, expression, datatype;
	wyChar      *tbuff =  NULL;
	
    if(!cwrapobj || !cwrapobj->m_newval)
        return;

    fieldattr = cwrapobj->m_newval;

	if(m_ismariadb52){
	virtuality.SetAs(fieldattr->m_virtuality);
	expression.SetAs(fieldattr->m_expression);
	}

	else if(m_ismysql57){
	virtuality.SetAs(fieldattr->m_mysqlvirtuality);
	expression.SetAs(fieldattr->m_mysqlexpression);
	}

	if(!((m_ismariadb52 || m_ismysql57) && (expression.GetLength()!=0) && (virtuality.GetLength()!=0)&& (virtuality.CompareI("(none)")!=0)))
	{return;}

	datatype.SetAs(fieldattr->m_datatype);
	if(expression.GetLength()!=0 && virtuality.GetLength()!=0 )
	{
		query.AddSprintf("AS (");


		if(IsDatatypeNumeric(datatype))
		{
			query.AddSprintf("%s", expression.GetString());
		}
		/// If datatype is timestamp
		else if((datatype.CompareI("timestamp") == 0 || (datatype.CompareI("datetime") == 0 && IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel,&m_mdiwnd->m_mysql) )) && 
				(expression.FindI("CURRENT_TIMESTAMP") != -1 || expression.FindI("NOW") != -1))
		{
			query.AddSprintf("%s ", expression.GetString());
		}
		else if(expression.Compare("''") == 0)
		{
			query.Add("'' ");
		}
		else
		{
			if(expression.GetCharAt(0) == '`' && expression.GetCharAt(expression.GetLength() - 1) == '`' 
				&& expression.GetLength() == MAXLENWITHBACKTICKS)
			{
				if(expression.Find("''", 1))
					expression.SetAs("''");
			}
			
			if (expression.GetCharAt(0)== '\'' && expression.GetCharAt(expression.GetLength())== '\'')
			{
				expression.Erase(expression.GetLength(),1);
				expression.Strip(1);
				tbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), expression.GetString());
				expression.Sprintf("'%s'", tbuff);
			}
			else {
				//tbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), expression.GetString());
				expression.Sprintf("%s", expression.GetString());
			}
		
			query.AddSprintf("%s", expression.GetString());
		}

		query.AddSprintf(") %s ", virtuality.GetString());
	
	}
}
/*
wyBool
TabFields::IsDatatypeNumeric(wyString  &datatype)
{
    if(!(
        datatype.CompareI("bit") == 0 ||
        datatype.CompareI("tinyint") == 0 ||
        datatype.CompareI("bool") == 0 ||
        datatype.CompareI("boolean") == 0 ||
        datatype.CompareI("smallint") == 0 ||
        datatype.CompareI("mediumint") == 0 ||
        datatype.CompareI("int") == 0 ||
        datatype.CompareI("integer") == 0 ||
        datatype.CompareI("bigint") == 0 ||
        datatype.CompareI("float") == 0 ||
        datatype.CompareI("double") == 0 ||
        datatype.CompareI("decimal") == 0 ||
        datatype.CompareI("dec") == 0
        ))
        return wyFalse;

    return wyTrue;
}
*/
void        
TabFields::GetCharsetAndCollationValue(wyString& query, FIELDATTRIBS*   pfieldattribs)
{
	wyString		collationstr, charsetstr;

    if(pfieldattribs->m_charset.GetLength() > 0 && pfieldattribs->m_charset.Compare(STR_DEFAULT) != 0)
		query.AddSprintf("CHARSET %s ", pfieldattribs->m_charset.GetString());
    if(pfieldattribs->m_collation.GetLength() > 0 && pfieldattribs->m_collation.Compare(STR_DEFAULT) != 0)
		query.AddSprintf("COLLATE %s ", pfieldattribs->m_collation.GetString());
}

wyBool
TabFields::GetColumnAndDataType(wyString &query, FIELDATTRIBS*   pfieldattribs)
{
    wyString tmpstr;

    tmpstr.SetAs(pfieldattribs->m_name);
    tmpstr.FindAndReplace("`", "``");

    query.AddSprintf("%s%s%s %s", m_backtick, tmpstr.GetString(), m_backtick, pfieldattribs->m_datatype.GetString());
    if(pfieldattribs->m_len.GetString() && strlen(pfieldattribs->m_len.GetString()))
    {
        query.AddSprintf("(%s)", pfieldattribs->m_len.GetString());
    }
    query.Add(" ");
    return wyFalse;
}

wyInt32
TabFields::ValidateFields(wyBool showmsg)
{
    wyString	prevfieldname, curfieldname; //Holdes the field name
	wyString	datatypename;       //Holds the datatype name
	wyString	lengthfield;		//Holds the length of the datatype(length field)
	wyString	defaultfield;
	wyString	message;
    FIELDATTRIBS    *pfieldattribs = NULL, *ptempfieldattribs = NULL;
    wyInt32         fieldcounter = 0;
    FieldStructWrapper* cwrapobj = NULL, *cwrapobjtemp = NULL;
    
    cwrapobj = (FieldStructWrapper*) m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        if(cwrapobj->m_oldval == cwrapobj->m_newval)
        {
            fieldcounter++;
            cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
            continue;
        }

        pfieldattribs = cwrapobj->m_newval;

        if(!(pfieldattribs))   //..if newval is null (will be true if the user drops the field)
        {
            cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
            continue;
        }
        cwrapobj->m_errmsg.Clear();
        fieldcounter++;
        //checks whether field name is empty and datatype field is entered
        if((pfieldattribs->m_name.GetLength() == 0) && (pfieldattribs->m_datatype.GetLength() != 0))
        {
            //..Store error message only if nothing is there in m_errmsg..
            if(cwrapobj->m_errmsg.GetLength() == 0)
                cwrapobj->m_errmsg.SetAs(FIELDNAME_NOTSPECIFIED_EX);

            if(showmsg)
            {
                if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                wyInt32 row = GetRowNoForWrapper(cwrapobj);
                CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);

                SetFocus(m_hgridfields);
                MessageBox(m_hwnd, cwrapobj->m_errmsg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                SetFocus(m_hgridfields);
			    return 0;
            }
        }

        //..both column-name and datatype erased for existing fields (which are not dropped)
        if((pfieldattribs->m_name.GetLength() == 0) && (pfieldattribs->m_datatype.GetLength() == 0) && (cwrapobj->m_oldval && cwrapobj->m_newval))
        {
            message.SetAs(_("Column name and datatype can not be empty"));
            if(cwrapobj->m_errmsg.GetLength() == 0)
                cwrapobj->m_errmsg.SetAs(message);

            if(showmsg)
            {
                if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                wyInt32 row = GetRowNoForWrapper(cwrapobj);
                CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);

                SetFocus(m_hgridfields);
                MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                SetFocus(m_hgridfields);
                return 0;
            }
        }

        //checks wheather field name is entered and datatype field is empty 
        if((pfieldattribs->m_name.GetLength() != 0) && (pfieldattribs->m_datatype.GetLength() == 0))
        {
            wyString    errmsg("");
            errmsg.SetAs(_("Data type not specified"));

            if(cwrapobj->m_errmsg.GetLength() == 0)
                cwrapobj->m_errmsg.SetAs(errmsg);

            if(showmsg)
            {
                if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                wyInt32 row = GetRowNoForWrapper(cwrapobj);
                CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? DATATYPE : 0, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? DATATYPE : 0, wyTrue);
                
                SetFocus(m_hgridfields);
                MessageBox(m_hwnd, errmsg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                SetFocus(m_hgridfields);
                return 0;
            }
        }

        //..Checks for default value for character type datatype
        if((pfieldattribs->m_default.GetLength() != 0) && (pfieldattribs->m_default.CompareI("''") == 0))
		{
            if(!((pfieldattribs->m_datatype.CompareI("varchar") == 0) || (pfieldattribs->m_datatype.CompareI("char") == 0) ||
                (pfieldattribs->m_datatype.CompareI("varbinary") == 0) || (pfieldattribs->m_datatype.CompareI("enum") == 0) ||
                (pfieldattribs->m_datatype.CompareI("set") == 0)))
			{
                message.Sprintf(_("Default empty string ('') is not supported for %s data type"), pfieldattribs->m_datatype.GetString());

                if(cwrapobj->m_errmsg.GetLength() == 0)
                    cwrapobj->m_errmsg.SetAs(message);

                if(showmsg)
                {
                    if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                        CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                    wyInt32 row = GetRowNoForWrapper(cwrapobj);
                    CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? DEFVALUE : 0, wyTrue);
                    CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? DEFVALUE : 0, wyTrue);

                    SetFocus(m_hgridfields);
				    MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                    SetFocus(m_hgridfields);
				    return 0;
                }
			}
		}

        //..Checks for length of "varchar" and "varbinary" datatype
        if(
            ((pfieldattribs->m_datatype.CompareI("varchar") == 0) || (pfieldattribs->m_datatype.CompareI("varbinary") == 0) || 
             (pfieldattribs->m_datatype.CompareI("enum") == 0) || (pfieldattribs->m_datatype.CompareI("set") == 0)
            ) && pfieldattribs->m_len.GetLength() == 0)
        {
            message.Sprintf(_("Length must be specified for %s data type"), pfieldattribs->m_datatype.GetString());

            if(cwrapobj->m_errmsg.GetLength() == 0)
                cwrapobj->m_errmsg.SetAs(message);

            if(showmsg)
            {
                if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                wyInt32 row = GetRowNoForWrapper(cwrapobj);
                CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? LENGTH : 0, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, (row != -1 )? LENGTH : 0, wyTrue);

                SetFocus(m_hgridfields);
				MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                SetFocus(m_hgridfields);
				return 0;
            }
        }

        //..Checks for duplicate field
        cwrapobjtemp = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();
        
        for(cwrapobjtemp = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();     //..Initialization
            cwrapobjtemp;                                                           //..Condition
            cwrapobjtemp = (FieldStructWrapper*)cwrapobjtemp->m_next                 //..
            )
        {
            if(cwrapobjtemp == cwrapobj)
                continue;

            ptempfieldattribs   =   cwrapobjtemp->m_newval;

            if(ptempfieldattribs && ptempfieldattribs->m_name.CompareI(pfieldattribs->m_name) == 0)
                break;
        }
        
        if(cwrapobjtemp)    //..condition will be true, if the same field name is used in two different column definitions
        {
            if(cwrapobj->m_errmsg.GetLength() == 0)
                cwrapobj->m_errmsg.SetAs(DUPLICATED_FIELDNAME_EX);

            if(showmsg)
            {
                if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);

                wyInt32 row = GetRowNoForWrapper(cwrapobj);
                CustomGrid_SetCurSelection(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfields, (row != -1 )? row : 0, 0, wyTrue);

                SetFocus(m_hgridfields);
                MessageBox(m_hwnd, DUPLICATED_FIELDNAME_EX, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                SetFocus(m_hgridfields);
                return 0;
            }
        }

        cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
    }

    
    if(fieldcounter == 0)   //.. (Create table : No row in the grid)    (Alter table : All fields are dropped)
    {
        if(showmsg)
        {
            if(!(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN))
                CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 0);
            
            CustomGrid_SetCurSelection(m_hgridfields, 0, 0, wyTrue);
            CustomGrid_EnsureVisible(m_hgridfields, 0, 0, wyTrue);

            SetFocus(m_hgridfields);
            MessageBox(m_hwnd, EMPTY_ROWS_EX, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
            SetFocus(m_hgridfields);
            return 0;
        }
    }
    
	return 1;
}

void
TabFields::InitCharsetCol()
{
    wyString        charset, query;
    MYSQL_ROW       myrow;
    
	//only once we are executing the query.
	//if it is not NULL means if we already executed the query then we are reusing the resultset
	if(m_charsetres == NULL)
	{
		query.SetAs("show charset");
		m_charsetres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
		if(!m_charsetres)
		{
			ShowMySQLError(m_hgridfields, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
			return;
		}
	}
	else
		m_mdiwnd->m_tunnel->mysql_data_seek(m_charsetres, 0);

    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(m_charsetres))
    {
        charset.SetAs(myrow[0]);
        CustomGrid_InsertTextInList(m_hgridfields, CHARSETCOL, charset.GetAsWideChar());
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        if(CustomGrid_FindTextInList(m_hgridfields, CHARSETCOL, TEXT(STR_DEFAULT)) == -1)
            CustomGrid_InsertTextInList(m_hgridfields, CHARSETCOL, TEXT(STR_DEFAULT));  
    }
    return;
}

LRESULT
TabFields::OnGVNBeginLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyUInt32		row = wParam;
	wyUInt32		col = lParam;
    wyInt32         charsetlen = 0;
    wyString        selcharsetstr, datatype;
    wyUInt32		totalrows = 0;

    GetGridCellData(m_hgridfields, wParam, lParam, m_fieldprevval);

    /// Validating GVN__Begin_label_Edit message
    if(ValidateOnBeginLabelEdit(row,col) == wyFalse)
		return FALSE;

    if(m_ismysql41 == wyTrue)
	{
		if(col == CHARSETCOL)
		{
            /// Deleting all values from the list and refilling charset values in the list
			CustomGrid_DeleteListContent(m_hgridfields, CHARSETCOL);
			InitCharsetCol();
		}
		else if(col == COLLATIONCOL)
		{
            charsetlen = GetGridCellData(m_hgridfields, row, CHARSETCOL, selcharsetstr);
			CustomGrid_DeleteListContent(m_hgridfields, COLLATIONCOL);

			if(selcharsetstr.CompareI(STR_DEFAULT) == 0)
				InitCollationCol();
			else if(charsetlen > 0)
				FilterCollationColumn(row, charsetlen);
		}
	}

    GetGridCellData(m_hgridfields, wParam, lParam, m_oldvalue);
    
	if(col == LENGTH)
    {
        GetGridCellData(m_hgridfields, wParam, DATATYPE, datatype);
        if((datatype.CompareI("enum") == 0 || datatype.CompareI("set") == 0))
        {
            HandleEnumColumn(m_hgridfields, row, datatype);
            return 0;
        }
    }

    totalrows = CustomGrid_GetRowCount(m_hgridfields);

	if(row == (totalrows - 1))
    {
		CustomGrid_InsertRow(m_hgridfields);
        SendMessage(m_ptabmgmt->m_hwndtool, TB_SETSTATE, IDI_MOVEDOWN, (LPARAM)TBSTATE_ENABLED);
    }

    return 1;    
}

void
TabFields::FilterCollationColumn(wyInt32 row, wyInt32 charsetlen)
{
    wyWChar         *selcharset = NULL, *relcollation = NULL;
    wyString        collation, query, selcharsetstr;
    MYSQL_RES       *myres = NULL;
    MYSQL_ROW       myrow;
    
    selcharset  = AllocateBuffWChar(charsetlen + 1);

	CustomGrid_GetItemText(m_hgridfields, row, CHARSETCOL, selcharset);
    CustomGrid_DeleteListContent(m_hgridfields, COLLATIONCOL);
    selcharsetstr.SetAs(selcharset);
    
    free(selcharset);

    query.SetAs("show collation");
    myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
    if(!myres)
	{
        ShowMySQLError(m_hgridfields, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return;
	}
    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
    {
        collation.SetAs(myrow[0]);
        
        // Add the relevent Items
        if((relcollation = wcsstr(collation.GetAsWideChar(), selcharsetstr.GetAsWideChar())) != NULL)
        {
            if(collation.GetCharAt(charsetlen) == '_')
                CustomGrid_InsertTextInList(m_hgridfields, COLLATIONCOL, collation.GetAsWideChar());
        }
    }
    
    CustomGrid_InsertTextInList(m_hgridfields, COLLATIONCOL, TEXT(STR_DEFAULT));
    
    m_mdiwnd->m_tunnel->mysql_free_result(myres);

    return;
}

void
TabFields::InitCollationCol()
{
    wyString        collation, query;
    MYSQL_RES       *myres = NULL;
    MYSQL_ROW       myrow;
    
    query.SetAs("show collation");
    myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
     if(!myres)
	{
        ShowMySQLError(m_hgridfields, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return;
	}
    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
    {
        collation.SetAs(myrow[0]);
        CustomGrid_InsertTextInList(m_hgridfields, COLLATIONCOL, collation.GetAsWideChar());
    }
    if(CustomGrid_FindTextInList(m_hgridfields, COLLATIONCOL, TEXT(STR_DEFAULT)) == -1)
        CustomGrid_InsertTextInList(m_hgridfields, COLLATIONCOL, TEXT(STR_DEFAULT));

    m_mdiwnd->m_tunnel->mysql_free_result(myres);

    return;
}

wyBool
TabFields::ValidateOnBeginLabelEdit(wyUInt32 row, wyUInt32 col)
{
    wyString celldata("");
	wyString    tempstr("");
    wyWChar     *data;
    wyUInt32    celldatalen = 0;

    GetGridCellData(m_hgridfields, row, CNAME, celldata);
    if(col != CNAME && !celldata.GetLength())
		return wyFalse;

    GetGridCellData(m_hgridfields, row, DATATYPE, celldata);
    if(col != ZERO && col != DATATYPE && !celldata.GetLength())
		return wyFalse;

	celldatalen = 0;
	celldatalen = CustomGrid_GetItemTextLength(m_hgridfields, row, VIRTUALITY);
	if(celldatalen)
	{
		data = (wyWChar*)malloc(sizeof(wyWChar) * (celldatalen + 1));
		data[0] = '\0';
		CustomGrid_GetItemText(m_hgridfields, row, VIRTUALITY, data);
		tempstr.SetAs(data);
		free(data);
	}

    // For a column to be editable, it has to satisfy the following two conditions:
	// 1.) First column can be editable only if the previous row has data. 0th row is an exception
	// 2.) 1+n column can only be edited if there is data in the first column. So you will have to enter name first to change other values
    switch(col)
	{
    case NOTNULL:
        if(m_ismysql41)
        {
            if(CustomGrid_GetBoolValue(m_hgridfields, row, PRIMARY) == wyTrue)
                return wyFalse;
        }
		if(m_ismariadb52 || m_ismysql57){

			if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
			}
				 //if cloumn is virtual or stored than don't allow it to be NULL from 12.05
				 //if(CustomGrid_GetItemTextLength(m_hgridfields, row, VIRTUALITY)>6)
               // return wyFalse;
        }
        break;

    case UNSIGNED:
        if(!m_ismysql41)
        {
            if(CustomGrid_GetBoolValue(m_hgridfields, row, PRIMARY) == wyTrue)
                return wyFalse;
        }
        break;
	case PRIMARY:
		if(m_ismariadb52 || m_ismysql57)
			 { //if cloumn is virtual or stored than don't allow it to be primary key from 12.05
				if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
				}
			}
		break;
		case DEFVALUE:
		if(m_ismariadb52 || m_ismysql57)
			{ //if cloumn is virtual or stored than don't allow it to be Default value from 12.05
				if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
				}
         }
		break;
		case AUTOINCR:
		if(m_ismariadb52 || m_ismysql57)
			 { //if cloumn is virtual or persitent than don't allow it to be autoincrement from 12.05
				
				if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
				}
			 }
		break;
		//expreesion is not allowed if virtuality is not given
     case EXPRESSION:
	 if(m_ismariadb52 || m_ismysql57)
			 { //if cloumn is not virtual or persitent  then don't allow EXpression
				if(!tempstr.Compare("") || !tempstr.Compare("(none)")){
				return wyFalse;
				}
			}
		break;
		 case CHARSETCOL:
		   if(m_ismariadb52 || m_ismysql57)
			 { //if cloumn is virtual or persitent than don't allow it to be charset from 12.05
				 
				if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
				}
			}
		break;
		 case COLLATIONCOL:
		   if(m_ismariadb52 || m_ismysql57)
			 { //if cloumn is virtual or persitent than don't allow it to be colliation from 12.05
				if(tempstr.Compare("") && tempstr.Compare("(none)")){
				return wyFalse;
				}
			 }
		break;
	default:
        break;
	}

    if(col == (AUTOINCR + (m_ismysql41 ? 0 : 1)) )
    {
        GetGridCellData(m_hgridfields, row, DEFVALUE, celldata);
        if(m_autoincrowid != -1 && m_autoincrowid != row)
            return wyFalse;
        else if(celldata.GetLength())
            return wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::ValidateOnEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyUInt32		index = 0, addcol = 0;
	wyUInt32		row = LOWORD(wParam);
	wyUInt32		col = HIWORD(wParam);
	wyBool			checked;
    wyInt32         issuccess = 0;
    HWND            hwndgrid = m_hgridfields;
	wyString		tempstr("");

    if(m_ismysql41 == wyTrue)                 //The following comment is there in TableMakerBase.cpp
		index = 1;							// this operation needs to be commented
    else
        addcol = 1;

    /* Basically we do certain validation in this function:

	   The following validations are done:
	   1. If a user has CHECKED primary then we select AUTOCINCREMENT column and NOT NULL column as a column cannot be primary and NOT AUTOINCR and NOTNULL and we make NOT NULL readonly.
	   2. If a user has UNCHECKED primary, then we unselect AUTOCINCREMENT and make NOT NULL column non-read only.
	   3. If a user has selected DATATYPE then we do some validation depending upon selected datatype.
	   4. If a user has selected AUTOINCREMENT column then we select PRIMARY KEY too as there can be no AUTOINCREMENT without PRIMARY KEY.
	   5. If a user has selected AUTOINCREMENT column then DEFAULT VALUE column is disabled
	   6. If a user has selected DEFAULT VALUE column then AUTOINCREMENT column is disabled
	*/

    if(col == PRIMARY)
	{
        FieldStructWrapper *cwrapobj = NULL;
        cwrapobj = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row);

        /// Changing the PRIMARY index
		m_ptabmgmt->m_tabindexes->HandlePrimaryKeyIndex();
        
		if(stricmp((wyChar*)lParam, GV_TRUE)== 0) 
		{
            // user has checked PRIMARY KEY.
			
			
			CustomGrid_SetBoolValue(hwndgrid, row, NOTNULL + addcol, GV_TRUE);
			CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyTrue);
			
		} 
		else if (stricmp((CHAR*)lParam, GV_FALSE) == 0 || stricmp((CHAR*)lParam, "") == 0 )
		{
			// user has unchecked PRIMARY KEY option
			CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyFalse);
		}
	}
    else if(col == DATATYPE)
    {
        /// validating(enabling/disabling) grid-columns for the row 
		SetValidation(row, (wyChar*)lParam);
    }
    else if(col == (AUTOINCR + addcol))
	{	
        if(m_autoincrowid == -1)
		{
			CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + addcol, GV_TRUE);

			//If a user has selected AUTOINCREMENT column then DEFAULT VALUE column is disabled
			CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyTrue);
			/// storing auto-increment row number
            m_autoincrowid = row;
            GetGridCellData(m_hgridfields, row, CNAME, m_autoinccol);
		}
		else
		{
			checked = CustomGrid_GetBoolValue(hwndgrid,row, col);
			if(checked == wyFalse && row == m_autoincrowid)
			{
				m_autoincrowid = -1;
                m_autoinccol.Clear();
				CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse);
			}
		}
	}

    if(m_ismysql41 == wyTrue && col == CHARSET)
    {
        issuccess = CustomGrid_DeleteListContent(hwndgrid, COLLATION);
        
        FieldStructWrapper *cwrapobj = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row);

        if(cwrapobj && cwrapobj->m_oldval && (cwrapobj->m_oldval->m_charset.CompareI((wyChar*)lParam) == 0))
        {
            CustomGrid_SetText(hwndgrid, row, COLLATION, (wyChar*)cwrapobj->m_oldval->m_collation.GetString());
            cwrapobj->m_newval->m_collation.SetAs(cwrapobj->m_oldval->m_collation);
        }
        else
            CustomGrid_SetText(hwndgrid, row, COLLATION, STR_DEFAULT);
    }

    // if default value is present then disable autoincrement column 
	if(col == DEFVALUE)
	{
		if(strlen((wyChar*)lParam) !=  0)
        {
			CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyTrue);
        }
		else
			CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyFalse);
	}

	if(col == VIRTUALITY && (m_ismariadb52 || m_ismysql57))
	{
	tempstr.SetAs((wyChar*)lParam);
	FieldStructWrapper *cwrapobj = NULL;
    cwrapobj = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row);

	// If column is Virtuality then set readonly/default value for various field

	if(tempstr.Compare("") && tempstr.Compare("(none)")){
	//we will uncheck every checkbox not required and set them read only if column is VIRTUAL/PERSISTENT/STORED
	CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + addcol, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyTrue);

	CustomGrid_SetText(hwndgrid, row, DEFVALUE + addcol, "");
	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE + addcol,wyTrue);

	CustomGrid_SetBoolValue(hwndgrid, row, PRIMARY + addcol, GV_FALSE);
	m_ptabmgmt->m_tabindexes->HandlePrimaryKeyIndex();			 /// Changing the PRIMARY index
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY + addcol, wyTrue);

	CustomGrid_SetBoolValue(hwndgrid, row, NOTNULL + addcol,GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyTrue);

	CustomGrid_SetBoolValue(hwndgrid, row, ONUPDATECT  + addcol, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT   + addcol, wyTrue);

	CustomGrid_SetColumnReadOnly(hwndgrid, row,  EXPRESSION  + addcol, wyFalse);

	cwrapobj->m_newval->m_pk = wyFalse;
	cwrapobj->m_newval->m_notnull = wyFalse;
    cwrapobj->m_newval->m_autoincr = wyFalse;
    cwrapobj->m_newval->m_default.SetAs("");
	cwrapobj->m_newval->m_onupdate = wyFalse;
	}
	else
	{
		// If virtuality selected as (none) then enable primary key, Not nullcheckbox, and disable Expression
    CustomGrid_SetText(hwndgrid, row,  EXPRESSION  + addcol,"");
    CustomGrid_SetColumnReadOnly(hwndgrid, row,  EXPRESSION  + addcol, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY + addcol, wyFalse);		
	CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE + addcol,wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT   + addcol, wyFalse);
	}
	}
    return wyTrue;
}

FieldStructWrapper*
TabFields::GetNextCWrapObjFromGrid(wyUInt32 row)
{
    wyUInt32    count = CustomGrid_GetRowCount(m_hgridfields);
    FieldStructWrapper* cwrapobj = NULL;

    for( ; row < count; row++)
    {
        cwrapobj = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hgridfields, row);
        if(cwrapobj)
        {
            return cwrapobj;
        }
    }
    return NULL;
}

wyBool
TabFields::IsValidLength(wyString  &lengthstr, wyString &datatypestr)
{
    wyInt32 cntcomma = 0;
    wyInt32 len = lengthstr.GetLength();

    for(wyInt32 i = 0; i<len; i++)
    {
        switch(lengthstr.GetCharAt(i))
        {
        case ',':
            if(!(datatypestr.CompareI("real") == 0 || datatypestr.CompareI("double") == 0 
                    || datatypestr.CompareI("decimal") == 0 || datatypestr.CompareI("float") == 0
                    || datatypestr.CompareI("numeric") == 0 
                    ))
                return wyFalse;
            cntcomma++;

        case ' ':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            continue;
            
        default:
            return wyFalse;
        }
    }

    if(cntcomma > 1)
        return wyFalse;

    return wyTrue;
}

LRESULT
TabFields::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyUInt32        row, col;
    wyInt32         index = 0, addcol = 0;
    wyString        curdatastr, datatypestr;
    wyString        datastr, lenstr;
    wyChar          *data = (wyChar*)lParam;
    FieldStructWrapper *cwrapobj = NULL, *cwrapobjnext = NULL;
    
    row = LOWORD(wParam);
    col = HIWORD(wParam);

    /// To avoid CustomGrid right-click Issue

	if(m_ismariadb52||m_ismysql57)
    { //for virtual/persistent columns go till column 14
		// Added check constraint column so now it will go till column 15
		if(!(col >= 0 && col <= 15))
        return 1;
	}
	else

	{
	
	if(!(col >= 0 && col <= 13))
        return 1;
	
	}

    if(!(row >= 0 && row <= CustomGrid_GetRowCount(m_hgridfields)))
        return 1;

    if(data)
        curdatastr.SetAs(data);
    curdatastr.RTrim();

    /// If the cell-data is same as before, then return from here.
    if(m_fieldprevval.Compare(curdatastr) == 0)
        return 1;

    /// Get the attached wrapper
    cwrapobj = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);

    /// Make the flag dirty
    m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    
    if(m_ismysql41)
		index = 1;
    else
        addcol = 1;

    if(col == (AUTOINCR + addcol))
    {
        /// To avoid modifying wrapper on pressing "Delete" key (for Checkbox in CustomGrid)
        if(curdatastr.GetLength() == 0 && m_fieldprevval.CompareI("false") == 0)
            return 0;
    }
    else if(col == DATATYPE)
    {
        GetGridCellData(m_hgridfields, row, LENGTH, lenstr);

        /// Modifying m_len structure element according to the datatype
        if(curdatastr.CompareI("enum") != 0 && curdatastr.CompareI("set") != 0)
        {
            /// Checking whether the length (on the grid) is valid for the newly set datatype or not?
            if(!IsValidLength(lenstr, curdatastr))
            {
                if(cwrapobj)
                {
                    if(cwrapobj->m_oldval && cwrapobj->m_oldval == cwrapobj->m_newval)
                        cwrapobj->m_newval = GetDuplicateFieldAttribsStruct(cwrapobj->m_oldval);
                    if(cwrapobj->m_newval)
                        cwrapobj->m_newval->m_len.Clear();
                }
                CustomGrid_SetText(m_hgridfields, row, LENGTH, (wyChar*)"");
            }
        }
        else
        {
            if(!(m_fieldprevval.CompareI("enum") == 0 || m_fieldprevval.CompareI("set") == 0))
            {
                if(lenstr.GetLength())
                {
                    if(lenstr.GetCharAt(0) != 0 || lenstr.GetCharAt(lenstr.GetLength()-1) != 0)
                    {
                        if(cwrapobj)
                        {
                            if(cwrapobj->m_oldval && cwrapobj->m_oldval == cwrapobj->m_newval)
                                cwrapobj->m_newval = GetDuplicateFieldAttribsStruct(cwrapobj->m_oldval);
                            if(cwrapobj->m_newval)
                                cwrapobj->m_newval->m_len.Clear();
                        }
                        CustomGrid_SetText(m_hgridfields, row, LENGTH, (wyChar*)"");
                    }
                }
            }
        }
    }
    else if(col == LENGTH)
    {
        wyString datatype;
        GetGridCellData(m_hgridfields, row, DATATYPE, datatype);

        if(datatype.CompareI("set") != 0 && datatype.CompareI("enum") != 0)
        {
            /// Changing the lParam value if the length is not valid for the datatype 
            if(!IsValidLength(curdatastr, datatype))
            {
                wyChar* data = (wyChar*)lParam;
                wyString len;

                len.Sprintf("%d", atoi(data));
                
                if(len.GetLength())
                {
                    strcpy(((wyChar*)lParam), (wyChar*)len.GetString());
                    curdatastr.SetAs(len);
                }
            }
        }
    }
    
    if(col == CNAME)
    {
        /// Saving auto_increment column name to the class variable m_autoinccol, if the current row is auto_increment
        if(row == m_autoincrowid)
            m_autoinccol.SetAs(curdatastr);
    }

    if(cwrapobj)
    {
        /// If any value of the existing column (grid-row) is not modified, then create the duplicate of the existing column and assign new value to the m_newval. (Alter table only)
        if((cwrapobj->m_newval == cwrapobj->m_oldval) && (m_fieldprevval.Compare(data) != 0))
        {
            cwrapobj->m_newval = GetDuplicateFieldAttribsStruct(cwrapobj->m_oldval);
        }
        /// When user has manually deleted existing column (grid-row)
        else if(!cwrapobj->m_newval)
        {
            cwrapobj->m_newval = new FIELDATTRIBS();
            InitFieldAttribs(cwrapobj->m_newval);
        }
        
        if(col == CNAME)
        {
            cwrapobj->m_newval->m_name.SetAs(curdatastr);
            /// Changing the column-names in corresponsing indexes and foreign-keys
            HandleIndexesOnFieldRename(cwrapobj);
            HandleFKsOnFieldRename(cwrapobj);
        }

        if(col == DATATYPE)
        {
            if( !(curdatastr.CompareI("CHAR") == 0 || curdatastr.CompareI("VARCHAR") == 0 || 
                  curdatastr.CompareI("TINYTEXT") == 0 || curdatastr.CompareI("TEXT") == 0 || 
                  curdatastr.CompareI("MEDIUMTEXT") == 0 || curdatastr.CompareI("LONGTEXT") == 0 || 
                  curdatastr.CompareI("ENUM") == 0 || curdatastr.CompareI("SET") == 0 ))
            {
                cwrapobj->m_newval->m_charset.Clear();
                cwrapobj->m_newval->m_collation.Clear();
                
                CustomGrid_SetText(m_hgridfields, row, CHARSET, "");
                CustomGrid_SetText(m_hgridfields, row, COLLATION, "");
            }

            if(!((curdatastr.CompareI("CHAR") == 0 || curdatastr.CompareI("VARCHAR") == 0 || 
                  curdatastr.CompareI("TINYTEXT") == 0 || curdatastr.CompareI("TEXT") == 0 || 
                  curdatastr.CompareI("MEDIUMTEXT") == 0 || curdatastr.CompareI("LONGTEXT") == 0 || 
                  curdatastr.CompareI("blob") == 0 || curdatastr.CompareI("JSON") == 0 || curdatastr.CompareI("tinyblob") == 0 ) ||
                  curdatastr.CompareI("longblob") == 0 || curdatastr.CompareI("mediumblob") == 0 ))
            {
                /// Modifies Index-Length when user sets datatype other than above
                HandleIndexesOnDatatypeChange(cwrapobj);
            }
        }

        if(m_ismysql41 && col == CHARSET)
        {
            /// Resetting Structure variable for collation 
            cwrapobj->m_newval->m_collation.Clear();
        }
    }
    /// If no wrapper is associated with the grid-row, then create a wrapper and attach it with the grid-row.
    else if(curdatastr.GetLength())
    {
        FIELDATTRIBS *value = new FIELDATTRIBS;
        InitFieldAttribs(value);
        cwrapobj = new FieldStructWrapper(value, wyTrue);

        /// Checking whether the new wrapper is in between other grid-rows that have wrappers associated with them
        cwrapobjnext = GetNextCWrapObjFromGrid(row+1);
        if(cwrapobjnext)
            m_listwrapperstruct.InsertBefore(cwrapobjnext, cwrapobj);
        else
            m_listwrapperstruct.Insert(cwrapobj);

        CustomGrid_SetRowLongData(m_hgridfields, row, (LPARAM)cwrapobj);
        
        wyInt32 ncols = CustomGrid_GetColumnCount(m_hgridfields);
        //..Loop to read the row and assign values to the class-object members
        for(int i=0; i<ncols; i++)
        {
            wyString tempstr;
            GetGridCellData(m_hgridfields, row, i, tempstr);
            SetValueToStructure(row, i, (wyChar*) tempstr.GetString());
        }
    }

    GetGridCellData(m_hgridfields, row, DATATYPE, datatypestr);

    /// If user has deleted both datatype and column name, then drop the column from indexes and foreign-keys
    if((!curdatastr.GetLength() && !datatypestr.GetLength()) && col == CNAME && cwrapobj)
    {
        HandleIndexesOnFieldDelete(cwrapobj);
        HandleFKsOnFieldDelete(cwrapobj);

        if(!cwrapobj->m_oldval)
            CustomGrid_SetRowLongData(m_hgridfields, row, (LPARAM)NULL);

        /// Resetting entire row
        ResetRowValues(row);
        ChangeListOnDelete(cwrapobj);

        return 1;
    }
    
    /// Setting value to the structure
    if(cwrapobj)
        SetValueToStructure(row, col, (wyChar*) curdatastr.GetString());

    ValidateOnEndLabelEdit(wParam,lParam);

    datastr.SetAs(data);

    if(cwrapobj->m_newval)
        ScanEntireRow(row, col, datastr);
	
    m_fieldprevval.Clear();
    return 1;
}

void
TabFields::ResetRowValues(wyUInt32 row)
{
    wyUInt32 addcol = 0;

    if(m_ismysql41 != wyTrue)
        addcol = 1;

    CustomGrid_SetText(m_hgridfields, row, LENGTH, "");
    CustomGrid_SetText(m_hgridfields, row, DEFVALUE, "");

    CustomGrid_SetBoolValue(m_hgridfields, row, PRIMARY, GV_FALSE);
    CustomGrid_SetBoolValue(m_hgridfields, row, BINARY, GV_FALSE);
    CustomGrid_SetBoolValue(m_hgridfields, row, NOTNULL + addcol, GV_FALSE);
    CustomGrid_SetBoolValue(m_hgridfields, row, UNSIGNED + addcol, GV_FALSE);
    CustomGrid_SetBoolValue(m_hgridfields, row, AUTOINCR + addcol, GV_FALSE);
    CustomGrid_SetBoolValue(m_hgridfields, row, ZEROFILL + addcol, GV_FALSE);
    
    if(m_ismysql41)
    {
        CustomGrid_SetText(m_hgridfields, row, CHARSET, "");
        CustomGrid_SetText(m_hgridfields, row, COLLATION, "");
        CustomGrid_SetText(m_hgridfields, row, COMMENT_, "");
    }
}

void
TabFields::HandleIndexesOnDatatypeChange(FieldStructWrapper* fieldswrapobj)
{
    IndexedBy              *indexedby = NULL;

    indexedby = (IndexedBy*)fieldswrapobj->m_listindexesworkingcopy.GetFirst();

    while(indexedby)
    {
        m_ptabmgmt->m_tabindexes->HandleIndexesOnDatatypeChange(indexedby->m_pindexwrap, fieldswrapobj);
        indexedby = (IndexedBy*)indexedby->m_next;
    }
}

FIELDATTRIBS*
TabFields::GetDuplicateFieldAttribsStruct(FIELDATTRIBS* original)
{
    FIELDATTRIBS* newstruct = NULL;

    if(!original)
        return NULL;

    newstruct = new FIELDATTRIBS;

    newstruct->m_name.SetAs(original->m_name);
    newstruct->m_datatype.SetAs(original->m_datatype);
    newstruct->m_len.SetAs(original->m_len);
    newstruct->m_default.SetAs(original->m_default);

    newstruct->m_pk = original->m_pk;
    newstruct->m_binary = original->m_binary;
    newstruct->m_notnull = original->m_notnull;
    newstruct->m_unsigned = original->m_unsigned;
    newstruct->m_autoincr = original->m_autoincr;
    newstruct->m_zerofill = original->m_zerofill;
    newstruct->m_onupdate = original->m_onupdate;

    newstruct->m_charset.SetAs(original->m_charset);
    newstruct->m_collation.SetAs(original->m_collation);
    newstruct->m_comment.SetAs(original->m_comment);
	newstruct->m_virtuality.SetAs(original->m_virtuality);
	newstruct->m_expression.SetAs(original->m_expression);
	newstruct->m_mysqlvirtuality.SetAs(original->m_mysqlvirtuality);
	newstruct->m_mysqlexpression.SetAs(original->m_mysqlexpression);
	newstruct->m_mycheckexpression.SetAs(original->m_mycheckexpression);
	
    newstruct->m_next = NULL;

    return newstruct;
}

void
TabFields::SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data)
{
    FieldStructWrapper *cwrap = NULL;

    cwrap = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);

    if(col == CNAME)
    {
        cwrap->m_newval->m_name.SetAs(data);
    }
    else if(col == DATATYPE)
    {
        cwrap->m_newval->m_datatype.SetAs(data);
    }
    else if(col == LENGTH)
    {
        cwrap->m_newval->m_len.SetAs(data);
    }
    else if(col == DEFVALUE)
    {
        cwrap->m_newval->m_default.SetAs(data);
    }
	else if(col == ONUPDATECT)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            cwrap->m_newval->m_onupdate = wyTrue;
        }
        else
        {
            cwrap->m_newval->m_onupdate = wyFalse;
        }
    }
    else if(col == PRIMARY)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            cwrap->m_newval->m_pk = wyTrue;
            cwrap->m_newval->m_notnull = wyTrue;
        }
        else
        {
            cwrap->m_newval->m_pk = wyFalse;
        }
    }
    else if(col == BINARY)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_binary = wyTrue;
            else
                cwrap->m_newval->m_notnull = wyTrue;
        }
        else
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_binary = wyFalse;
            else
                cwrap->m_newval->m_notnull = wyFalse;
        }
    }
    else if(col == UNSIGNED)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_notnull = wyTrue;
            else
                cwrap->m_newval->m_unsigned = wyTrue;
        }
        else
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_notnull = wyFalse;
            else
                cwrap->m_newval->m_unsigned = wyFalse;
        }
    }
    else if(col == AUTOINCR)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_unsigned = wyTrue;
            else
                cwrap->m_newval->m_autoincr = wyTrue;
        }
        else
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_unsigned = wyFalse;
            else
                cwrap->m_newval->m_autoincr = wyFalse;
        }
    }
    else if(col == ZEROFILL)
    {
        if(stricmp(data, GV_TRUE)== 0) 
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_autoincr = wyTrue;
            else
                cwrap->m_newval->m_zerofill = wyTrue;
        }
        else
        {
            if(!m_ismysql41)
                cwrap->m_newval->m_autoincr = wyFalse;
            else
                cwrap->m_newval->m_zerofill = wyFalse;
        }
    }
	else if (col == CHECKCONSTRAINT)
	{
		if (m_ismariadb52)
			cwrap->m_newval->m_mycheckexpression.SetAs(data);
	}
    if(m_ismysql41 && data)
    {
        switch(col)
        {
        case CHARSET:
            cwrap->m_newval->m_charset.SetAs(data);
            break;
        
        case COLLATION:
            cwrap->m_newval->m_collation.SetAs(data);
            break;

        case COMMENT_:
            cwrap->m_newval->m_comment.SetAs(data);
            break;
        }
    }
	if((m_ismariadb52 || m_ismysql57) && data)
	{
	
	 switch(col)
        {
        case VIRTUALITY:
            if(m_ismariadb52)
				cwrap->m_newval->m_virtuality.SetAs(data);
			else
				cwrap->m_newval->m_mysqlvirtuality.SetAs(data);
            break;
        
        case EXPRESSION:
			if(m_ismariadb52)
				cwrap->m_newval->m_expression.SetAs(data);
			else
				cwrap->m_newval->m_mysqlexpression.SetAs(data);
			
            break;
        }
	}
	
}

wyBool
TabFields::ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata)
{
    wyString            newtext, origtext;
    FIELDATTRIBS        *fldattr = NULL;
    FieldStructWrapper* cwrapobj = NULL;
    wyUInt32            row;
    wyUInt32            col;
    
    wyBool              changed = wyFalse;

    row = currentrow;
    col = currentcol;

    cwrapobj = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);

    if(!cwrapobj)
        return wyFalse;

    if(!cwrapobj->m_oldval)
        return wyFalse;

    if(cwrapobj->m_oldval == cwrapobj->m_newval)
        return wyFalse;

    fldattr = cwrapobj->m_oldval;
    
    wyInt32 ncols = CustomGrid_GetColumnCount(m_hgridfields);

    for(int i=0; i < ncols; i++)
    {
        switch(i)
        {
        case CNAME:
            origtext.SetAs(fldattr->m_name);
            newtext.SetAs(cwrapobj->m_newval->m_name);
            break;

        case DATATYPE:
            origtext.SetAs(fldattr->m_datatype);
            newtext.SetAs(cwrapobj->m_newval->m_datatype);
            break;

        case LENGTH:
            origtext.SetAs(fldattr->m_len);
            newtext.SetAs(cwrapobj->m_newval->m_len);
            break;

        case DEFVALUE:
            origtext.SetAs(fldattr->m_default);
            newtext.SetAs(cwrapobj->m_newval->m_default);
            break;
		case ONUPDATECT:
			origtext.SetAs(fldattr->m_onupdate ? GV_TRUE : GV_FALSE);
            newtext.SetAs(cwrapobj->m_newval->m_onupdate ? GV_TRUE : GV_FALSE);
			break;

        case PRIMARY:
            continue;

        case BINARY:
            if(i == col && !currentdata.GetLength())
            {
                currentdata.SetAs(GV_FALSE);
            }
            if(!m_ismysql41)
            {               //..Binary Value
                origtext.SetAs(fldattr->m_binary ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_binary ? GV_TRUE : GV_FALSE);
            }
            else
            {               //..Else Not Null value (because, binary column will be absent. So Not_NUll will be at this index)
                origtext.SetAs(fldattr->m_notnull ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_notnull ? GV_TRUE : GV_FALSE);
            }
            
            break;

        case UNSIGNED:
            if(i == col && !currentdata.GetLength())
            {
                currentdata.SetAs(GV_FALSE);
            }
            if(!m_ismysql41)
            {
                origtext.SetAs(fldattr->m_notnull ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_notnull ? GV_TRUE : GV_FALSE);
            }
            else
            {
                origtext.SetAs(fldattr->m_unsigned ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_unsigned ? GV_TRUE : GV_FALSE);
            }
            
            break;

        case AUTOINCR:
            if(i == col && !currentdata.GetLength())
            {
                currentdata.SetAs(GV_FALSE);
            }
            if(!m_ismysql41)
            {
                origtext.SetAs(fldattr->m_unsigned ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_unsigned ? GV_TRUE : GV_FALSE);
            }
            else
            {
                origtext.SetAs(fldattr->m_autoincr ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_autoincr ? GV_TRUE : GV_FALSE);
            }
            
            break;

        case ZEROFILL:
            if(i == col && !currentdata.GetLength())
            {
                currentdata.SetAs(GV_FALSE);
            }
            if(!m_ismysql41)
            {
                origtext.SetAs(fldattr->m_autoincr ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_autoincr ? GV_TRUE : GV_FALSE);
            }
            else
            {
                origtext.SetAs(fldattr->m_zerofill ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_zerofill ? GV_TRUE : GV_FALSE);
            }
            
            break;

        case CHARSET:
            if(!m_ismysql41)
            {
                if(i == col && !currentdata.GetLength())
                {
                    currentdata.SetAs(GV_FALSE);
                }
                origtext.SetAs(fldattr->m_zerofill ? GV_TRUE : GV_FALSE);
                newtext.SetAs(cwrapobj->m_newval->m_zerofill ? GV_TRUE : GV_FALSE);
            }
            else
            {
                origtext.SetAs(fldattr->m_charset);
                newtext.SetAs(cwrapobj->m_newval->m_charset);
            }
            break;

        case COLLATION:
            origtext.SetAs(fldattr->m_collation);
            newtext.SetAs(cwrapobj->m_newval->m_collation);
            break;

        case COMMENT_:
            origtext.SetAs(fldattr->m_comment);
            newtext.SetAs(cwrapobj->m_newval->m_comment);
            break;

		case VIRTUALITY:
			if(m_ismariadb52){
			origtext.SetAs(fldattr->m_virtuality);
			newtext.SetAs(cwrapobj->m_newval->m_virtuality);}
			else{
			origtext.SetAs(fldattr->m_mysqlvirtuality);
			newtext.SetAs(cwrapobj->m_newval->m_mysqlvirtuality);
			}
            break;

		case EXPRESSION:
			if(m_ismariadb52){
			origtext.SetAs(fldattr->m_expression);
            newtext.SetAs(cwrapobj->m_newval->m_expression);
			}
			else{
			origtext.SetAs(fldattr->m_mysqlexpression);
			newtext.SetAs(cwrapobj->m_newval->m_mysqlexpression);
			}
            break;

		case CHECKCONSTRAINT:
			origtext.SetAs(fldattr->m_mycheckexpression);
			newtext.SetAs(cwrapobj->m_newval->m_mycheckexpression);
			break;
        }

        if(i == col)
            newtext.SetAs(currentdata);

        newtext.RTrim();
        if(origtext.Compare(newtext) != 0)
        {
            changed = wyTrue;
            break;
        }
    }

    if(!changed)
    {
        delete cwrapobj->m_newval;
        cwrapobj->m_newval = cwrapobj->m_oldval;
    }
    return wyTrue;
}

wyBool
TabFields::HandleTinyIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED + index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL + index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	if(m_ismysql41 == wyFalse)
	{	
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;

        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

    return wyTrue;
}

wyBool
TabFields::HandleBitValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
        cwrapobj->m_newval->m_autoincr = wyFalse;
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

    return wyTrue;
}

wyBool
TabFields::HandleBoolAndBooleanValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetText(hwndgrid, row, LENGTH, "");

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }
	return wyTrue;
}

wyBool
TabFields::HandleSmallIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
    
    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }
    return wyTrue;
}

wyBool
TabFields::HandleMediumIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }
	return wyTrue;
}

wyBool
TabFields::HandleIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }
	return wyTrue;
}

wyBool
TabFields::HandleIntegerValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }
	return wyTrue;
}

wyBool
TabFields::HandleBigIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleFloatValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleDoubleValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleRealValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED, GV_FALSE);

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;

        cwrapobj->m_newval->m_unsigned = wyFalse;
    }


	return wyTrue;
}

wyBool
TabFields::HandleDecimalValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
		
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;

        cwrapobj->m_newval->m_autoincr = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleNumericValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;

        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleDateValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
		cwrapobj->m_newval->m_onupdate = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
        if(m_ismysql41 == wyFalse)
            cwrapobj->m_newval->m_binary = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleDataTypeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	wyBool mysql565 = wyFalse;
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	if(IsMySQL565MariaDB1001(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql))
	{
		mysql565 = wyTrue;
		CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyFalse);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	}
	if(!IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql))
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
		CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	}
	else
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();

        if(m_ismysql41 == wyFalse)
        {
            cwrapobj->m_newval->m_binary = wyFalse;
        }
		if(!mysql565)
			cwrapobj->m_newval->m_onupdate = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
    }

	return wyTrue;
}

wyBool
TabFields::HandleTimeStampValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyFalse);
	if(!IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql))
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
		CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	}
	else
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();

        if(m_ismysql41 == wyFalse)
        {
            cwrapobj->m_newval->m_binary = wyFalse;
        }
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleTimeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(!IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql))
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
		CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	}
	else
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    if(cwrapobj && cwrapobj->m_newval)
    {    
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
        cwrapobj->m_newval->m_onupdate = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
    }

	
	return wyTrue;
}

wyBool
TabFields::HandleYearValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetText(hwndgrid, row, CHARSET, "");
    CustomGrid_SetText(hwndgrid, row, COLLATION, "");

	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);

    if(cwrapobj && cwrapobj->m_newval)
    {
        cwrapobj->m_newval->m_charset.Clear();
        cwrapobj->m_newval->m_collation.Clear();
        cwrapobj->m_newval->m_onupdate = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
    }

	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	return wyTrue;
}

wyBool
TabFields::HandleCharAndBinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isbinary)
{
    FieldStructWrapper *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);

    if(isbinary == wyTrue)
    {
        CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	    CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
        CustomGrid_SetText(hwndgrid, row, CHARSET, "");
        CustomGrid_SetText(hwndgrid, row, COLLATION, "");
    }
    else
    {
        CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
	    CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
    }
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );

	if(m_ismysql41 == wyFalse)
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	

	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );

    if(cwrapobj && cwrapobj->m_newval)
    {
        if(isbinary == wyTrue)
        {
            cwrapobj->m_newval->m_charset.Clear();
            cwrapobj->m_newval->m_collation.Clear();
        }
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
		cwrapobj->m_newval->m_onupdate = wyFalse;
    }

	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	return wyTrue;
}

wyBool
TabFields::HandleVarCharAndVarbinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isvarbinary)
{
    FieldStructWrapper *cwrapobj = NULL;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);

	if(isvarbinary == wyTrue)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
        CustomGrid_SetText(hwndgrid, row, CHARSET, "");
        CustomGrid_SetText(hwndgrid, row, COLLATION, "");
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );
    
	if(m_ismysql41 == wyFalse)
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
    
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
    CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
    
    if(cwrapobj && cwrapobj->m_newval)
    {
        if(isvarbinary == wyTrue)
        {
            cwrapobj->m_newval->m_charset.Clear();
            cwrapobj->m_newval->m_collation.Clear();
        }
		cwrapobj->m_newval->m_onupdate = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
    }

	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	return wyTrue;
}

wyBool
TabFields::HandleAllBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype)
{
    FieldStructWrapper *cwrapobj = NULL;

	// makes the charset and collation column readonly for the following datatype
	if ((stricmp (datatype , "tinytext" )  == 0 )  || (stricmp (datatype , "mediumtext" )  == 0 ) || (stricmp (datatype , "longtext" )  == 0 ))
    {
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
        CustomGrid_SetText(hwndgrid, row, CHARSET, "");
        CustomGrid_SetText(hwndgrid, row, COLLATION, "");
	}
//	if(stricmp(datatype,"json"))
	//	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyTrue);
		
    /*
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, PRIMARY, GV_FALSE);
    */
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyTrue );
	CustomGrid_SetText			(hwndgrid, row, DEFVALUE, "" );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue );
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "" );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
	if(stricmp(datatype,"json")==0)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY+ index, wyTrue );
	    CustomGrid_SetBoolValue     (hwndgrid, row, PRIMARY+ index, GV_FALSE );
	
	}
	if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        if (!(stricmp (datatype , "tinytext" )  == 0 )  || (stricmp (datatype , "mediumtext" )  == 0 ) || (stricmp (datatype , "longtext" )  == 0 ))
        {
            cwrapobj->m_newval->m_charset.Clear();
            cwrapobj->m_newval->m_collation.Clear();
        }
        if(m_ismysql41 == wyFalse)
        {
            cwrapobj->m_newval->m_binary = wyFalse;
        }
        cwrapobj->m_newval->m_pk = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
		cwrapobj->m_newval->m_default.SetAs("");
		cwrapobj->m_newval->m_onupdate = wyFalse;
    }
	return wyTrue;
}

wyBool
TabFields::HandleBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype)
{
    FieldStructWrapper *cwrapobj = NULL;
	// makes the charset and collation column readonly for the "text" datatype
	//if((stricmp (datatype, "text") == 0) || (stricmp (datatype, "json") == 0) )
	if((stricmp (datatype, "text") == 0))
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue );
		CustomGrid_SetText			(hwndgrid, row, LENGTH, "" );
	}
    /*
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, PRIMARY, GV_FALSE);
    */
	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyTrue );
	CustomGrid_SetText			(hwndgrid, row, DEFVALUE, "" );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        if((stricmp (datatype, "text") != 0))
        {
            cwrapobj->m_newval->m_charset.Clear();
            cwrapobj->m_newval->m_collation.Clear();
        }
        if(m_ismysql41 == wyFalse)
        {
            cwrapobj->m_newval->m_binary = wyFalse;
        }
        cwrapobj->m_newval->m_pk = wyFalse;
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
        cwrapobj->m_newval->m_len.Clear();
		cwrapobj->m_newval->m_default.SetAs("");
		cwrapobj->m_newval->m_onupdate = wyFalse;
    }

	return wyTrue;
}

wyBool
TabFields::HandleSetEnumValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    FieldStructWrapper *cwrapobj = NULL;

	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );

	if(m_ismysql41 == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse );
	
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ONUPDATECT, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ONUPDATECT, GV_FALSE);
    if(m_autoincrowid == row)
        m_autoincrowid = -1;

    cwrapobj = (FieldStructWrapper *)CustomGrid_GetRowLongData(m_hgridfields, row);
    if(cwrapobj && cwrapobj->m_newval)
    {
        if(m_ismysql41 == wyFalse)
        {
            cwrapobj->m_newval->m_binary = wyFalse;
        }
        cwrapobj->m_newval->m_unsigned = wyFalse;
        cwrapobj->m_newval->m_autoincr = wyFalse;
        cwrapobj->m_newval->m_zerofill = wyFalse;
		cwrapobj->m_newval->m_onupdate = wyFalse;
    }
	//m_isenumorset = wyTrue;

	return wyTrue;
}

wyBool
TabFields::SetValidation(wyUInt32 row, wyChar* datatype)
{
	wyUInt32	index = 0;
    HWND        hwndgrid = m_hgridfields;
	wyBool		isvarbinarydatatype = wyFalse, isbinarydatatype = wyFalse;

	if(m_ismysql41 == wyFalse)
		index = 1;
    else
        index = 0;

	if ((stricmp (datatype , "tinyint" ) == 0 ) )
	   return HandleTinyIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "bit" ) == 0 ) )
        return HandleBitValidation(hwndgrid, row, index);

	if ((stricmp (datatype , "bool" ) == 0 ) ||
		 (stricmp (datatype , "boolean" ) == 0 ) )

         return HandleBoolAndBooleanValidation(hwndgrid, row, index);
		
	if ((stricmp (datatype , "smallint" ) == 0 ) )
        return HandleSmallIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "mediumint") == 0 ) )
        return HandleMediumIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "int") == 0 ) )
        return HandleIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "integer") == 0 ) )
        return HandleIntegerValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "bigint") == 0 ) )
        return HandleBigIntValidation(hwndgrid, row, index);

	if ((stricmp (datatype , "float") == 0 ) )
        return HandleFloatValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "double") == 0 ) )
        return HandleDoubleValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "real" ) == 0 ) ) ) 
        return HandleRealValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "decimal" ) == 0 ) ) ) 
        return HandleDecimalValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "numeric" ) == 0 ) ) ) 
        return HandleNumericValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "date" ) == 0 ) ) ) 
        return HandleDateValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "datetime" ) == 0 ) ) )
        return HandleDataTypeValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "timestamp" ) == 0 ) ) ) 
        return HandleTimeStampValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "time" ) == 0 ) ) ) 
        return HandleTimeValidation(hwndgrid, row, index);
	
	if (stricmp (datatype , "year" ) == 0 )
        return HandleYearValidation(hwndgrid, row, index);

    if (stricmp (datatype , "binary" ) == 0) 
        isbinarydatatype = wyTrue;
	
	if (stricmp (datatype , "char" ) == 0 || isbinarydatatype == wyTrue)
        return HandleCharAndBinaryValidation(hwndgrid, row, index, isbinarydatatype);
	
	if(stricmp (datatype , "varbinary" ) == 0)
		isvarbinarydatatype = wyTrue;

	if((stricmp (datatype , "varchar" ) == 0 ) || isvarbinarydatatype == wyTrue)
        return HandleVarCharAndVarbinaryValidation(hwndgrid, row, index, isvarbinarydatatype);
	
	if ((stricmp (datatype , "tinyblob" )  == 0 )  ||
		 (stricmp (datatype , "tinytext" )  == 0 )  ||
		 (stricmp (datatype , "mediumblob" )  == 0 ) ||
		 (stricmp (datatype , "mediumtext" )  == 0  )||
		 (stricmp (datatype , "longblob" )  == 0 ) ||
		 (stricmp (datatype , "longtext" )  == 0 ) )
         return HandleAllBlobTextValidation(hwndgrid, row, index, datatype);
	

	if ((stricmp (datatype , "blob" ) == 0 )  ||
		 (stricmp (datatype , "text" ) == 0) ) 
         return HandleBlobTextValidation(hwndgrid, row, index, datatype);

	if(stricmp (datatype , "json" ) == 0)
		return HandleAllBlobTextValidation(hwndgrid, row, index, datatype);

	if ((stricmp (datatype , "set" )  == 0 )  ||
		 (stricmp (datatype , "enum" )  == 0 ) )
         return HandleSetEnumValidation(hwndgrid, row, index);
	
	return wyTrue;
}

/**
-Handle the enum dialog
-When user selects Enum/set datatype from Create/Alter table datatype combo
- When doubleclick/space-bar on Length coulmn
*/
wyBool			
TabFields::HandleEnumColumn(HWND hwndgrid, wyInt32 row, wyString& datatypestr)
{
	wyString	values;
	wyBool		isenum = wyFalse;
	ValueList   valuelist(&(m_mdiwnd->m_mysql));
    FieldStructWrapper *cwrapobj = NULL;

	if(stricmp(datatypestr.GetString(),"enum") == 0 || stricmp(datatypestr.GetString(),"set") == 0)
	{
		if(stricmp(datatypestr.GetString(),"enum") == 0)
			isenum = wyTrue;
		else 
			isenum = wyFalse;
		
        GetGridCellData(m_hgridfields, row, 2, values);
        SetFocus(m_hgridfields);
		
        /// Opening value list dialogbox.
        if(valuelist.Create(m_hgridfields, &values, isenum))
        {
            if(m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
            {
                m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
            }

		    CustomGrid_SetText(m_hgridfields, row, 2, values.GetString());
            //CustomGrid_SetColumnReadOnly(m_hgridfields, row, LENGTH, wyTrue);
            cwrapobj = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row);
            if(cwrapobj)
            {
                if(cwrapobj->m_oldval == cwrapobj->m_newval || !cwrapobj->m_newval)
                {
                    cwrapobj->m_newval = GetDuplicateFieldAttribsStruct(cwrapobj->m_oldval);
                }
                cwrapobj->m_newval->m_len.SetAs(values);
                ScanEntireRow(row, LENGTH, values);
            }
        }
	}
	return wyTrue;
}

LRESULT CALLBACK
TabFields::GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TabFields* tabfields = (TabFields*)CustomGrid_GetLongData(hwnd);
    wyString			tblname("__create_table");

    switch(message)
	{
    case GVN_PASTECLIPBOARDBEGIN:
        return TRUE;

    case GVN_DESTROY:
        {
            tabfields->ClearAllMemory();
            tabfields->m_hgridfields = NULL;
        }
        break;

	case GVN_BEGINLABELEDIT:
		return tabfields->OnGVNBeginLabelEdit(wParam,lParam);

	case GVN_ENDLABELEDIT:
		return tabfields->OnGVNEndLabelEdit(wParam,lParam);

	case GVN_ENDADDNEWROW:
        return TRUE;

	case GVN_SPLITTERMOVE:
		OnGridSplitterMove(hwnd, &tblname, wParam);	
		break;
        
	case GVN_FINISHENDLABELEDIT:
		if(lParam == 1)
		{
            wyString curdatastr;

            tabfields->GetGridCellData(tabfields->m_hgridfields, LOWORD(wParam), DATATYPE, curdatastr);
            if((curdatastr.CompareI("enum") == 0 || curdatastr.CompareI("set") == 0))
			    tabfields->HandleEnumColumn(hwnd, LOWORD(wParam), curdatastr);
		}
		break;
        
    case GVN_CHECKBOXCLICK:
        {
            tabfields->HandleCheckboxClick(hwnd, lParam, wParam);
        }
        break;

    case GVN_ENDCHANGEROW:
        {
            tabfields->OnGVNEndRowChange(wParam);
        }
        break;

    case GVN_DRAWROWCHECK:
        {
            ((GVROWCHECKINFO*)lParam)->ischecked = CustomGrid_GetRowCheckState(hwnd, wParam) ? wyTrue : wyFalse;
        }
        break;

    case GVN_NEXTTABORDERITEM:
        SetFocus(GetNextDlgTabItem(tabfields->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, FALSE));
        break;

    case GVN_PREVTABORDERITEM:
        SetFocus(GetNextDlgTabItem(tabfields->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, TRUE));
        break;

    case GVN_SYSKEYDOWN:
        return tabfields->OnGVNSysKeyDown(hwnd, wParam, lParam);

    //case GVN_SELECTALLCLICK:
    //    tabfields->m_
	}

	return 1;
}

LRESULT
TabFields::OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if(m_ptabmgmt->OnSysKeyDown(hwnd, wParam, lParam) == wyTrue)
    {
        return 1;
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        return 1;

    return m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(0, wParam, lParam);
}

void
TabFields::OnGVNEndRowChange(WPARAM wParam)
{
    wyInt32 row = wParam;
    wyInt32 nrows = CustomGrid_GetRowCount(m_hgridfields);

    if(row == 0)
        SendMessage(m_ptabmgmt->m_hwndtool, TB_SETSTATE, IDI_MOVEUP, (LPARAM)TBSTATE_INDETERMINATE);
    else
        SendMessage(m_ptabmgmt->m_hwndtool, TB_SETSTATE, IDI_MOVEUP, (LPARAM)TBSTATE_ENABLED);

    if(row == nrows - 1)
        SendMessage(m_ptabmgmt->m_hwndtool, TB_SETSTATE, IDI_MOVEDOWN, (LPARAM)TBSTATE_INDETERMINATE);
    else
        SendMessage(m_ptabmgmt->m_hwndtool, TB_SETSTATE, IDI_MOVEDOWN, (LPARAM)TBSTATE_ENABLED);
}

void
TabFields::HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam)
{
    wyInt32     ret, startpos, endpos, flag;

    ret = GetKeyState(VK_SHIFT);// shift key is pressed or not

    if(m_lastclick == -1 || !(ret & 0x8000))   //..If first time, any checkbox is clicked or shift key is not pressed, then no multiple selection required..
    {
        m_lastclick = wparam;
        return;
    }

    if(m_lastclick == wparam)
        return;

    if(wparam < m_lastclick)
    {
        startpos = wparam;
        endpos = m_lastclick;
    }
    else
    {
        startpos = m_lastclick;
		endpos = wparam;
    }

    flag = CustomGrid_GetRowCheckState(m_hgridfields, wparam);

    while(startpos <= endpos)
    {
        CustomGrid_SetRowCheckState(m_hgridfields, startpos, flag == GV_CHEKCED ? wyTrue : wyFalse);
        startpos++;
    }
    m_lastclick = wparam;
}

void
TabFields::CancelChanges(wyBool    isaltertable)
{
    wyUInt32    count = -1, row = -1;
    FieldStructWrapper *cwrapobj = NULL,*cwrapobj_2 = NULL;

    /// Applying grid changes
    CustomGrid_ApplyChanges(m_hgridfields);

    count = CustomGrid_GetRowCount(m_hgridfields);

    for(row = 0; row < count; row++)
    {
        cwrapobj = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hgridfields, row);
        if(cwrapobj && cwrapobj->m_newval && !cwrapobj->m_oldval)                           //..Create/Alter table (If new (valid)field is added)
        {
            m_listwrapperstruct.Remove(cwrapobj);
            delete cwrapobj;
        }
        else if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_oldval)                           //..Alter table (If field is added)
        {
            if(cwrapobj->m_newval != cwrapobj->m_oldval)                                    //..If field modified, delete m_newval, and set it as m_oldval
                delete cwrapobj->m_newval;

            cwrapobj->m_newval = cwrapobj->m_oldval;
        }
    }

    cwrapobj = (FieldStructWrapper*)m_listwrapperstruct.GetFirst();

	cwrapobj_2 = (FieldStructWrapper*) m_listwrapperstruct_2.GetFirst();
    while(cwrapobj_2)
    {
        //cwrapobjtemp = cwrapobj;
        cwrapobj_2 = (FieldStructWrapper*) m_listwrapperstruct_2.Remove(cwrapobj_2);
        
        //delete cwrapobjtemp;
    }
	
    while(cwrapobj)
    {
        cwrapobj->m_errmsg.Clear();
        cwrapobj->m_newval = cwrapobj->m_oldval;
		cwrapobj->m_ischanged = wyFalse;
		cwrapobj_2 = new FieldStructWrapper(cwrapobj->m_oldval, wyFalse);
		m_listwrapperstruct_2.Insert(cwrapobj_2);
        //..removing indexcolumns working copy elements
        IndexedBy *indexedby = (IndexedBy *)cwrapobj->m_listindexesworkingcopy.GetFirst(), *tmpindby = NULL;
        while(indexedby)
        {
            tmpindby = indexedby;
            indexedby = (IndexedBy *)cwrapobj->m_listindexesworkingcopy.Remove(indexedby);
            delete tmpindby;
        }

        //..removing fkcolumns working copy elements
        ReferencedBy *refby = (ReferencedBy*) cwrapobj->m_listfkworkingcopy.GetFirst(), *tmprefby = NULL;
        while(refby)
        {
            tmprefby = refby;
            refby = (ReferencedBy *)cwrapobj->m_listfkworkingcopy.Remove(refby);
            delete tmprefby;
        }

        cwrapobj = (FieldStructWrapper*)cwrapobj ->m_next;
    }

    m_autoincrowid = -1;        //..Must be before FillInitData();
    m_lastclick     =   -1;

    /// Adding initial rows for create table and returning
    if(!isaltertable)
    {
        CustomGrid_DeleteAllRow(m_hgridfields, wyTrue);
        
        for(int i=0; i<6; i++)
            CustomGrid_InsertRow(m_hgridfields, wyTrue);
        InvalidateRect(m_hgridfields, NULL, TRUE);
        return;
    }

    CustomGrid_DeleteAllRow(m_hgridfields, wyTrue);

    /// Setting original Values to the grid (Alter table)
    FillInitData();
    
    if(m_ptabmgmt->GetActiveTabImage() == IDI_COLUMN)
    {
        SetFocus(m_hgridfields);
        CustomGrid_SetCurSelection(m_hgridfields, 0, 0, wyTrue);
    }
    return;
}

void
TabFields::ClearAllMemory()
{
    FieldStructWrapper* cwrapobj = NULL, *cwrapobjtemp = NULL;

    /// Removing all wrappers from the list
    cwrapobj = (FieldStructWrapper*) m_listwrapperstruct.GetFirst();
    while(cwrapobj)
    {
        cwrapobjtemp = cwrapobj;
        cwrapobj = (FieldStructWrapper*) m_listwrapperstruct.Remove(cwrapobjtemp);
        
        delete cwrapobjtemp;
    }

    cwrapobj = (FieldStructWrapper*) m_listwrapperstruct_2.GetFirst();
    while(cwrapobj)
    {
        cwrapobjtemp = cwrapobj;
        cwrapobj = (FieldStructWrapper*) m_listwrapperstruct_2.Remove(cwrapobjtemp);
        
        //delete cwrapobjtemp;
    }
    return;
}

void 
TabFields::OnClickMoveUp()
{
    wyInt32     selrow = -1, totrows = -1;
    wyInt32     selcol = -1;
	FieldStructWrapper* cwrapobj1 = NULL;
    selrow  = CustomGrid_GetCurSelRow(m_hgridfields);
    selcol  = CustomGrid_GetCurSelCol(m_hgridfields);
    totrows = CustomGrid_GetRowCount(m_hgridfields);

    CustomGrid_ApplyChanges(m_hgridfields, wyTrue);

    if(selrow <=0)
        return;
	cwrapobj1 = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, selrow);
	if(cwrapobj1)
		cwrapobj1->m_ischanged = wyTrue;
    /// Exchangin row values
    ExchangeRowValues(selrow - 1, selrow);

    /// Enabling/disabling toolbar buttons
    OnGVNEndRowChange((WPARAM)selrow - 1);

    CustomGrid_SetCurSelRow(m_hgridfields, selrow-1, wyTrue);
    CustomGrid_EnsureVisible(m_hgridfields, selrow - 1, selcol);
}

void 
TabFields::OnClickMoveDown()
{
    wyInt32     selrow = -1, totrows = -1;
    wyInt32     selcol = -1;
	FieldStructWrapper* cwrapobj1 = NULL;
    selrow  = CustomGrid_GetCurSelRow(m_hgridfields);
    selcol  = CustomGrid_GetCurSelCol(m_hgridfields);
    totrows = CustomGrid_GetRowCount(m_hgridfields);

    if(selrow == totrows - 1 || selrow  == -1)
        return;
    
    CustomGrid_ApplyChanges(m_hgridfields, wyTrue);
	cwrapobj1 = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, selrow);
	if(cwrapobj1)
		cwrapobj1->m_ischanged = wyTrue;
    /// Exchangin row values
    ExchangeRowValues(selrow, selrow + 1);

    /// Enabling/disabling toolbar buttons
    OnGVNEndRowChange((WPARAM)selrow + 1);

    CustomGrid_SetCurSelRow(m_hgridfields, selrow + 1, wyTrue);
    CustomGrid_EnsureVisible(m_hgridfields, selrow + 1, selcol);
}

wyUInt32
TabFields::GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata)
{
    wyString    tempstr("");
    wyWChar     *data;
    wyUInt32    celldatalen = 0;

    celldatalen = 0;
    celldatalen = CustomGrid_GetItemTextLength(hwndgrid, row, col);
    if(celldatalen)
    {
        data = (wyWChar*)malloc(sizeof(wyWChar) * (celldatalen + 1));
        data[0] = '\0';

        CustomGrid_GetItemText(hwndgrid, row, col, data);
        tempstr.SetAs(data);
        free(data);
    }
    else
        tempstr.SetAs("");
    tempstr.RTrim();
    celldata.SetAs(tempstr);
    return celldata.GetLength();
}

void 
TabFields::ExchangeRowValues(wyInt32 row1, wyInt32 row2)
{
    FieldStructWrapper  *cwrapobj1 = NULL, *cwrapobj2 = NULL;
    wyString        data1(""), data2(""), datatype1(""), datatype2("");
    wyInt32         ncols = -1, temp;
    wyBool          boolval1, boolval2;
    
    ncols = CustomGrid_GetColumnCount(m_hgridfields);

    if(row1 > row2)
    {
        temp = row1;
        row1 = row2;
        row2 = temp;
    }

    if(row1 == m_autoincrowid)
        m_autoincrowid = row2;
    else if(row2 == m_autoincrowid)
        m_autoincrowid = row1;

    //..Column Name
    GetGridCellData(m_hgridfields, row1, CNAME, data1);
    GetGridCellData(m_hgridfields, row2, CNAME, data2);
    CustomGrid_SetText(m_hgridfields, row1, CNAME, (wyChar*)data2.GetString());
    CustomGrid_SetText(m_hgridfields, row2, CNAME, (wyChar*)data1.GetString());

    //..Datatype
    GetGridCellData(m_hgridfields, row1, DATATYPE, datatype1);
    GetGridCellData(m_hgridfields, row2, DATATYPE, datatype2);
    CustomGrid_SetText(m_hgridfields, row1, DATATYPE, (wyChar*)datatype2.GetString());
    CustomGrid_SetText(m_hgridfields, row2, DATATYPE, (wyChar*)datatype1.GetString());

    //..Size
    GetGridCellData(m_hgridfields, row1, LENGTH, data1);
    GetGridCellData(m_hgridfields, row2, LENGTH, data2);
    CustomGrid_SetText(m_hgridfields, row1, LENGTH, data2.GetString());
    CustomGrid_SetText(m_hgridfields, row2, LENGTH, data1.GetString());

    //..Default
    GetGridCellData(m_hgridfields, row1, DEFVALUE, data1);
    GetGridCellData(m_hgridfields, row2, DEFVALUE, data2);
    CustomGrid_SetText(m_hgridfields, row1, DEFVALUE, data2.GetString());
    CustomGrid_SetText(m_hgridfields, row2, DEFVALUE, data1.GetString());

	GetGridCellData(m_hgridfields, row1, CHECKCONSTRAINT, data1);
	GetGridCellData(m_hgridfields, row2, CHECKCONSTRAINT, data2);
	CustomGrid_SetText(m_hgridfields, row1, CHECKCONSTRAINT, data2.GetString());
	CustomGrid_SetText(m_hgridfields, row2, CHECKCONSTRAINT, data1.GetString());

	if(m_ismariadb52 || m_ismysql57)
	{
	 GetGridCellData(m_hgridfields, row1, VIRTUALITY, data1);
	 GetGridCellData(m_hgridfields, row2, VIRTUALITY, data2);
	 CustomGrid_SetText(m_hgridfields, row1, VIRTUALITY, data2.GetString());
     CustomGrid_SetText(m_hgridfields, row2, VIRTUALITY, data1.GetString());

	 GetGridCellData(m_hgridfields, row1, EXPRESSION, data1);
	 GetGridCellData(m_hgridfields, row2, EXPRESSION, data2);
	 CustomGrid_SetText(m_hgridfields, row1, EXPRESSION, data2.GetString());
     CustomGrid_SetText(m_hgridfields, row2, EXPRESSION, data1.GetString());

	}

    for(int i=0; i<6; i++)
    {
        if(m_ismysql41 && i == 5)
            continue;
        boolval1 = CustomGrid_GetBoolValue(m_hgridfields, row1, PRIMARY + i);
        boolval2 = CustomGrid_GetBoolValue(m_hgridfields, row2, PRIMARY + i);
        CustomGrid_SetBoolValue(m_hgridfields, row1, PRIMARY + i, boolval2 ? GV_TRUE : GV_FALSE);
        CustomGrid_SetBoolValue(m_hgridfields, row2, PRIMARY + i, boolval1 ? GV_TRUE : GV_FALSE);
    }
    
    if(m_ismysql41)
    {
        GetGridCellData(m_hgridfields, row1, CHARSET, data1);
        GetGridCellData(m_hgridfields, row2, CHARSET, data2);
        CustomGrid_SetText(m_hgridfields, row1, CHARSET, data2.GetString());
        CustomGrid_SetText(m_hgridfields, row2, CHARSET, data1.GetString());

        GetGridCellData(m_hgridfields, row1, COLLATION, data1);
        GetGridCellData(m_hgridfields, row2, COLLATION, data2);
        CustomGrid_SetText(m_hgridfields, row1, COLLATION, data2.GetString());
        CustomGrid_SetText(m_hgridfields, row2, COLLATION, data1.GetString());

        GetGridCellData(m_hgridfields, row1, COMMENT_, data1);
        GetGridCellData(m_hgridfields, row2, COMMENT_, data2);
        CustomGrid_SetText(m_hgridfields, row1, COMMENT_, data2.GetString());
        CustomGrid_SetText(m_hgridfields, row2, COMMENT_, data1.GetString());

		//on Update value

		boolval1 = CustomGrid_GetBoolValue(m_hgridfields, row1, ONUPDATECT);
        boolval2 = CustomGrid_GetBoolValue(m_hgridfields, row2, ONUPDATECT);
        CustomGrid_SetBoolValue(m_hgridfields, row1, ONUPDATECT, boolval2 ? GV_TRUE : GV_FALSE);
        CustomGrid_SetBoolValue(m_hgridfields, row2, ONUPDATECT, boolval1 ? GV_TRUE : GV_FALSE);
    }

    //..Row Long Data
    cwrapobj1 = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row1);
    cwrapobj2 = (FieldStructWrapper *) CustomGrid_GetRowLongData(m_hgridfields, row2);
    
    if((cwrapobj1 && cwrapobj2) && !m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

    if((cwrapobj1 && cwrapobj2 && ((cwrapobj1->m_oldval && !cwrapobj2->m_oldval) || (!cwrapobj1->m_oldval && cwrapobj2->m_oldval) || (!cwrapobj1->m_oldval && !cwrapobj2->m_oldval))))
    {
        m_listwrapperstruct.Remove(cwrapobj2);
        m_listwrapperstruct.InsertBefore(cwrapobj1, cwrapobj2);
    }
    

    CustomGrid_SetRowLongData(m_hgridfields, row1, (LPARAM) cwrapobj2);
    CustomGrid_SetRowLongData(m_hgridfields, row2, (LPARAM) cwrapobj1);

    boolval1 = (wyBool)CustomGrid_GetRowCheckState(m_hgridfields, row1);
    boolval2 = (wyBool)CustomGrid_GetRowCheckState(m_hgridfields, row2);

    CustomGrid_SetRowCheckState(m_hgridfields, row1, boolval2);
    CustomGrid_SetRowCheckState(m_hgridfields, row2, boolval1);

    cwrapobj1 = (FieldStructWrapper *) m_listwrapperstruct.GetFirst();

    SetValidation(row1, (wyChar*)datatype2.GetString());
    SetValidation(row2, (wyChar*)datatype1.GetString());
}

void
TabFields::OnTabClosing()
{
    /// Clearing all memory
    ClearAllMemory();
}

//for hiding the charset and collation
void 
TabFields::ShowHideCharsetAndCollation()
{	
	wyInt32		curselcol;

	CustomGrid_ApplyChanges(m_hgridfields);
	
    if(SendMessage(m_ptabmgmt->m_tabfields->m_hchkboxhidelanguageoptions, BM_GETCHECK, 0,0) == BST_CHECKED)
	{		
		CustomGrid_ShowOrHideColumn(m_hgridfields, CHARSETCOL, wyFalse);
		CustomGrid_ShowOrHideColumn(m_hgridfields, COLLATIONCOL, wyFalse);

		//if the current selection is in charset/collation column 
		//then after hiding we will set the selection to the comment column 
		curselcol = CustomGrid_GetCurSelCol(m_hgridfields);
		if(curselcol == CHARSETCOL || curselcol == COLLATIONCOL)
			CustomGrid_SetCurSelCol(m_hgridfields, COMMENT_, wyFalse);
	}
	else
	{
		CustomGrid_ShowOrHideColumn(m_hgridfields, CHARSETCOL, wyTrue);
		CustomGrid_ShowOrHideColumn(m_hgridfields, COLLATIONCOL, wyTrue);
	}
}

void
TabFields::OnPrimaryIndexChange()
{
    wyInt32                 selrow, nrows = -1, row = -1;
    wyInt32                 index = 0, addcol = 0;
    FieldStructWrapper      *cwrapobj = NULL;
    wyString                celldata;
    HWND                    hgridindexes = m_ptabmgmt->m_tabindexes->m_hgridindexes;

    if(m_ismysql41)
		index = 1;
    else
        addcol = 1;

    selrow = m_ptabmgmt->m_tabindexes->m_automatedindexrow;
    nrows = CustomGrid_GetRowCount(m_hgridfields);

    /// Unchecking "PK?" in all rows
    for(row = 0; row < nrows; row++)
    {
        CustomGrid_SetBoolValue(m_hgridfields, row, PRIMARY, GV_FALSE);
        CustomGrid_SetColumnReadOnly(m_hgridfields, row, NOTNULL + addcol, wyFalse);
    }
    
    if(selrow == -1)
        return;
    /// Getting the list of index-columns
    List *list = (List*)CustomGrid_GetItemLongValue(hgridindexes, selrow, 1);

    for(row = 0; (row<nrows) && (list); row++)
    {
        cwrapobj = (FieldStructWrapper*) CustomGrid_GetRowLongData(m_hgridfields, row);

        if(!cwrapobj)
            continue;

        IndexColumn *indcols = (IndexColumn*)list->GetFirst();

        /// rotating through all index-columns and setting m_pk member variable
        while(indcols)
        {
            if(indcols->m_pcwrapobj == cwrapobj)
            {
                CustomGrid_SetBoolValue(m_hgridfields, row, PRIMARY, GV_TRUE);
                CustomGrid_SetBoolValue(m_hgridfields, row, NOTNULL + addcol, GV_TRUE);
                if(cwrapobj->m_newval == cwrapobj->m_oldval)
                    cwrapobj->m_newval = GetDuplicateFieldAttribsStruct(cwrapobj->m_oldval);
                
                cwrapobj->m_newval->m_pk = wyTrue;
                cwrapobj->m_newval->m_notnull = wyTrue;
                wyString temp(GV_TRUE);
                ScanEntireRow(row, NOTNULL + addcol, temp);

                CustomGrid_SetColumnReadOnly(m_hgridfields, row, NOTNULL + addcol, wyFalse);
            }
            indcols = (IndexColumn*)indcols->m_next;
        }
    }
}

void
TabFields::ReInitializeGrid()
{
    ClearAllMemory();
    CustomGrid_DeleteAllRow(m_hgridfields);
    SetInitValues();
}

FieldStructWrapper*
TabFields::GetWrapperObjectPointer(wyString& columnname)
{
    FieldStructWrapper* cwrapobj = NULL;

    cwrapobj  = (FieldStructWrapper*) m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        /// Checks whether the columnname column is not dropped
        if(cwrapobj->m_newval && (cwrapobj->m_newval->m_name.CompareI(columnname) == 0))
            return cwrapobj;
        cwrapobj = (FieldStructWrapper*)cwrapobj->m_next;
    }
    return NULL;
}

wyBool
TabFields::SelectColumn(wyString &colname)
{
    wyUInt32    row, nrows;
    wyString    celldata;

    nrows = CustomGrid_GetRowCount(m_hgridfields);

    for(row = 0; row < nrows; row++)
    {
        GetGridCellData(m_hgridfields, row, CNAME, celldata);
        if(celldata.Compare(colname) == 0)
        {
            CustomGrid_SetCurSelection(m_hgridfields, row, CNAME);
            return wyTrue;
        }
    }

    return wyFalse;
}