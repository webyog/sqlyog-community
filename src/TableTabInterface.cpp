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

#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabEditorSplitter.h"
#include "TabFields.h"
#include "MySQLVersionHelper.h"
#include "TabIndexes.h"
#include "TabForeignKeys.h"
#include "TabAdvancedProperties.h"
#include "TabPreview.h"
#include "DoubleBuffer.h"
#include "TabCheck.h"

#define	SIZE_24	        24
#define	SIZE_12	        12
#define BK_COLOR		RGB(255, 255, 255)

#define VK_KEY_0        0x30
#define VK_KEY_1        0x31
#define VK_KEY_2        0x32
#define VK_KEY_3        0x33
#define VK_KEY_4        0x34
#define VK_KEY_5        0x35
#define VK_KEY_9        0x39

extern	PGLOBALS		pGlobals;

#define IDC_TABMGMT		1234

/*
#define EDITRELERROR _(L"There in no direct way to ALTER a Foreign Key in MySQL.\n"\
L"SQLyog will drop the existing one and create a new one using the existing name for the key.\n"\
L"If necessary preconditions for a Foreign Key (matching data types, appropriate indexes etc.) are not met, the create process will fail."\
L"The result will be that existing FK is dropped and no new one is created.\n"\
L"You can avoid this by specifying a new name for the key.\n\n"\
L"Do you want to continue?")
*/
TableTabInterface::TableTabInterface(HWND hwnd, wyBool open_in_dialog, wyBool isaltertable, wyInt32 setfocustotab):TabTypes(hwnd)
{
    m_isbuffereddraw        =   wyFalse;
    m_hwnd                  =   NULL;
    m_hwndparent            =   hwnd;
    m_ptabintmgmt           =   NULL;
    m_isaltertable          =   isaltertable;
    m_open_in_dialog        =   open_in_dialog;
	m_setfocustotab         =   setfocustotab;
    m_isfksupported         =   wyTrue;
	m_isfkforndbcluster		=	wyFalse;

    m_dirtytab              =   wyFalse;
    m_disableenchange       =   wyFalse;

    m_hstattblname          =   NULL;
    m_hstatdbname           =   NULL;
    m_hstattabletype        =   NULL;
    m_hstatcharset          =   NULL;
    m_hstatcollation        =   NULL;
    m_hedittblname          =   NULL;
    m_hcmbdbname            =   NULL;
    m_hcmbtabletype         =   NULL;
    m_hcmbcharset           =   NULL;
    m_hcmbcollation         =   NULL;
	m_hbtncancel			=	NULL;

    //Used for SD Created/Alter table Dialog-box
    m_issdempty             =   wyTrue;
    m_listoftablescreated   =   NULL;
    m_ntablescreated        =   0;
    m_istablealtered        =   wyFalse;

    m_hbtncancelchanges     =   NULL;
    m_hbtnsave              =   NULL;

    m_myrestablestatus      =   NULL;

    m_wporigtblname         =   NULL;
    m_wporigdbname          =   NULL;
    m_wporigengine          =   NULL;
    m_wporigcharset         =   NULL;
    m_wporigcollate         =   NULL;
    m_wporigbtnsave         =   NULL;
    m_wporigbtncancel       =   NULL;
	m_wporigbtncancelchanges =  NULL;

    m_wporigbtnsave         =   NULL;

	if(open_in_dialog == wyFalse)
		m_objbkcolor            =    CreateSolidBrush(BK_COLOR);
	else
		m_objbkcolor            =    CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	m_mdiwnd					= GetActiveWin();
}

TableTabInterface::~TableTabInterface()
{
    if(m_myrestablestatus)
        m_mdiwnd->m_tunnel->mysql_free_result(m_myrestablestatus);
    //..Deleting Window
    if(m_hwnd)
    {
        VERIFY(DestroyWindow(m_hwnd));
        m_hwnd = NULL;
    }
    
    DeleteObject(m_objbkcolor);


    if(m_ptabintmgmt)
        delete m_ptabintmgmt;
    
    m_ptabintmgmt = NULL;


}

void
TableTabInterface::TabInterfaceTitle(wyString *tunneltitle)
{
    if(m_dirtytab)
    {
        if(tunneltitle->GetCharAt(tunneltitle->GetLength()-1) != '*')
            tunneltitle->Add("*");
    }
    else if(tunneltitle->GetCharAt(tunneltitle->GetLength()-1) == '*')
        tunneltitle->Strip(1);
}

wyBool
TableTabInterface::Create()
{
    wyBool                  error = wyFalse;
    wyUInt32 styles = WS_CHILD | WS_VISIBLE;
    HINSTANCE   hinst   = pGlobals->m_entinst?pGlobals->m_entinst:pGlobals->m_hinstance;

	m_ismysql41             =   m_mdiwnd->m_ismysql41;
	

    //..Setting dbname and origtablename from the object browser
    if(!m_open_in_dialog)
        m_dbname.SetAs(m_mdiwnd->m_pcqueryobject->m_seldatabase);
    
    if(m_isaltertable && !m_open_in_dialog)
        m_origtblname.SetAs(m_mdiwnd->m_pcqueryobject->m_seltable);

    if(m_isaltertable)
    {
        if(!GetMyResTableStatus())
            return wyFalse;

        if(!m_myrestablestatus)
            return wyFalse;

        //..Checking whether FK is supported or not
        error = wyFalse;
        m_isfksupported = IsTableInnoDB(error);
        if(error)
            return wyFalse;

        if(!m_isfksupported)
        {
            if(m_setfocustotab == TABFK)
            {
                MessageBox(m_hwnd, _(L"The selected table does not support foreign keys.\nTable engine must be InnoDB, PBXT, SolidDB or ndbcluster (if ndbcluster engine version is greater than or equal to 7.3)."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
				return wyFalse;
            }
        }
    }
    
	m_hwnd              = CreateWindowEx(m_open_in_dialog ? WS_EX_CONTROLPARENT : 0, TABLE_TABINTERFACE_WINDOW, L"", 
									styles, 
									0, 0, 0, 0, 
									m_hwndparent, (HMENU)0, 
									hinst, this);
	
    if(!m_hwnd)
        return wyFalse;

    CreateOtherWindows();
    if(m_isaltertable)
        EnableWindow(m_hcmbdbname, FALSE);

    GetAllDatabases();
	SelectDatabase();
    InitTableTypeValues(m_isaltertable ? wyFalse : wyTrue);
    if(m_ismysql41 == wyFalse)
    {
        ShowWindow(m_hstatcharset, SW_HIDE);
        ShowWindow(m_hcmbcharset, SW_HIDE);
        ShowWindow(m_hstatcollation, SW_HIDE);
        ShowWindow(m_hcmbcollation, SW_HIDE);
    }                                                                           
    else
    {
        InitCharacterSetCombo(m_isaltertable ? wyFalse : wyTrue);
        InitCollationCombo(m_isaltertable ? wyFalse : wyTrue);
    }

    if(!CreateTabMgmt())
        return wyFalse;

    CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfields->m_hgridfields, 0, 0);
    CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabindexes->m_hgridindexes, 0, 1);
	CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabcheck->m_hgridtblcheckconst, 0, 1);
    CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfk->m_hgridfk, 0, 1);
	

    if(m_setfocustotab == -1)
    {
        m_ptabintmgmt->SelectTab(0);
        PostMessage(m_hwnd, UM_SETFOCUS, (LONG) m_hedittblname, NULL);
    }
    else
    {
        m_ptabintmgmt->SelectTab(m_setfocustotab);
		
		//workaround for table name not displaying in wine after tab selection
		SetWindowText(m_hedittblname, m_origtblname.GetAsWideChar());
        SetInitFocus();
    }

	if(!m_open_in_dialog)
		Resize();

    ShowWindow(m_hwnd, SW_SHOW);
    
    MarkAsDirty(wyFalse);
    return wyTrue;
}

void
TableTabInterface::InitCollationCombo(wyBool includedefault)
{
    wyString    query, collationstr;
    wyInt32     index;
    
    HWND        hwndcombo = m_hcmbcollation;
   
    if(includedefault)
    {
        if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
            index = SendMessage(hwndcombo, CB_SETCURSEL, index, (LPARAM)TEXT(STR_DEFAULT));
    }
}

wyInt32 
TableTabInterface::OnWmNotify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	LPNMHDR			lpnmhdr =(LPNMHDR)lparam;
	LPNMCTC         lpnmctc;
	
    switch(lpnmhdr->code)
	{
        case CTCN_SELCHANGING:
        {
			lpnmctc = (LPNMCTC)lparam;
            if(lpnmctc->count == 2 && !m_isfksupported && m_isaltertable)
            {
				MessageBox(m_hwnd, _(L"The selected table does not support foreign keys.\nTable engine must be InnoDB, PBXT, SolidDB or ndbcluster (if ndbcluster engine version is greater than or equal to 7.3)."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
				return 0;
			}

            return m_ptabintmgmt->OnTabSelChanging();
        }
        break;

        case CTCN_SELCHANGE:
        {
            m_ptabintmgmt->OnTabSelChange();
            return 1;
        }

		case CTCN_PAINTTIMERSTART:
            m_ptabintmgmt->m_isbuffereddraw = wyTrue;
            CustomTab_SetBufferedDrawing(wyTrue);
            return 1;

        case CTCN_PAINTTIMEREND:
            CustomTab_SetBufferedDrawing(wyFalse);
            m_ptabintmgmt->m_isbuffereddraw = wyFalse;
            return 1;
    }

    return 0;
}

List*
TableTabInterface::GetListOfUnsavedWrappers(wyInt32 initrow)
{
    List                *listunsavedwrps = NULL;
    FKStructWrapper     *fkwrap = NULL;
    wyInt32             nrows = CustomGrid_GetRowCount(m_ptabintmgmt->m_tabfk->m_hgridfk);
    wyInt32             row = -1;

    fkwrap = (FKStructWrapper*)CustomGrid_GetRowLongData(m_ptabintmgmt->m_tabfk->m_hgridfk, initrow);

    //..Storing the first failed FK wrapper
    if(fkwrap)
    {
        if(!listunsavedwrps)
            listunsavedwrps = new List();

        m_ptabintmgmt->m_tabfk->m_listfkwrappers.Remove(fkwrap);
        m_ptabintmgmt->m_tabfk->ClearListSrcCols(fkwrap, fkwrap->m_newval->m_listsrccols);
        listunsavedwrps->Insert(fkwrap);
    }

    for(row = (initrow + 1); row < nrows; row++)
    {
        fkwrap = (FKStructWrapper*)CustomGrid_GetRowLongData(m_ptabintmgmt->m_tabfk->m_hgridfk, row);
        
        if(!fkwrap)
            continue;

        if(!fkwrap->m_newval)
            continue;

        if(!fkwrap->m_oldval)
            continue;

        if(fkwrap->m_newval == fkwrap->m_oldval)
            continue;

        m_ptabintmgmt->m_tabfk->m_listfkwrappers.Remove(fkwrap);
        m_ptabintmgmt->m_tabfk->ClearListSrcCols(fkwrap, fkwrap->m_newval->m_listsrccols);
        if(!listunsavedwrps)
            listunsavedwrps = new List();
        listunsavedwrps->Insert(fkwrap);
    }

    return listunsavedwrps;
}

wyBool
TableTabInterface::SaveTable(wyBool &queryexecuted)
{
    wyString        tblname, dbname;
    wyString        escapetblname(""), escapedbname("");
    wyString        query(""), renamequery(""), newfkquery(""), tempstr(""), droprecreate("");
    List            *listunsavedfks = NULL;
    wyString        **dropcreate = new wyString*[2];
    wyInt32         cntfkgridrows   = 0;
    wyBool          error = wyFalse;
    
    m_ptabintmgmt->GetActiveTabImage();
    //..Apply\Cancel GridChanges
    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfields->m_hgridfields, wyTrue);
    m_ptabintmgmt->m_tabindexes->ApplyCancelGridChanges();
	m_ptabintmgmt->m_tabcheck->ApplyCancelGridChanges();
    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfk->m_hgridfk, wyTrue);

    //=> Validations
    //...Validating Table name
    if(!GetNewTableName(tblname, wyTrue))
        return wyFalse;

    escapetblname.SetAs(tblname);
    escapetblname.FindAndReplace("`", "``");
    
    //...Validating Fields
    if(!m_ptabintmgmt->m_tabfields->ValidateFields(wyTrue))
        return wyFalse;

    //...Validating Indexes
    if(! m_ptabintmgmt->m_tabindexes->ValidateIndexes(wyTrue))
        return wyFalse;

    if(m_isfksupported)
    {
        //...Validating Foreign Keys
        if(! m_ptabintmgmt->m_tabfk->ValidateFKs(wyTrue))
            return wyFalse;
    }
    //=> Generating Query
    if(!GenerateQuery(query, wyTrue))
    {
        return wyFalse;
    }

    if(query.GetLength())
    {
        //=> Executing query
        if(!ExecuteQuery(query))
        {
            return wyFalse;
        }
        queryexecuted = wyTrue;
    }

    if(m_isaltertable)
    {
        /// Generating and Creating Alter FK query
        dropcreate[0] = new wyString();
        dropcreate[1] = new wyString();
        
        cntfkgridrows = CustomGrid_GetRowCount(m_ptabintmgmt->m_tabfk->m_hgridfk);

        for(int i=0; i<cntfkgridrows; i++)
        {
            if(!m_ptabintmgmt->m_tabfk->GenerateFKDropRecreateQuery(dropcreate, i))
                continue;

            tblname.FindAndReplace("`", "``");

            tempstr.Sprintf("Alter table `%s`.`%s` %s", m_dbname.GetString(), tblname.GetString(), dropcreate[0]->GetString());
            
            if(!ExecuteQuery(tempstr))
            {
                if(queryexecuted == wyTrue)
                {
                    /// if Alter FK fails, then we shall get the wrappers from all the next rows and store them in a list
                    listunsavedfks = GetListOfUnsavedWrappers(i);

                    /// Getting the Table status and Setting the m_isfksupported variable
                    GetMyResTableStatus();
                    
                    error = wyFalse;
                    m_isfksupported = IsTableInnoDB(error);
                    if(error)
                        return wyFalse;

                    /// ReInitializing all windows
                    ReInitializeBasicOptions();
                    m_ptabintmgmt->m_tabfields->ReInitializeGrid();
                    m_ptabintmgmt->m_tabindexes->ReInitializeGrid();
					m_ptabintmgmt->m_tabcheck->ReInitializeGrid();
                    m_ptabintmgmt->m_tabfk->ReInitializeGrid(listunsavedfks);
                    m_ptabintmgmt->m_tabadvprop->ReinitializeValues();
                    if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
                        m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();

                    if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
                        m_ptabintmgmt->ShowHideToolButtons(wyTrue);
                    else
                        m_ptabintmgmt->ShowHideToolButtons(wyFalse);
                }
                return wyFalse;
            }
            FKStructWrapper *fkwrap = NULL;
            fkwrap = (FKStructWrapper*) CustomGrid_GetRowLongData(m_ptabintmgmt->m_tabfk->m_hgridfk, i);
            
            //..Clearing SrcColsList & TgtColsList for oldval because, that fk is now dropped
            if(fkwrap->m_oldval)
            {
                m_ptabintmgmt->m_tabfk->ClearListSrcCols(fkwrap, fkwrap->m_oldval->m_listsrccols);
                m_ptabintmgmt->m_tabfk->ClearListTgtCols(fkwrap->m_oldval->m_listtgtcols);
                delete fkwrap->m_oldval;
                fkwrap->m_oldval = NULL;
            }
            queryexecuted = wyTrue;
            //reloadvalues = wyTrue;

            if(dropcreate[1]->GetLength())
            {
                tempstr.Sprintf("Alter table `%s`.`%s` %s", m_dbname.GetString(), tblname.GetString(), dropcreate[1]->GetString());

                if(!ExecuteQuery(tempstr))
                {
                    if(queryexecuted == wyTrue)
                    {
                        listunsavedfks = GetListOfUnsavedWrappers(i);
                        
                        /// Getting the Table status and Setting the m_isfksupported variable
                        GetMyResTableStatus();

                        error = wyFalse;
                        m_isfksupported = IsTableInnoDB(error);
                        if(error)
                            return wyFalse;

                        //..ReInitialize All Values
                        ReInitializeBasicOptions();
                        m_ptabintmgmt->m_tabfields->ReInitializeGrid();
                        m_ptabintmgmt->m_tabindexes->ReInitializeGrid();
                        m_ptabintmgmt->m_tabfk->ReInitializeGrid(listunsavedfks);
                        m_ptabintmgmt->m_tabadvprop->ReinitializeValues();
                        if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
                            m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();

                        if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
                            m_ptabintmgmt->ShowHideToolButtons(wyTrue);
                        else
                            m_ptabintmgmt->ShowHideToolButtons(wyFalse);
                    }
                    return wyFalse;
                }
            }
        }

        delete dropcreate[0];
        delete dropcreate[1];
        delete dropcreate;
        
        GetRenameQuery(renamequery, wyTrue);
        
        if(renamequery.GetLength())
        {
            if(!ExecuteQuery(renamequery))
            {
                if(queryexecuted == wyTrue)
                {
                    /// Getting the Table status and Setting the m_isfksupported variable
                    GetMyResTableStatus();
                    
                    error = wyFalse;
                    m_isfksupported = IsTableInnoDB(error);
                    if(error)
                        return wyFalse;

                    ReInitializeBasicOptions();
                    m_ptabintmgmt->m_tabfields->ReInitializeGrid();
                    m_ptabintmgmt->m_tabindexes->ReInitializeGrid();
                    m_ptabintmgmt->m_tabfk->ReInitializeGrid();
                    m_ptabintmgmt->m_tabadvprop->ReinitializeValues();
                    if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
                        m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();

                    if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
                        m_ptabintmgmt->ShowHideToolButtons(wyTrue);
                    else
                        m_ptabintmgmt->ShowHideToolButtons(wyFalse);
                }
                return wyFalse;
            }
            queryexecuted = wyTrue;
            m_ptabintmgmt->m_tabpreview->m_renamequeryrowno = -1;
        }
    }

    return wyTrue;
}

void
TableTabInterface::SetFocusToNewTab()
{
    wyInt32             tabindex = -1;
    CTCITEM             ctci = {0};
    wyUInt32            mask = CTBIF_IMAGE | CTBIF_LPARAM;
    TableTabInterface   *tabint = NULL;

    ctci.m_mask         =   mask;
    tabindex = CustomTab_GetCurSel(m_hwndparent);

    VERIFY(CustomTab_GetItem(m_hwndparent, tabindex, &ctci));

    tabint = (TableTabInterface*) ctci.m_lparam;

    if(tabint)
    {
        PostMessage(tabint->m_hwnd, UM_SETFOCUS, (WPARAM) tabint->m_hedittblname, 0);
    }
}

void
TableTabInterface::SetInitFocus()
{
    wyInt32 image = -1;

    image = m_mdiwnd->m_pcqueryobject->GetSelectionImage();

    if(m_ptabintmgmt->GetActiveTabImage() == IDI_COLUMN)
    {
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabfields->m_hgridfields, 0);
    }
    else if(m_ptabintmgmt->GetActiveTabImage() == IDI_MANINDEX_16)
    {
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabindexes->m_hgridindexes, 0);
    }
    else if(m_ptabintmgmt->GetActiveTabImage() == IDI_MANREL_16)
    {
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabfk->m_hgridfk, 0);
    }
    else if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABLEOPTIONS)
    {
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabadvprop->m_heditcomment, 0);
    }
    else if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
    {
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabpreview->m_hwndpreview, 0);
    }
	else if (m_ptabintmgmt->GetActiveTabImage() == IDI_CHECKCONSTRAINT)
	{
		PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabcheck->m_hgridtblcheckconst, 0);
	}
}

wyBool
TableTabInterface::SetTabItem(HWND hwnd, wyInt32 tabindex, wyString& text, wyUInt32 mask, wyInt32 image, wyInt32 lparam, wyBool addasterisk)
{
    wyString    objectname, tempstr;
    wyString    strdbname, strtblname, strtooltip;
    CTCITEM ctci = {0};
    wyInt32		ncurselindex;

    objectname.SetAs(text);
    if(addasterisk)
        objectname.Add("*");

    if(objectname.GetLength() >  SIZE_24)       //..If table name is more than 24 characters, make table name smaller
	{
		tempstr.SetAs(objectname);

		//take 1st 12 characters
		tempstr.Strip(objectname.GetLength() - SIZE_12);
		tempstr.Add("...");

		//take last 12 characters
		tempstr.Add(objectname.Substr((objectname.GetLength() - SIZE_12), SIZE_12));

		objectname.SetAs(tempstr);
	}

    GetComboboxValue(m_hcmbdbname, strdbname);
    strtblname.SetAs(text);

    strdbname.FindAndReplace("`", "``");
    strtblname.FindAndReplace("`", "``");
    
    strtooltip.Sprintf("`%s`.`%s`", strdbname.GetString(), strtblname.GetString());

    //..Change the Tab Name.
    ctci.m_mask         =   mask;
    ncurselindex	    =	tabindex;
    VERIFY(CustomTab_GetItem(hwnd, ncurselindex, &ctci));

    ctci.m_psztext      =   (wyChar*)objectname.GetString();
    ctci.m_mask         =   CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
    ctci.m_tooltiptext  =   (wyChar*)strtooltip.GetString();
    ctci.m_cchtextmax   =   objectname.GetLength();
    ctci.m_iimage       =   IDI_ALTERTABLE;
    if(lparam)
        ctci.m_lparam   =   lparam;

    CustomTab_SetItem(m_hwndparent, ncurselindex, &ctci);
    UpdateWindow(m_hwndparent);

	//update here name in dropdown
	UpdateDropdownStruct(ncurselindex, objectname.GetString());

    return wyFalse;
}


void
TableTabInterface::UpdateDropdownStruct(wyInt32 index,wyString newname)
{
	MDIListForDropDrown *p, *pfound;
	wyInt32 tabindexindropdown, tabcount=0;
	MDIWindow *wnd;
	wyBool found = wyFalse, foundmodifiedtab = wyFalse;

	wnd = GetActiveWin();

	p = (MDIListForDropDrown *)pGlobals->m_mdilistfordropdown->GetFirst();

	if (!p)
	{
		return;
	}
	if (!wnd)
	{
		return;
	}
	while (p)
	{
		if (wnd == p->mdi)
		{
			found = wyTrue;
			pfound = p;
			break;
		}
		p = (MDIListForDropDrown *)p->m_next;
	}
	if (found) {
		if (pfound) {

			//get the tab which is modifed
			tabcount = p->opentab->GetCount();

			ListOfOpenQueryTabs *node2 = new ListOfOpenQueryTabs();
			node2 = (ListOfOpenQueryTabs *)p->opentab->GetFirst();
			for (tabindexindropdown = 0; tabindexindropdown < tabcount; tabindexindropdown++)
			{
				if (tabindexindropdown == index)//as indexs for tabs starts from 0 
				{
					foundmodifiedtab = wyTrue;
					break;
				}
				node2 = (ListOfOpenQueryTabs *)node2->m_next;
			}
			if (foundmodifiedtab)
			{
				node2->tabname.SetAs(newname.GetString());

			}
		}
	}
}

wyBool
TableTabInterface::OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
        /*
    case ACCEL_FIRSTTAB:
        {
            int kk = 0;
        }
        break;

    case ACCEL_SECONDTAB:
        {
            int kk = 0;
        }
        break;
        */

    case IDC_SAVE:
        OnClickSave();
        break;

    case IDC_CANCEL_CHANGES:
        OnClickCancelChanges();
        break;

	case IDCANCEL:
        if(m_open_in_dialog == wyTrue)
        {
		    SendMessage(m_hwndparent, WM_CLOSE, 0, 0);
        }
		break;

    case IDC_DBNAME:
    case IDC_TABLETYPESLIST:
    case IDC_TABCHARSET:
    case IDC_TABCOLLATION:
        {
            switch(HIWORD(wParam))
            {
            case CBN_SETFOCUS:
                {
                    m_lastfocus = (HWND)lParam;
                }
                break;

            case CBN_SELENDOK:
            case CBN_CLOSEUP:
                {
                    if(!m_disableenchange)
                    {
                        wyString    str;
						MDIWindow	*wnd = NULL;
						
						VERIFY(wnd = GetActiveWin());
						/// getting the engine version from the combo-box
                        GetComboboxValue((HWND)lParam, str);
            
                        if((HWND)lParam == m_hcmbtabletype)
                        {
                            /// if engine type is ndbcluster we need to check if, this version of ndbcluster engine supports foreign key or not
							/// Foreign key support for NDBcluster engine was provided from cluster version 7.3 and greater.
							if(str.CompareI("ndbcluster") == 0)
							{
								m_isfkforndbcluster = GetClusterdbSupportForFk(wnd);
							}
							if(str.CompareI("innodb") == 0 || str.CompareI("pbxt") == 0 || str.CompareI(STR_DEFAULT) == 0 || m_isfkforndbcluster)
								m_isfksupported = wyTrue;
							else
						    {
								m_isfksupported = wyFalse;
								/// Changing the selected subtab, if the active subtab is Foreign-Keys
								if(m_ptabintmgmt->GetActiveTabImage() == IDI_MANREL_16)
									m_ptabintmgmt->SelectTab(1);
							}
                        }
                        else if(!m_ptabintmgmt->m_tabpreview->m_istabempty && ((HWND)lParam == m_hcmbdbname))
                        {
                            m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
                        }
                        
                        if(m_prevvalue.Compare(str) != 0 && ((HWND)lParam != m_hcmbdbname))
                        {
                            if(LOWORD(wParam) == IDC_TABCHARSET)
                            {
                                HandleTabCharset();
                            }
                            if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW) //..Checks if the Active subtab is a Preview tab
                            {
                                m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
                            }
                            MarkAsDirty(wyTrue);
                            //m_prevvalue.Clear();
                        }
                    }
                }
                break;

            case CBN_DROPDOWN:
                {
                    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfields->m_hgridfields);
                    m_ptabintmgmt->m_tabindexes->ApplyCancelGridChanges();
                    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfk->m_hgridfk);
                    SetFocus((HWND)lParam);

                    //if(LOWORD(wParam) == IDC_TABCHARSET)
                    {
                        GetComboboxValue((HWND)lParam, m_prevvalue);
                    }
                }
                break;
            }
        }
        break;
    }

    switch(HIWORD(wParam))
    {
    case EN_CHANGE:
        {
            if(!m_ptabintmgmt || !m_ptabintmgmt->m_allsubtabsloaded)
                break;

            m_lastfocus = (HWND)lParam;
            if(!m_disableenchange)
            {
                if(LOWORD(wParam) == IDC_TABLENAME)
                {
                    wyString tblname, dbname;
                    GetNewTableName(tblname);

                    tblname.RTrim();

                    if(m_isaltertable && tblname.Compare(m_origtblname) == 0 && m_ptabintmgmt->m_tabpreview->m_istabempty)
                        return wyTrue;
                    
                    if(!m_isaltertable && tblname.GetLength() == 0 && m_ptabintmgmt->m_tabpreview->m_istabempty)
                        return wyTrue;

                    if(m_ptabintmgmt && m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
                    {
                        if(m_isaltertable)
                        {
                            //if(tblname.Compare(m_origtblname) == 0)
                            if(m_ptabintmgmt->m_tabpreview->m_istabempty)
                                m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
                            else
                                m_ptabintmgmt->m_tabpreview->UpdateTableName(tblname);
                        }
                        else
                        {
                            if(m_ptabintmgmt->m_tabpreview->m_istabempty)
                                m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
                            else
                                m_ptabintmgmt->m_tabpreview->UpdateTableName(tblname);
                        }
                    }

                    if(!m_dirtytab)
                        MarkAsDirty(wyTrue);
                }
            }
        }
        break;

    case EN_SETFOCUS:
        {
            if(m_ptabintmgmt && m_ptabintmgmt->m_allsubtabsloaded)
            {
                CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfields->m_hgridfields);
                m_ptabintmgmt->m_tabindexes->ApplyCancelGridChanges();
                CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfk->m_hgridfk);
                SetFocus(m_hedittblname);
            }
        }
        break;
    }
    return wyTrue;
}

wyBool
TableTabInterface::OnClickSave(wyBool onclosetab, wyBool showsuccessmsg)
{
    wyBool      queryexecuted = wyFalse, newtabopened = wyFalse, error = wyFalse;
    wyInt32     prevtabind = -1;
    HCURSOR		hCursor;
    wyString    tempstr;

    hCursor = ::GetCursor();
    ::SetCursor(LoadCursor(NULL,IDC_WAIT));
    ::ShowCursor(1);

    //..If query execution fails, we shall return from here..
    if(!SaveTable(queryexecuted))
    {
        ::SetCursor( hCursor );
        ::ShowCursor(1);
        return wyFalse;
    }

    //..If any query is executed
    if(queryexecuted)
    {
        if(showsuccessmsg)
        {
            wyString msg;

            prevtabind = CustomTab_GetCurSel(m_hwndparent);
            if(!m_isaltertable && !onclosetab)
            {
                if(m_open_in_dialog)
                {
                    /// Checking whether the database is same as the db on SD from where the dialog was opened
                    GetComboboxValue(m_hcmbdbname, m_dbname);
                    if(!m_sddatabase.GetLength())
                        m_sddatabase.SetAs(m_dbname);

                    if(m_sddatabase.Compare(m_dbname) == 0)
                    {
                        m_ntablescreated++;
                        
                        if(!m_listoftablescreated)
                            m_listoftablescreated = (wyString**)malloc(m_ntablescreated * sizeof(wyString*));
                        else
                            m_listoftablescreated = (wyString**)realloc(m_listoftablescreated, m_ntablescreated * sizeof(wyString*));

                        m_listoftablescreated[m_ntablescreated-1] = new wyString();

                        GetNewTableName(tempstr);
                        m_listoftablescreated[m_ntablescreated-1]->SetAs(tempstr);
                    }
                }
                
                wyInt32 ret = IDNO;
                msg.SetAs(_("Table created successfully.\r\n\r\nDo you want to create more tables?"));
                ret = MessageBox(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONINFORMATION);

                if(ret == IDYES)
                {
                    if(m_open_in_dialog)
                    {
                        OnClickCancelChanges();
                        m_ptabintmgmt->SelectTab(0);
                        PostMessage(m_hwnd, UM_SETFOCUS, (LONG)m_hedittblname, 0);
                        return wyTrue;
                    }
                    else
                    {
                        newtabopened = wyTrue;
                        GetNewTableName(m_origtblname);
                        GetComboboxValue(m_hcmbdbname, m_dbname);
                        RefreshObjectBrowser(m_dbname, m_origtblname);
                        m_parentptr->CreateTableTabInterface(m_mdiwnd);
                    }
                }
                else
                {
                    if(m_open_in_dialog)
                    {
                        EndDialog(GetParent(m_hwnd), 1);
                        ::SetCursor( hCursor );
                        ::ShowCursor(1);
                        return wyTrue;
                    }
                }
            }
            else
            {
                if(m_isaltertable && m_open_in_dialog)
                {
                    GetNewTableName(m_origtblname);
                    m_istablealtered = wyTrue;
                    EndDialog(GetParent(m_hwnd), 1);
                    ::SetCursor( hCursor);
                    ::ShowCursor(1);
                    return wyTrue;
                }

                if(m_isaltertable)
                    MessageBox(m_hwnd, ALTERED_SUCESSFULLY_EX, pGlobals->m_appname.GetAsWideChar(), MB_OK|MB_ICONINFORMATION);
                else
                    MessageBox(m_hwnd, CREATED_SUCESSFULLY, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
            }
        }
        
        if(!onclosetab)
        {
            //..Setting main-tab name
            GetNewTableName(m_origtblname);
            SetTabItem(m_hwndparent, prevtabind, m_origtblname, CTBIF_IMAGE | CTBIF_LPARAM, IDI_ALTERTABLE, NULL);
            GetComboboxValue(m_hcmbdbname, m_dbname);

            if(!GetMyResTableStatus())
                return wyFalse;

            if(!m_myrestablestatus)
                return wyFalse;
            
            error = wyFalse;
            m_isfksupported = IsTableInnoDB(error);
            if(error)
                return wyFalse;

            if(!m_isfksupported && m_ptabintmgmt->GetActiveTabImage() == IDI_MANREL_16)
            {
                m_ptabintmgmt->SelectTab(1);
            }

            m_isaltertable = wyTrue;
            //..Reinitializing all sub-tab values
            ReInitializeAllValues();

            CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfields->m_hgridfields, 0, 0);
            CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabindexes->m_hgridindexes, 0, 1);
            CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfk->m_hgridfk, 0, 1);

            if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
            {
                m_ptabintmgmt->ShowHideToolButtons(wyTrue);
            }
            if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
                m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();

            //..Disabling the database combo-box
            EnableWindow(m_hcmbdbname, FALSE);
        }

        if(!newtabopened)
        {
            GetComboboxValue(m_hcmbdbname, m_dbname);
            GetNewTableName(m_origtblname);
            RefreshObjectBrowser(m_dbname, m_origtblname);
        }
    }
    else
    {
        m_ptabintmgmt->m_tabadvprop->FillInitValues();
    }
    if(!onclosetab)
        SetInitFocus();

    //..Disabling the buttons
    EnableWindow(m_hbtnsave, FALSE);
    EnableWindow(m_hbtncancelchanges, FALSE);
    
    //..making dirty tab FALSE
    m_isaltertable = wyTrue;
    MarkAsDirty(wyFalse, (wyBool)(queryexecuted && wyTrue));

    if(newtabopened)
        SetFocusToNewTab();

    ::SetCursor( hCursor );
    ::ShowCursor(1);

    return wyTrue;
}


void
TableTabInterface::CancelChanges()
{
    m_disableenchange = wyTrue;
    SendMessage(m_hedittblname, WM_SETTEXT, 0, (LPARAM)(m_isaltertable ? m_origtblname.GetAsWideChar() : TEXT(""))); //Sets table name

    /// Selecting database
    if(!m_isaltertable)
        SelectDatabase();

    SendMessage(m_hcmbtabletype, CB_SELECTSTRING, 0, (LPARAM) (m_isaltertable ? m_origtableengine.GetAsWideChar() : TEXT(STR_DEFAULT)));

    /// setting m_isfksupported variable
	wyBool error = wyTrue;
	if (m_isaltertable)
		m_isfksupported = IsTableInnoDB(error);
	else
		m_isfksupported = wyTrue; // bug: click on revert ignores the details entered in Foreign key tab for create table 

    if(m_ismysql41)
    {
        wyString tempstr, origval;
        GetComboboxValue(m_hcmbcharset, tempstr);

        if(m_isaltertable)
        {
            if(tempstr.Compare(m_origcharset) != 0)
            {
                SendMessage(m_hcmbcharset, CB_SELECTSTRING, 0, (LPARAM) m_origcharset.GetAsWideChar());
                HandleTabCharset();
            }
            SendMessage(m_hcmbcollation, CB_SELECTSTRING, 0, (LPARAM) m_origcollate.GetAsWideChar());
        }
        else
        {
            if(tempstr.Compare(STR_DEFAULT) != 0)
            {
                SendMessage(m_hcmbcharset, CB_SELECTSTRING, 0, (LPARAM) TEXT(STR_DEFAULT));
                SendMessage(m_hcmbcollation, CB_SELECTSTRING, 0, (LPARAM) TEXT(STR_DEFAULT));
            }
        }
    }
    m_disableenchange = wyFalse;

}

LRESULT
TableTabInterface::OnWMSysKeyDown(wyUInt32 currenttab, WPARAM wParam, LPARAM lParam)
{
    wyInt32 altkeypressed = (wyInt32)lParam;

    altkeypressed = altkeypressed >> 29;

    //..return 1 if Alt key is not processed
    if(!altkeypressed)
        return 1;

    switch(wParam)
    {
    case VK_KEY_0:
    case VK_KEY_1:
        if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
            m_ptabintmgmt->SelectTab(0);
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabfields->m_hgridfields, 0);
        return 0;

    case VK_KEY_2:
        if(m_ptabintmgmt->GetActiveTabImage() != IDI_MANINDEX_16)
            m_ptabintmgmt->SelectTab(1);
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabindexes->m_hgridindexes, 0);
        return 0;
        
    case VK_KEY_3:
        if(m_ptabintmgmt->GetActiveTabImage() != IDI_MANREL_16)
            m_ptabintmgmt->SelectTab(2);
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabfk->m_hgridfk, 0);
        return 0;

    case VK_KEY_4:
        if(m_ptabintmgmt->GetActiveTabImage() != IDI_TABLEOPTIONS)
            m_ptabintmgmt->SelectTab(3);
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabadvprop->m_heditcomment, 0);
        return 0;

    case VK_KEY_5:
    case VK_KEY_9:
        if(m_ptabintmgmt->GetActiveTabImage() != IDI_TABPREVIEW)
            m_ptabintmgmt->SelectTab(4);
        PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_ptabintmgmt->m_tabpreview->m_hwndpreview, 0);
        return 0;
    }

    return 1;
}

wyBool
TableTabInterface::GetComboboxValue(HWND hcombobox, wyString& value)
{
    wyWChar     *valuew = NULL;
    wyInt32     valuelen = 0, ind;

    value.Clear();
    ind = SendMessage(hcombobox, CB_GETCURSEL, 0, 0);

    if(ind != CB_ERR)
    {
        valuelen = SendMessage(hcombobox, CB_GETLBTEXTLEN, ind, 0);
        valuew = AllocateBuffWChar(valuelen+1);
        SendMessage(hcombobox, CB_GETLBTEXT, ind, (LPARAM) valuew);
    }

    if(valuew)
    {
        value.SetAs(valuew);
        free(valuew);
    }

    return wyTrue;
}

wyBool
TableTabInterface::GenerateQuery(wyString &query, wyBool showmsg)
{
    wyString    tblname(""), tblnameold(""), dbname(""), tempquery(""), tempstr("");
    wyBool      retval = wyTrue, nocols = wyFalse;
    wyInt32     index = -1;
	wyChar*		backtick;

	//from  .ini file
	backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    retval = GetNewTableName(tblname);

    tblname.FindAndReplace("`", "``");

    tblnameold.SetAs(m_origtblname);
    tblnameold.FindAndReplace("`", "``");

    GetComboboxValue(m_hcmbdbname, dbname);
    dbname.FindAndReplace("`", "``");	

    if(m_isaltertable)
        tempquery.AddSprintf("Alter table %s%s%s.%s%s%s ", backtick, dbname.GetString(), backtick,
			backtick, tblnameold.GetString(), backtick);
    else
    {
        tempquery.AddSprintf("Create table %s%s%s.%s%s%s (", backtick, dbname.GetString(), backtick,
			backtick, tblname.GetString(), backtick);

        tempstr.SetAs(TABLE_NAME_MISSING);
        if(!tblname.GetLength())
            tempquery.AddSprintf("\t\t/* %s */", tempstr.GetString());
        tempstr.Clear();
    }

    //..Merging Columns into query
    retval = (wyBool) (m_ptabintmgmt->m_tabfields->GenerateQuery(tempstr) && retval);

    if(!tempstr.GetLength())
        nocols = wyTrue;
    //..Merging indexes into query
    retval = (wyBool) (m_ptabintmgmt->m_tabindexes->GenerateQuery(tempstr) && retval);
    tempstr.RTrim();

    if(m_isfksupported)
    {
        //..Merging (Dropped and) Newly added fks into query
        m_ptabintmgmt->m_tabfk->GenerateNewAndDroppedFKQuery(tempstr);
        tempstr.RTrim();
    }

	//Add method for table level check constraint
	retval = (wyBool)(m_ptabintmgmt->m_tabcheck->GenerateQuery(tempstr) && retval);

    //..In Create table, remove last comma, Add bracket..
    if(!m_isaltertable)
    {
        wyString    otherprop("");

        AppendBasics(otherprop);
        otherprop.RTrim();
        m_ptabintmgmt->m_tabadvprop->GenerateQuery(otherprop);

        if(!tempstr.GetLength())
        {
            if(otherprop.GetLength() || tblname.GetLength())
            {
                tempstr.RTrim();
                tempstr.AddSprintf("\r\n   /* %s */ \r\n   ", NO_COLUMNS_DEFINED);
            } 
        }
        else if(tempstr.GetCharAt(tempstr.GetLength()-1) != ',')
        {
            index = tempstr.FindIWithReverse("\t\t/* ", 0, wyTrue);
            if(index != -1)
            {
                wyChar* errormsg = tempstr.Substr(tempstr.GetLength() - index - 5, index + 5);
                wyString errorstr("");

                errorstr.SetAs(errormsg);

                tempstr.Strip(index + 5);
                tempstr.RTrim();

                if(tempstr.GetCharAt(tempstr.GetLength() - 1) == ',')
                    tempstr.Strip(1);

                tempstr.Add(errorstr.GetString());
            }
        }

        wyString    strnocols, strnoquery;
        strnocols.Sprintf("\r\n   /* %s */ \r\n   ", NO_COLUMNS_DEFINED);
        strnoquery.Sprintf("/* %s */", NO_PENDING_QUERIES);

        if(nocols && tempstr.GetLength() && tempstr.Compare(strnoquery) != 0 && tempstr.Compare(strnocols) != 0 )
        {
            wyString    str1;
            str1.Sprintf("\r\n   /* %s */ \r\n   ", NO_COLUMNS_DEFINED);
            str1.RTrim();

            str1.AddSprintf("%s", tempstr.GetString());
            tempstr.SetAs(str1);
        }
        
        while(tempstr.GetCharAt(tempstr.GetLength()-1) == ',')
            tempstr.Strip(1);

        if(tempstr.Compare(strnocols) == 0)
        {
            tempstr.RTrim();
        }

        if(tempstr.GetLength()  && tempstr.Compare(strnoquery) != 0)
        {
				tempstr.Add("\r\n");
				tempstr.Add(")");
            if(otherprop.GetLength())
                tempstr.AddSprintf("%s", otherprop.GetString());
            tempstr.RTrim();
        }
    }
    else
    {
        wyString    advprop("\r\n ");
        AppendBasics(advprop);
        advprop.RTrim();
        m_ptabintmgmt->m_tabadvprop->GenerateQuery(advprop);
        advprop.RTrim();

        if(advprop.GetLength())
            tempstr.AddSprintf("%s", advprop.GetString());

        tempstr.RTrim();
        if(tempstr.GetCharAt(tempstr.GetLength()-1) != ',')
        {
            index = tempstr.FindIWithReverse(" */", 0, wyTrue);

            if(index == 0)
            {
                index = tempstr.FindIWithReverse("\t\t/* ", 0, wyTrue);
                if(index != -1)
                {
                    wyChar* errormsg = tempstr.Substr(tempstr.GetLength() - index - 5, index + 5);
                    wyString errorstr("");

                    errorstr.SetAs(errormsg);

                    tempstr.Strip(index + 5);
                    tempstr.RTrim();

                    if(tempstr.GetCharAt(tempstr.GetLength() - 1) == ',')
                        tempstr.Strip(1);

                    tempstr.Add(errorstr.GetString());
                }
            }
        }

        while(tempstr.GetCharAt(tempstr.GetLength()-1) == ',')
            tempstr.Strip(1);
    }

    if(tempstr.GetLength())
    {
        if(tempquery.GetLength())
            tempquery.Add(" ");
        tempquery.AddSprintf("%s", tempstr.GetString());
        query.Sprintf("%s", tempquery.GetString());
    }

    return retval;
}

wyBool
TableTabInterface::ExecuteQuery(wyString &query)
{
    MYSQL_RES		*res;
	wyInt32 isintransaction = 1;

    res = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction,  GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;
        
    if(!res && m_mdiwnd->m_tunnel->mysql_affected_rows(m_mdiwnd->m_mysql) == -1)
    {
        ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
        return wyFalse;
    }
    m_mdiwnd->m_tunnel->mysql_free_result(res);

    return wyTrue;
}

wyBool
TableTabInterface::OnClickCancelChanges()
{
    this->CancelChanges();
    m_ptabintmgmt->m_tabfields->CancelChanges(m_isaltertable);
    m_ptabintmgmt->m_tabindexes->CancelChanges(m_isaltertable);
    m_ptabintmgmt->m_tabfk->CancelChanges();
    m_ptabintmgmt->m_tabadvprop->CancelChanges(m_isaltertable);
    m_ptabintmgmt->m_tabpreview->CancelChanges();
	m_ptabintmgmt->m_tabcheck->CancelChanges(m_isaltertable);

    //if(m_isaltertable)
    {
        CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfields->m_hgridfields, 0, 0, wyTrue);
        CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabindexes->m_hgridindexes, 0, 1, wyTrue);
        CustomGrid_SetCurSelection(m_ptabintmgmt->m_tabfk->m_hgridfk, 0, 1, wyTrue);
        CustomGrid_SetSelAllState(m_ptabintmgmt->m_tabfields->m_hgridfields, BST_UNCHECKED);
        CustomGrid_SetSelAllState(m_ptabintmgmt->m_tabindexes->m_hgridindexes, BST_UNCHECKED);
        CustomGrid_SetSelAllState(m_ptabintmgmt->m_tabfk->m_hgridfk, BST_UNCHECKED);
		CustomGrid_SetSelAllState(m_ptabintmgmt->m_tabcheck->m_hgridtblcheckconst, BST_UNCHECKED);


    }

    if(m_ptabintmgmt->GetActiveTabImage() != IDI_COLUMN)
    {
        SendMessage(m_ptabintmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(TRUE,0));
        SendMessage(m_ptabintmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(TRUE,0));
    }

    if(m_ptabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
    {
        m_ptabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
    }

    switch(m_ptabintmgmt->GetActiveTabImage())
    {
    case IDI_COLUMN:
        SetFocus(m_ptabintmgmt->m_tabfields->m_hgridfields);
        break;

    case IDI_MANINDEX_16:
        SetFocus(m_ptabintmgmt->m_tabindexes->m_hgridindexes);
        break;

    case IDI_MANREL_16:
        SetFocus(m_ptabintmgmt->m_tabfk->m_hgridfk);
        break;

    case IDI_TABLEOPTIONS:
        SetFocus(m_ptabintmgmt->m_tabadvprop->m_hlastfocusedwnd);
        break;

    case IDI_TABPREVIEW:
        SetFocus(m_ptabintmgmt->m_tabpreview->m_hwnd);
        break;
    }
    
    MarkAsDirty(wyFalse);

    return wyTrue;
}

void
TableTabInterface::HandleTabCharset()
{
    wyInt32     index;
    wyString    charsetstr("");
    
    GetComboboxValue(m_hcmbcharset, charsetstr);

    if(m_prevvalue.GetLength() && m_prevvalue.CompareI(charsetstr) == 0)
        return;
    m_prevvalue.Clear();
    if(charsetstr.CompareI(STR_DEFAULT) != 0)
        ReInitRelatedCollations(m_hcmbcharset, charsetstr.GetAsWideChar());
    else
    {
        {
            SendMessage(m_hcmbcollation, CB_RESETCONTENT, 0, 0);
            SendMessage(m_hcmbcollation, CB_ADDSTRING, 0, (LPARAM)TEXT(STR_DEFAULT)); 
            SendMessage(m_hcmbcollation, CB_SETCURSEL, (WPARAM)0, 0);
        }
    }
    
    if((index = SendMessage(m_hcmbcharset, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT(STR_DEFAULT))) == CB_ERR && !m_isaltertable)
        SendMessage(m_hcmbcharset, CB_ADDSTRING, 0, (LPARAM)TEXT(STR_DEFAULT));
}

void
TableTabInterface::ReInitRelatedCollations(HWND hwnd, wyWChar  *charsetname)
{
    MYSQL_RES   *myres;
    MYSQL_ROW   myrow;
    wyWChar     *relcollation  = NULL;
    wyString    query, collationstr;
    wyInt32     ret, selcharsetlen  = 0;

    HWND    hwndcombo = m_hcmbcollation;// GetDlgItem(hwnd, IDC_TABCOLLATION);

    query.SetAs("show collation");
    myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(hwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query.GetString());
		return;
	}

    VERIFY((hwndcombo, CB_RESETCONTENT, 0, 0));

    if(charsetname)
        selcharsetlen = wcslen(charsetname);

    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
    {
        collationstr.SetAs(myrow[0]);
        ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)collationstr.GetAsWideChar());
	    if(ret != CB_ERR)
        {
            //delete the items which are not relevent
           if(wcsstr(collationstr.GetAsWideChar(), charsetname) == NULL)
                SendMessage(hwndcombo, CB_DELETESTRING, ret, 0);
        }
        else if((relcollation = wcsstr(collationstr.GetAsWideChar(), charsetname)) != NULL)
        {
            // Add the relevent items
            if(collationstr.GetCharAt(selcharsetlen) == '_')
                SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)collationstr.GetAsWideChar()); 
        }
    }
    
    m_mdiwnd->m_tunnel->mysql_free_result(myres);
    
    wyString charsetstr;
    charsetstr.SetAs(charsetname);

    ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)TEXT(STR_DEFAULT));

    if(ret == CB_ERR)
    {
        if(!m_isaltertable || (m_isaltertable && (charsetstr.CompareI(m_origcharset) != 0)))
            SendMessage(m_hcmbcollation, CB_INSERTSTRING, 0, (LPARAM)TEXT(STR_DEFAULT));
        SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)0, 0);
    }
    else
    {
        if(m_isaltertable && (charsetstr.CompareI(m_origcharset) == 0))
        {
            SendMessage(hwndcombo, CB_DELETESTRING, ret, 0);
            SendMessage(m_hcmbcollation, CB_SELECTSTRING, -1, (LPARAM)m_origcollate.GetAsWideChar());
        }
        else
            SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)0, 0);
    }
}

LRESULT CALLBACK
TableTabInterface::SysKeyDownWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TableTabInterface   *tabint = (TableTabInterface*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    WNDPROC             wndproc = NULL;

    if(!tabint)
        return 1;

    switch(message)
    {
    case WM_SYSKEYDOWN:
        {
            if(!tabint->OnWMSysKeyDown(CustomTab_GetCurSel(tabint->m_ptabintmgmt->m_hwnd), wParam, lParam))
                return 0;
        }
        break;
    }
    
    if(hwnd == tabint->m_hedittblname)
        wndproc = tabint->m_wporigtblname;
    else if(hwnd == tabint->m_hcmbdbname)
        wndproc = tabint->m_wporigdbname;
    else if(hwnd == tabint->m_hcmbtabletype)
        wndproc = tabint->m_wporigengine;
    else if(hwnd == tabint->m_hcmbcharset)
        wndproc = tabint->m_wporigcharset;
    else if(hwnd == tabint->m_hcmbcollation)
        wndproc = tabint->m_wporigcollate;
    else if(hwnd == tabint->m_hbtnsave)
        wndproc = tabint->m_wporigbtnsave;
    else if(hwnd == tabint->m_hbtncancel)
        wndproc = tabint->m_wporigbtncancel;
	else if(hwnd == tabint->m_hbtncancelchanges)
        wndproc = tabint->m_wporigbtncancelchanges;
    else if(tabint->m_ptabintmgmt && tabint->m_ptabintmgmt->m_tabfields && hwnd == tabint->m_ptabintmgmt->m_tabfields->m_hchkboxhidelanguageoptions)
        wndproc = tabint->m_ptabintmgmt->m_tabfields->m_wporighidelangopt;

    if(!wndproc)
        return 1;

    return CallWindowProc(wndproc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK
TableTabInterface::TableTabInterfaceWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TableTabInterface *tabint = (TableTabInterface*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    RECT temp;

    switch(message)
    {
    case WM_NCCREATE:
        tabint = (TableTabInterface*)((CREATESTRUCT *) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) tabint);
        break;

    case WM_CTLCOLORSTATIC:
        if(tabint->m_hstattblname == (HWND) lParam  || tabint->m_hstatdbname == (HWND) lParam  || 
            tabint->m_hstattabletype == (HWND) lParam   || tabint->m_hstatcharset == (HWND) lParam   || 
			tabint->m_hstatcollation == (HWND) lParam)
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetBkColor((HDC)wParam, BK_COLOR);

			return (INT_PTR)tabint->m_objbkcolor;
        }
        break;

	case WM_CTLCOLORBTN:
		if(tabint->m_hbtnsave == (HWND) lParam || tabint->m_hbtncancelchanges == (HWND) lParam || tabint->m_hbtncancel == (HWND) lParam )
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetBkColor((HDC)wParam, BK_COLOR);

            return (INT_PTR)tabint->m_objbkcolor;
        }
        break;

    case WM_PAINT:
		{
            PAINTSTRUCT ps = {0};
            HDC hdc;
            hdc = BeginPaint(hwnd, &ps);
            
            if(tabint->m_open_in_dialog == wyTrue)
            {
                /*DoubleBuffer db(hwnd, hdc);
                db.EraseBackground(GetSysColor(COLOR_3DFACE));*/
                DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
                GetClientRect(hwnd, &temp);
                temp.left = temp.right - 14;
                temp.top = temp.bottom - 14;
                DrawFrameControl(hdc, &temp, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);                
                //db.PaintWindow();
            }
            else
            {
                if(tabint->m_isbuffereddraw == wyTrue)
                {
                    DoubleBuffer db(hwnd, hdc);
                    db.EraseBackground(BK_COLOR);
                    db.PaintWindow();
                }
                else
                {
                    DoubleBuffer::EraseBackground(hwnd, hdc, NULL, BK_COLOR);
                }
            }

            EndPaint(hwnd, &ps);
            return 1;
        }
		break;

    case UM_SETFOCUS:
        SetFocus((HWND)wParam);
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
		//if(!tabint->m_open_in_dialog)
        tabint->Resize();
        break;

    case WM_NOTIFY:
        return tabint->OnWmNotify(hwnd, wParam, lParam);

    case UM_CLOSEDLG:
        if(tabint->m_open_in_dialog)
			PostMessage(tabint->m_hwndparent, WM_CLOSE, wParam, lParam);
        return 0;

    case WM_COMMAND:
        tabint->OnWmCommand(hwnd, wParam, lParam);
        break;

    case WM_CLOSE:
        return 0;

    case WM_HELP:
        {
            if(tabint->m_isaltertable)
                ShowHelp("http://sqlyogkb.webyog.com/article/88-alter-table-in-database");
            else
                ShowHelp("http://sqlyogkb.webyog.com/article/85-create-table");
            return 1;
        }
        break;
	}
    return DefWindowProc(hwnd, message, wParam, lParam);
}

wyBool
TableTabInterface::ConfirmClose(HWND hparent)
{
    wyString    query, renamequery, msg, caption;
    wyInt32		ret;

    /// Generating queries
    m_ptabintmgmt->m_tabpreview->GenerateQuery(query);
    GetRenameQuery(renamequery);

    /// No need to prompt if no query generated, so return "wyTrue"
    if(!(m_dirtytab && (query.GetLength() || renamequery.GetLength())))
        return wyTrue;

    /// The message that will pop-up
    if(m_isaltertable)
        msg.SetAs(_("The table has been changed.\n\nDo you want to save the changes?"));
    else
        msg.SetAs(_("Do you want to save the changes?"));

    caption.Sprintf("%s - [%s]", pGlobals->m_appname.GetString(), m_mdiwnd->m_title.GetString());

    /// Show Confirmation Dialog
    ret = CustomSaveMessageBox(hparent, msg.GetAsWideChar(), caption.GetAsWideChar(), m_mdiwnd->m_ismdiclosealltabs);

    /// Check the user selection and proceed accordingly
    switch(ret)
	{
	case IDYES:
		if(!OnClickSave(wyTrue, wyTrue))
			return wyFalse;
		return wyTrue;
	
    case IDNO:
		return wyTrue;

	case IDCANCEL:
		return wyFalse;
	}

    return wyFalse;
}

void
TableTabInterface::SetAllFonts()
{
    SetFont(m_ptabintmgmt->m_tabfields->m_hgridfields);
    SetFont(m_ptabintmgmt->m_tabindexes->m_hgridindexes);
    SetFont(m_ptabintmgmt->m_tabfk->m_hgridfk);
}

void 
TableTabInterface::SetFont(HWND hgrid)
{
	
	wyWChar	    *lpfileport = 0;
	wyWChar	    directory[MAX_PATH + 1] = {0};
	wyString	fontnamestr, dirstr;
	LOGFONT	    datafont = {0};
	wyInt32	    px, height;
	HDC		    hdc;
	HWND		hwnd;
	hwnd	=	hgrid;

	VERIFY(hdc = GetDC(GetParent(hwnd)));

	height = GetDeviceCaps( hdc, LOGPIXELSY );

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);
	// fill with default data.
	FillEditFont(&datafont, hwnd);
	
	wyIni::IniGetString(DATAFONT, "FontName", "Courier New", &fontnamestr, dirstr.GetString());
	
	wcscpy(datafont.lfFaceName, fontnamestr.GetAsWideChar());

	px = wyIni::IniGetInt(DATAFONT, "FontSize", 9, dirstr.GetString()); 	
	
    datafont.lfHeight = (wyInt32)((- px) * height / 72.0);

	datafont.lfWeight = wyIni::IniGetInt(DATAFONT, "FontStyle", 0, dirstr.GetString()); 	
	datafont.lfItalic = wyIni::IniGetInt(DATAFONT, "FontItalic", 0, dirstr.GetString()); 	
	datafont.lfCharSet = wyIni::IniGetInt(DATAFONT, "FontCharSet", DEFAULT_CHARSET, dirstr.GetString()); 
	
	CustomGrid_SetFont(hwnd, &datafont);
	
	//post 8.01
	//InvalidateRect(hwnd, NULL, FALSE);

}

void
TableTabInterface::CreateOtherWindows()
{
    DWORD           style = WS_CHILD | WS_VISIBLE;
    
    //...Creating "Table name" Windows (Static & Edit)
	m_hstattblname = CreateWindowEx(0, WC_STATIC, _(TEXT("Table Name")), 
										 style  , 0,0, 0,20,	m_hwnd, (HMENU)IDC_ABORT, 
										 (HINSTANCE)pGlobals->m_hinstance, NULL);
    
    ShowWindow(m_hstattblname , SW_SHOW);
    UpdateWindow(m_hstattblname);
    
    m_hedittblname = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, m_isaltertable ? m_origtblname.GetAsWideChar() :  _(TEXT("")), 
                                        style | ES_SUNKEN | ES_AUTOHSCROLL | WS_TABSTOP| WS_GROUP  , 0,0, 0,0, m_hwnd,
                                        (HMENU)IDC_TABLENAME, (HINSTANCE)pGlobals->m_hinstance, NULL);
    
    ShowWindow(m_hedittblname , SW_SHOW);
    UpdateWindow(m_hedittblname);
    SendMessage(m_hedittblname, EM_LIMITTEXT, 64, 0);

    //...Creating "Database" Windows (Static & Combo)
    m_hstatdbname = CreateWindowEx(0, WC_STATIC, _(TEXT("Database")), 
										 style  , 0,0, 0,0,	m_hwnd, (HMENU)IDC_ABORT, 
										 (HINSTANCE)pGlobals->m_hinstance, NULL);
    ShowWindow(m_hstatdbname , SW_SHOW);
    UpdateWindow(m_hstatdbname);

    m_hcmbdbname       =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 
										0, 0, 0, 0, m_hwnd, 
                                        (HMENU) IDC_DBNAME, (HINSTANCE)pGlobals->m_hinstance, NULL);
    ShowWindow(m_hcmbdbname , SW_SHOW);
    UpdateWindow(m_hcmbdbname);
    
    //...Creating Table-type Windows (Static & Combo)
    m_hstattabletype =   CreateWindowEx(0, WC_STATIC, _(TEXT("Engine")), 
                                        style  , 0,0, 0,0, m_hwnd, (HMENU) 0, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstattabletype)
    {
        ShowWindow(m_hstattabletype, SW_SHOW);
        UpdateWindow(m_hstattabletype);
    }

    m_hcmbtabletype =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 
										0, 0, 0, 0, m_hwnd, (HMENU) IDC_TABLETYPESLIST, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbtabletype)
    {
        ShowWindow(m_hcmbtabletype, SW_SHOW);
        UpdateWindow(m_hcmbtabletype);
    }

    //...Creating Character set (Static & Combo)
    m_hstatcharset =   CreateWindowEx(0, WC_STATIC, _(TEXT("Character Set")), 
                                        style | CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL, 
										0, 0, 0, 0, m_hwnd, (HMENU) 0, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatcharset)
    {
        ShowWindow(m_hstatcharset, SW_SHOW);
        UpdateWindow(m_hstatcharset);
    }

    m_hcmbcharset =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP | WS_GROUP , 
										0, 0, 0, 0, m_hwnd, 
                                        (HMENU) IDC_TABCHARSET, (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbcharset)
    {
        ShowWindow(m_hcmbcharset, SW_SHOW);
        UpdateWindow(m_hcmbcharset);
    }

    //...Creating Colation Windows (Static & Combo)
    m_hstatcollation =   CreateWindowEx(0, WC_STATIC, _(TEXT("Collation")), 
                                        style, 0, 0, 0, 0, m_hwnd, (HMENU) 0, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatcollation)
    {
        ShowWindow(m_hstatcollation, SW_SHOW);
        UpdateWindow(m_hstatcollation);
    }

    m_hcmbcollation =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 
										0, 0, 0, 0, m_hwnd, 
                                        (HMENU) IDC_TABCOLLATION, (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbcollation)
    {
        ShowWindow(m_hcmbcollation, SW_SHOW);
        UpdateWindow(m_hcmbcollation);
    }

	m_hbtnsave = CreateWindowEx(0, WC_BUTTON, _(TEXT("&Save")), style | WS_TABSTOP| WS_GROUP  , 0,0, 0,0,
    											m_hwnd, (HMENU)IDC_SAVE, 
												(HINSTANCE)pGlobals->m_hinstance, NULL);

    if(!m_hbtnsave)
		return;

	ShowWindow(m_hbtnsave, SW_SHOW);
    EnableWindow(m_hbtnsave, FALSE);

    //..Creating "Cancel Changes" button
	m_hbtncancelchanges = CreateWindowEx(0, WC_BUTTON, _(TEXT("&Revert")),style| WS_TABSTOP| WS_GROUP , 0,0, 0,0,
    											m_hwnd, (HMENU)IDC_CANCEL_CHANGES, 
												(HINSTANCE)pGlobals->m_hinstance, NULL);
	if(!m_hbtncancelchanges)
		return;
	ShowWindow(m_hbtncancelchanges, SW_SHOW);
    EnableWindow(m_hbtncancelchanges, FALSE);

    //..subclassing all windows, if tabbed-interface is on the dialog

    if(m_open_in_dialog)
    {
		//CreateButtons(m_hwndparent);
        
		m_wporigtblname = (WNDPROC)SetWindowLongPtr(m_hedittblname, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hedittblname, GWLP_USERDATA, (LONG_PTR)this);
        
        m_wporigdbname = (WNDPROC)SetWindowLongPtr(m_hcmbdbname, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbdbname, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigengine = (WNDPROC)SetWindowLongPtr(m_hcmbtabletype, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbtabletype, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigcharset = (WNDPROC)SetWindowLongPtr(m_hcmbcharset, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbcharset, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigcollate = (WNDPROC)SetWindowLongPtr(m_hcmbcollation, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbcollation, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigbtnsave = (WNDPROC)SetWindowLongPtr(m_hbtnsave, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hbtnsave, GWLP_USERDATA, (LONG_PTR)this);
		
		m_wporigbtncancelchanges = (WNDPROC)SetWindowLongPtr(m_hbtncancelchanges, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hbtncancelchanges, GWLP_USERDATA, (LONG_PTR)this);
        
		m_wporigbtncancel = (WNDPROC)SetWindowLongPtr(m_hbtncancel, GWLP_WNDPROC, (LONG_PTR)TableTabInterface::SysKeyDownWndProc);
        SetWindowLongPtr(m_hbtncancel, GWLP_USERDATA, (LONG_PTR)this);
    }
	//else
		

    //Sets the Windows' font
    SendMessage(m_hstattblname, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hedittblname, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    
    SendMessage(m_hstatdbname, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbdbname, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstattabletype, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbtabletype, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatcharset, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbcharset, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatcollation, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbcollation, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hbtnsave, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hbtncancelchanges, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

wyBool
TableTabInterface::CreateTabMgmt()
{
    m_ptabintmgmt = new TableTabInterfaceTabMgmt(m_hwnd, this);

    if(!m_ptabintmgmt->Create())
        return wyFalse;

    return wyTrue;
}

void
TableTabInterface::GetMaxWidthOfStaticText(wyInt32* col1, wyInt32* col2)
{
    wyInt32     size = 0;
    wyInt32     max1  = 0;
	wyInt32     max2  = 0;

    size = GetTextLenOfStaticWindow(m_hstattblname);
    if(size > max1)
        max1 = size;

    size = GetTextLenOfStaticWindow(m_hstatdbname);
    if(size > max1)
        max1 = size;

    size = GetTextLenOfStaticWindow(m_hstattabletype);
    if(size > max2)
        max2 = size;

    size = GetTextLenOfStaticWindow(m_hstatcharset);
    if(size > max2)
        max2 = size;

    size = GetTextLenOfStaticWindow(m_hstatcollation);
    if(size > max2)
        max2 = size;

	*col1 = max1;
	*col2 = max2;
}

VOID
TableTabInterface::Resize(wyBool issplittermoved)
{
    RECT		rcmain;
    wyInt32		hpos, vpos, width, height;
    wyInt32     staticwndwidthcol1 = 0;
	wyInt32     staticwndwidthcol2 = 0;
    wyInt32     minxpos = 0;
    wyInt32     dist_static_edit = 10;   //..Distance from static to edit
    wyInt32     dist_edit_static = 20;   //..Distance from edit to next static
    wyInt32     col2_minx = 0;
    
    VERIFY(GetWindowRect(m_hwndparent, &rcmain));
    VERIFY(MapWindowPoints(NULL, m_hwndparent, (LPPOINT)&rcmain, 2));

    GetClientRect(m_hwndparent, &rcmain);

    hpos	= rcmain.left;
    if(!m_open_in_dialog)
        vpos	= CustomTab_GetTabHeight(m_hwndparent);
    else
        vpos	= 0;
    
    width	= rcmain.right;
    minxpos = rcmain.left + 11;

	GetMaxWidthOfStaticText(&staticwndwidthcol1, &staticwndwidthcol2);

    height	= rcmain.bottom - rcmain.top - vpos; // - (m_open_in_dialog ? 10 : 0);// - (rcstatus.bottom - rcstatus.top) - 10 /*check for QB_DEFGAP in Query Builder Resize function*/;

    VERIFY(MoveWindow(m_hwnd, hpos,   vpos,   width,  height, TRUE));  //..TabInterface Main Window
    
    //..Table Windows
    VERIFY(MoveWindow(m_hstattblname,   minxpos,     19,     staticwndwidthcol1,     20,     TRUE));
    VERIFY(MoveWindow(m_hedittblname,   minxpos + staticwndwidthcol1 + dist_static_edit,     15,     180,    21,     TRUE));
    
    //..Database Windows
    VERIFY(MoveWindow(m_hstatdbname,    minxpos,     49,     staticwndwidthcol1,     20,     TRUE));
    VERIFY(MoveWindow(m_hcmbdbname,     minxpos + staticwndwidthcol1 + dist_static_edit,     45,     180,    21,     TRUE));
    
    //...COLUMNS_2 Window Positioning
    col2_minx = minxpos + staticwndwidthcol1 + dist_static_edit + 180 + dist_edit_static;
    
    //..Table-Type Windows
    VERIFY(MoveWindow(m_hstattabletype, col2_minx,    19,     staticwndwidthcol2,     20,     TRUE));
    VERIFY(MoveWindow(m_hcmbtabletype,  col2_minx + staticwndwidthcol2 + dist_static_edit,    15,     180,    21,     TRUE));

    VERIFY(MoveWindow(m_hstatcharset,   col2_minx,    49,    staticwndwidthcol2,     20,     TRUE));
    VERIFY(MoveWindow(m_hcmbcharset,    col2_minx + staticwndwidthcol2 + dist_static_edit,    45,    180,    21,     TRUE));
    
    VERIFY(MoveWindow(m_hstatcollation, col2_minx,    79,    staticwndwidthcol2,     20,     TRUE));
    VERIFY(MoveWindow(m_hcmbcollation,  col2_minx + staticwndwidthcol2 + dist_static_edit,    75,    180,    21,     TRUE));

    if(m_open_in_dialog)
    {
        SetWindowPos(m_hbtncancel, NULL, rcmain.right - 92, rcmain.bottom - 50 + 13 ,  80, 26,   SWP_NOZORDER );
	    SetWindowPos(m_hbtncancelchanges, NULL, rcmain.right - 181, rcmain.bottom - 50 + 13 ,  80, 26,    SWP_NOZORDER);
	    SetWindowPos(m_hbtnsave, NULL, rcmain.right - 270, rcmain.bottom - 50 + 13, 80, 26, SWP_NOZORDER);
    }
    else
    {
        VERIFY(MoveWindow(m_hbtncancelchanges,  rcmain.right - 92, height - 50 + 13 ,  80, 26,     TRUE));
        VERIFY(MoveWindow(m_hbtnsave,           rcmain.right - 181, height - 50 + 13,  80, 26,     TRUE));
    }

    if(m_ptabintmgmt)
    {
        m_ptabintmgmt->Resize();
    }
}

VOID
TableTabInterface::ShowTabContent(wyInt32 tabindex, wyBool status)
{
    BOOL            val = (status == wyTrue)?TRUE:FALSE;

    ShowWindow(m_hwnd, val);

    return;
}

wyBool
TableTabInterface::CloseTab(wyInt32 index)         //called from MDIWindow
{
    return wyTrue;
}

VOID
TableTabInterface::HandleTabControls(wyInt32 tabcount, wyInt32 selindex)
{
    BOOL show;

    show = (tabcount == selindex) ? TRUE : FALSE;
}

VOID
TableTabInterface::HandleFlicker()      //Called by TabModule
{
    
}

void
TableTabInterface::SetBufferedDrawing(wyBool isset)
{
    m_isbuffereddraw = isset;
}

VOID
TableTabInterface::OnTabSelChange()
{
    HWND        hwndfocus = NULL;
    
    /// Destroy if Find dilaog box is present
	if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

    pGlobals->m_pcmainwin->m_htabinterface = m_hwnd;

    SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_OBJECT_INSERTUPDATE, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_OBJECT_VIEWDATA, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_TABLE_OPENINNEWTAB, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE);

	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_FORMATCURRENTQUERY,(LPARAM)TBSTATE_INDETERMINATE);

    m_mdiwnd->SetQueryWindowTitle();
    
    
    
	if(m_hwnd)
    {
        switch(m_ptabintmgmt->GetActiveTabImage())
        {
        case IDI_COLUMN:
            hwndfocus = m_ptabintmgmt->m_tabfields->m_hgridfields;
            break;

        case IDI_MANINDEX_16:
            hwndfocus = m_ptabintmgmt->m_tabindexes->m_hgridindexes;
            break;

        case IDI_MANREL_16:
            hwndfocus = m_ptabintmgmt->m_tabfk->m_hgridfk;
            break;

        case IDI_TABLEOPTIONS:
            hwndfocus = m_ptabintmgmt->m_tabadvprop->m_heditcomment;
            break;

        case IDI_TABPREVIEW:
            hwndfocus = m_ptabintmgmt->m_tabpreview->m_hwndpreview;
            break;

		case IDI_CHECKCONSTRAINT:
			hwndfocus = m_ptabintmgmt->m_tabcheck->m_hgridtblcheckconst;
			break;

        }
        SetFocus(hwndfocus);
    }
}

VOID
TableTabInterface::OnTabSelChanging()
{
    //..Applying CustomGrid Changes
    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfields->m_hgridfields);
    m_ptabintmgmt->m_tabindexes->ApplyCancelGridChanges();
    CustomGrid_ApplyChanges(m_ptabintmgmt->m_tabfk->m_hgridfk);
}

wyInt32
TableTabInterface::OnTabClosing(wyBool ismanual)
{
    wyString		    msg, caption, query, renamequery;
    wyInt32			    ret;
    TableTabInterface   *tabint;
    wyBool              retvalue = wyFalse;

    m_ptabintmgmt->m_tabpreview->GenerateQuery(query);
    GetRenameQuery(renamequery);
    
    if(m_dirtytab && (query.GetLength() || renamequery.GetLength()) && IsConfirmOnTabClose() == wyTrue)
    {
        if(m_mdiwnd && m_mdiwnd->m_ismdiclosealltabs == wyTrue)
	    {
            SendMessage(GetParent(m_hwnd), WM_SETREDRAW, TRUE, 0);
		    //post 8.01 painting, Paint the tabs once the message box promt for 'Saving'
		    InvalidateRect(GetParent(m_hwnd), NULL, TRUE);
		    UpdateWindow(GetParent(m_hwnd));
	    }   

        //if(m_mdiwnd && m_mdiwnd->m_ismdiclosealltabs == wyTrue)
            msg.SetAs(_("The content of this tab has been changed.\n\nDo you want to save the changes?"));
        /*else
            msg.SetAs("Do you want to save your changes?");*/

        if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue || m_mdiwnd->m_ismdiclosealltabs == wyFalse)
            caption.SetAs(pGlobals->m_appname);
        else
            caption.Sprintf("%s - [%s]", pGlobals->m_appname.GetString(), m_mdiwnd->m_title.GetString());

        if(m_mdiwnd->m_mdisaveselection == -1)
            ret = CustomSaveMessageBox(m_hwnd, msg.GetAsWideChar(), caption.GetAsWideChar(), m_mdiwnd->m_ismdiclosealltabs);
        else
            ret = m_mdiwnd->m_mdisaveselection;

        switch(ret)
        {
        case IDYES:
            {
                if(!OnClickSave(wyTrue, wyTrue))
                {
                    SendMessage(GetParent(m_hwnd), WM_SETREDRAW, TRUE, 0);
                    return 0;
                }
            }
            break;
        
        case IDCANCEL:
            {
                SendMessage(GetParent(m_hwnd), WM_SETREDRAW, TRUE, 0);
			    return 0;
            }
            break;

        case IDNO:
            break;

        case IDYESTOALL:
            {
                if(!OnClickSave(wyTrue, wyFalse))
                {
                    SendMessage(GetParent(m_hwnd), WM_SETREDRAW, TRUE, 0);
                    return 0;
                }
                m_mdiwnd->m_mdisaveselection = ret;
            }
            break;

        case IDNOTOALL:
            {
                m_mdiwnd->m_mdisaveselection = ret;
            }
            break;
        }
    }

    tabint  = (TableTabInterface*)m_mdiwnd->m_pctabmodule->GetActiveTabType();

    if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
    {
        //SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(pGlobals->m_pcmainwin->m_hwndconntab, NULL, TRUE);
	    UpdateWindow(pGlobals->m_pcmainwin->m_hwndconntab);
    }
 	
    //To paint while closing all tabs in Con window
	if(m_mdiwnd && m_mdiwnd->m_ismdiclosealltabs == wyTrue)
	{
        //SendMessage(m_hwndparent, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hwndparent, NULL, TRUE);
		UpdateWindow(m_hwndparent);
	}

    //set the mdi title
    if(retvalue == wyTrue)
    {
        m_mdiwnd->SetQueryWindowTitle();
    }
	
	//Again block paint to avoid flickering while closing All tabs in con- window
	if(m_mdiwnd && m_mdiwnd->m_ismdiclosealltabs == wyTrue)
		SendMessage(m_mdiwnd->m_pctabmodule->m_hwnd, WM_SETREDRAW, FALSE, 0);	

    if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
    {
        SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, FALSE, 0);
    }

    return 1;
}

VOID
TableTabInterface::OnTabClose()
{
    pGlobals->m_pcmainwin->m_htabinterface = NULL;
    /*
	//To disable execute,executeall buttons when on closing tabs focus comes on tab querybuilder
	SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE);
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_FORMATCURRENTQUERY ,(LPARAM)TBSTATE_INDETERMINATE);
    //update the query window title
    MarkAsDirty(wyFalse);
    */
}

wyInt32
TableTabInterface::OnWmCloseTab()
{
    return 1;
}

void
TableTabInterface::SelectDatabase()
{
	wyInt32	cnt = -1;
	wyWChar     *valuew = NULL;
    wyInt32     valuelen = 0, ind;
	wyString	value;

	cnt = SendMessage(m_hcmbdbname, CB_GETCOUNT, 0, 0);

	if(cnt == CB_ERR)
		return;

	for(ind = 0; ind<cnt; ind++)
	{
		valuelen = SendMessage(m_hcmbdbname, CB_GETLBTEXTLEN, ind, 0);
        valuew = AllocateBuffWChar(valuelen+1);
        SendMessage(m_hcmbdbname, CB_GETLBTEXT, ind, (LPARAM) valuew);
    
		if(valuew)
		{
			value.SetAs(valuew);
			free(valuew);
		}

		if(value.Compare(m_dbname) == 0)
		{
			SendMessage(m_hcmbdbname, CB_SETCURSEL, ind, 0);
			return;
		}
	}
}

void
TableTabInterface::GetAllDatabases()
{
    //The array which holds the db names
    wyString**      pdbarr;
    wyUInt32        ind, dbcount = 0;

    dbcount = GetDatabasesFromOB(&pdbarr);

    for(ind=0; ind<dbcount; ind++)
    {
        if(pdbarr[ind]->GetLength())
        {
            SendMessage(m_hcmbdbname, CB_ADDSTRING, 0, (LPARAM) pdbarr[ind]->GetAsWideChar());
            delete pdbarr[ind];
        }
    }

    free(pdbarr);
}

void
TableTabInterface::InitTableTypeValues(wyBool includedefault)
{
    HWND        hwndcombo;
	wyInt32     index;
    wyString    strengine;
    wyWChar     *enginebuff, *tokpointer;

    GetTableEngineString(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, strengine);
    
    enginebuff = AllocateBuffWChar(strengine.GetLength()+1);
    wcscpy(enginebuff, strengine.GetAsWideChar());
    //VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TYPE));   
    hwndcombo   =   m_hcmbtabletype;

    tokpointer = wcstok(enginebuff, L";");

    while(tokpointer != NULL)
	{
        if(wcsicmp(tokpointer, _(L"PERFORMANCE_SCHEMA")) != 0 || m_dbname.CompareI("PERFORMANCE_SCHEMA") == 0)
            SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)tokpointer);
        tokpointer = wcstok(NULL, L";");
    }
    free(enginebuff);

    if(includedefault)
    {
        if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
            SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
    }

    return;
}

void
TableTabInterface::InitCharacterSetCombo(wyBool includedefault)
{
    HWND        hwndcombo = m_hcmbcharset;
    wyString    query, charsetstr;
    MYSQL_ROW   myrow;
    MYSQL_RES   *myres;
    wyInt32     index;
    
    query.SetAs("show charset");

    myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
    {
        charsetstr.SetAs(myrow[0]);
        index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)charsetstr.GetAsWideChar());
    }
    
    if(includedefault)
    {
        if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
            SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
    }

	m_mdiwnd->m_tunnel->mysql_free_result(myres);

}

void
TableTabInterface::AppendBasics(wyString& query)
{
    wyString        engine, charset, collation;
    wyString        str("");
    
    GetComboboxValue(m_hcmbtabletype, engine);

    if(engine.CompareI(STR_DEFAULT) != 0 && engine.CompareI(m_origtableengine) != 0)
    {
        if(m_ismysql41)
            str.Sprintf(" ENGINE=%s", engine.GetString());
        else
            str.Sprintf(" TYPE=%s", engine.GetString());

        if(m_isaltertable)
            str.Add(",");
    }

    if(m_ismysql41)
    {
        GetComboboxValue(m_hcmbcharset, charset);
        if(charset.CompareI(STR_DEFAULT) == 0)
            collation.SetAs(STR_DEFAULT);
        else
            GetComboboxValue(m_hcmbcollation, collation);

        //..Setting charset
        if(charset.Compare(STR_DEFAULT) != 0 && charset.CompareI(m_origcharset) != 0)
        {
            str.AddSprintf(" charset=%s", charset.GetString());
            if(m_isaltertable)
                str.Add(",");
        }
        
        //..Setting Collation
        if(collation.Compare(STR_DEFAULT) != 0 && collation.CompareI(m_origcollate) != 0)
        {
            str.AddSprintf(" collate=%s", collation.GetString());
            if(m_isaltertable)
                str.Add(",");
        }
    }

    if(str.GetLength())
        query.AddSprintf("%s", str.GetString());
}

wyBool
TableTabInterface::ReInitializeAllValues()
{
    ReInitializeBasicOptions();
    
    GetComboboxValue(m_hcmbdbname, m_dbname);
    GetNewTableName(m_origtblname);

    m_ptabintmgmt->m_tabfields->ReInitializeGrid();
    m_ptabintmgmt->m_tabindexes->ReInitializeGrid();
	m_ptabintmgmt->m_tabcheck->ReInitializeGrid();
    m_ptabintmgmt->m_tabfk->ReInitializeGrid();
    m_ptabintmgmt->m_tabadvprop->ReinitializeValues();
	

    return wyTrue;
}

wyBool
TableTabInterface::ReInitializeBasicOptions()
{
    wyString    query;

    m_prevvalue.Clear();
    SendMessage(m_hcmbtabletype, CB_RESETCONTENT, 0, 0);
    if(m_ismysql41)
    {
        SendMessage(m_hcmbcharset, CB_RESETCONTENT, 0, 0);
        SendMessage(m_hcmbcollation, CB_RESETCONTENT, 0, 0);
    
        InitCharacterSetCombo(wyFalse);
        InitCollationCombo(wyFalse);
    }
    InitTableTypeValues(wyFalse);
    
    return wyTrue;
}

wyBool
TableTabInterface::SetFocusToGridRow(wyString& objname)
{
    switch(m_ptabintmgmt->GetActiveTabImage())
    {
		case IDI_COLUMN:
			return m_ptabintmgmt->m_tabfields->SelectColumn(objname);
        
	    case IDI_MANREL_16:
		    return m_ptabintmgmt->m_tabfk->SelectForeignKey(objname);
    }

    return wyTrue;
}

wyBool
TableTabInterface::IsTableInnoDB(wyBool& error)
{
	MYSQL_ROW	    mystatusrow; 
	wyString		query, str; 
	MDIWindow		*wnd = NULL;

	VERIFY(wnd = GetActiveWin());
	if(!m_myrestablestatus)
		return wyFalse;
    m_mdiwnd->m_tunnel->mysql_data_seek(m_myrestablestatus, 0);
	VERIFY(mystatusrow = m_mdiwnd->m_tunnel->mysql_fetch_row(m_myrestablestatus));
	
    if(!mystatusrow)
    {
        return wyFalse;
    }
    error = wyFalse;

	str.SetAs(mystatusrow[1]);
	/// if engine type is ndbcluster we need to check if, this version of ndbcluster engine supports foreign key or not
	/// Foreign key support for NDBcluster engine was provided from cluster version 7.3 and greater.
	if(str.CompareI("ndbcluster") == 0)
	{		
		m_isfkforndbcluster = GetClusterdbSupportForFk(wnd);
	}
	if(str.CompareI("innodb") == 0 || str.CompareI("pbxt") == 0 || str.CompareI(STR_DEFAULT) == 0 || m_isfkforndbcluster)
	{	
		m_isfksupported = wyTrue;
		return wyTrue;
	}
	else
	{
		m_isfksupported = wyFalse;
		return wyFalse;
	}
}

wyBool
TableTabInterface::GetRenameQuery(wyString& renamequery, wyBool showmsg)
{
    wyString    newname(""), escapestrold(""), escapestrnew(""), escapedbname("");
    wyString    tempstr;
    wyBool      retval = wyTrue;
	wyChar*		backtick;

	//from  .ini file
	backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    renamequery.Clear();

    retval = GetNewTableName(newname, showmsg);
    
    if(newname.Compare(m_origtblname) == 0)
        return wyTrue;
    
    escapedbname.SetAs(m_dbname);
    escapedbname.FindAndReplace("`", "``");

    escapestrold.SetAs(m_origtblname);
    escapestrold.FindAndReplace("`", "``");
    
    escapestrnew.SetAs(newname);
    escapestrnew.FindAndReplace("`", "``");
    
    renamequery.AddSprintf("Rename table %s%s%s.%s%s%s to %s%s%s.%s%s%s;", 
		backtick, escapedbname.GetString(), backtick,
		backtick, escapestrold.GetString(), backtick,
		backtick, escapedbname.GetString(), backtick,
		backtick, escapestrnew.GetString(), backtick);

    if(!retval)
    {
        tempstr.SetAs(TABLE_NAME_MISSING);
        renamequery.AddSprintf("\t\t/* %s */", _(tempstr.GetString()));
    }

    return retval;
}

wyBool
TableTabInterface::GetNewTableName(wyString& tablename, wyBool showmsg) //..Gets table name from Textbox
{
    wyWChar     *tblname = NULL;
    wyString    tempstr;
    wyInt32     len = 0;

    tablename.Clear();
    len = SendMessage(m_hedittblname, WM_GETTEXTLENGTH, 0, 0);
    
    tblname = AllocateBuffWChar(len+1);
    SendMessage(m_hedittblname, WM_GETTEXT, len + 1, (LPARAM) tblname);
    
    if(tblname)
    {
        tablename.SetAs(tblname);
        free(tblname);
    }
    
    tablename.RTrim();

    if(tablename.GetLength() == 0)
    {
        if(showmsg)
        {
            tempstr.SetAs(TABLE_NAME_MISSING);
            MessageBox(m_hwnd, tempstr.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
            PostMessage(m_hwnd, UM_SETFOCUS, (WPARAM)m_hedittblname, 0);
        }
        return wyFalse;
    }
    
    return wyTrue;
}

void
TableTabInterface::MarkAsDirty(wyBool  flag, wyBool onsave)
{
    wyString    newtabtitle;
    wyInt32     tabindex = -1;

	if(m_dirtytab == flag)
		return;
	EnableWindow(m_hbtnsave, (flag == wyTrue) ? TRUE : FALSE);
    EnableWindow(m_hbtncancelchanges, (flag == wyTrue) ? TRUE : FALSE);
    m_dirtytab = flag;

	m_mdiwnd->SetQueryWindowTitle();
    
    if(m_isaltertable && !m_open_in_dialog && !onsave)
    {
        newtabtitle.SetAs(m_origtblname);
        tabindex = CustomTab_GetCurSel(m_hwndparent);
        SetTabItem(m_hwndparent, tabindex, newtabtitle, CTBIF_IMAGE | CTBIF_LPARAM, IDI_ALTERTABLE, NULL, flag);
    }
}

void
TableTabInterface:: RefreshObjectBrowser(wyString &dbstr, wyString &tablename)
{
    HTREEITEM       dbhitem = NULL, folderhitem = NULL, objhitem = NULL;
    wyWChar         database[SIZE_512] = {0}, object[SIZE_512] = {0};
    wyString        objname, objname2;
    wyBool          dbfound = wyFalse;

    if(!m_mdiwnd)
        return;

    if(!dbstr.GetLength())
        return;

    dbhitem = TreeView_GetChild(m_mdiwnd->m_pcqueryobject->m_hwnd, TreeView_GetRoot(m_mdiwnd->m_pcqueryobject->m_hwnd));

    while(dbhitem)
    {
        GetNodeText(m_mdiwnd->m_pcqueryobject->m_hwnd, dbhitem, database, SIZE_512 - 1);
        objname.SetAs(database);
        if(!dbstr.Compare(objname))
        {
            dbfound = wyTrue;
            break;
        }
        dbhitem = TreeView_GetNextSibling(m_mdiwnd->m_pcqueryobject->m_hwnd, dbhitem);
    }

    if(!dbfound)
        return;

    TreeView_Expand(m_mdiwnd->m_pcqueryobject->m_hwnd, dbhitem, TVE_EXPAND);

    folderhitem = TreeView_GetChild(m_mdiwnd->m_pcqueryobject->m_hwnd, dbhitem);
    m_mdiwnd->m_pcqueryobject->RefreshParentNode(folderhitem);

    //TreeView_Expand(m_mdiwnd->m_pcqueryobject->m_hwnd, folderhitem, TVE_EXPAND);

    objhitem = TreeView_GetChild(m_mdiwnd->m_pcqueryobject->m_hwnd, folderhitem);

    while(objhitem)
    {
        GetNodeText(m_mdiwnd->m_pcqueryobject->m_hwnd, objhitem, object, SIZE_512 - 1);
        objname.SetAs(object);

        if(objname.Compare(tablename) == 0)
        {
            break;
        }
        objhitem = TreeView_GetNextSibling(m_mdiwnd->m_pcqueryobject->m_hwnd, objhitem);
    }

    if(objhitem)
        TreeView_SelectItem(m_mdiwnd->m_pcqueryobject->m_hwnd, objhitem);
    else
        TreeView_SelectItem(m_mdiwnd->m_pcqueryobject->m_hwnd, dbhitem);
}

wyBool
TableTabInterface:: GetMyResTableStatus()
{
    wyString    query;
    wyChar*         escstr = NULL;
    wyChar          lastchar = '\0';
    wyString        esctblname("");
    wyString        tmpdbname("");

    if(m_myrestablestatus)
        m_mdiwnd->m_tunnel->mysql_free_result(m_myrestablestatus);
    m_myrestablestatus = NULL;

    //..Escaping a single '\' with '\\\\' except the last char of the table name. For the last character, if it is '\', replace it with '\\'
    esctblname.SetAs(m_origtblname);
    if(esctblname.GetCharAt(esctblname.GetLength()-1) == '\\')
    {
        lastchar = '\\';
        esctblname.Strip(1);
    }

    esctblname.FindAndReplace("\\", "\\\\");

    escstr = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), esctblname.GetString());

    if(escstr)
    {
		esctblname.SetAs(escstr);
		free(escstr);
	}
    
    if(lastchar == '\\')
        esctblname.Add("\\\\");

    tmpdbname.SetAs(m_dbname);
    tmpdbname.FindAndReplace("`", "``");

    query.Sprintf("show table status from `%s` like '%s'", tmpdbname.GetString(), esctblname.GetString());

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    m_myrestablestatus = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query);

    SetCursor(LoadCursor(NULL, IDC_ARROW ));
    if(!m_myrestablestatus && m_mdiwnd->m_tunnel->mysql_affected_rows(m_mdiwnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql, query.GetString());
		return wyFalse;
	}
    return wyTrue;
}
void
TableTabInterface::OnDlgWMSizeInfo(LPARAM lparam, HWND hwnd)
{
	MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
	pminmax->ptMinTrackSize.x = m_dlgrect.right - m_dlgrect.left;
    pminmax->ptMinTrackSize.y = m_dlgrect.bottom - m_dlgrect.top;
}

void 
TableTabInterface::OnDlgWMPaint(HWND hwnd)
{
	HDC         hdc;
	PAINTSTRUCT ps;
	
	VERIFY(hdc = BeginPaint(hwnd, &ps));
    DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
	EndPaint(hwnd, &ps);
}

void
TableTabInterface::CreateDlgCancel(HWND hwnd)
{	
	m_hbtncancel = CreateWindowEx(0, WC_BUTTON, _(TEXT("Cance&l")), WS_CHILD | WS_VISIBLE | WS_TABSTOP| WS_GROUP , 0,0, 0,0,
    											m_hwnd, (HMENU)IDCANCEL, 
												(HINSTANCE)pGlobals->m_hinstance, NULL);
	SendMessage(m_hbtncancel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
	ShowWindow(m_hbtncancel , SW_SHOW);		
}
