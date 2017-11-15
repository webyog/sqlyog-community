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

#include <string.h>
#include <stdio.h>

#include "TabEditor.h"
#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "EditorProcs.h"  
#include "TabEditorSplitter.h"
#include "TabMgmt.h"

#include <scilexer.h>

#define IDC_CTAB		WM_USER+116

extern	PGLOBALS		pGlobals;

TabEditor::TabEditor(HWND hwnd) : TabTypes(hwnd)
{
	m_hwndparent	= hwnd; 
	m_hwnd			= hwnd; 
}

TabEditor::~TabEditor()
{
    TabMgmt* ptabmgmt;

    if(m_peditorbase)
	{
		delete m_peditorbase;
		m_peditorbase = NULL;
	}

	if(m_pctabmgmt)
	{
        ptabmgmt = m_pctabmgmt;
        m_pctabmgmt = NULL;
		delete ptabmgmt;
	}

	if(m_pcetsplitter)
	{
		delete m_pcetsplitter;
		m_pcetsplitter = NULL;
	}	
}
//Function to create a TabEditor, it includes EditorQuery/EditorProc, Splitter & TabMgmt.
wyBool 
TabEditor::Create(MDIWindow *wnd , HTREEITEM hitem, wyBool iseditquery, wyString *strhitemname)
{
	CreateTabEditorSplitter(wnd);
	SetEvent(pGlobals->m_pcmainwin->m_sessionchangeevent);
	if(iseditquery == wyTrue)
		CreateEditorQuery(wnd);	
	else
        CreateAdvEdit(wnd, hitem, strhitemname);
	
	CreateTabMgmt(wnd);		
	
	return wyTrue;
}

//function to create the H-splitter in TabEditor
void	
TabEditor::CreateTabEditorSplitter(MDIWindow *wnd)
{	
	
	if(pGlobals->m_pcmainwin->m_toppercent == 0)
	{
		m_pcetsplitter	= new TabEditorSplitter(m_hwnd, 50);

		m_pcetsplitter->Create();
		m_pcetsplitter->m_lasttoppercent = 30;
	}

	else
	{
		m_pcetsplitter	= new TabEditorSplitter(m_hwnd, pGlobals->m_pcmainwin->m_toppercent);
        
		m_pcetsplitter->Create();
		m_pcetsplitter->m_lasttoppercent = pGlobals->m_pcmainwin->m_toppercent;

        if(m_pcetsplitter->m_lasttoppercent == 100)
        {
            // Since the Toppercent(Query) is taking 100 ~ we are not showing the resultpan. 
            // So keep the result pan flag as false.
           m_isresultwnd = wyFalse;
        }
	}
	
	m_pcetsplitter->SetTabEditorPtr(this);
	
	return;
}


//Function to create the Query Editor tab('EditorQuery')
wyBool
TabEditor::CreateEditorQuery(MDIWindow * wnd)
{
	EditorQuery *editorquery = new EditorQuery(m_hwnd);
	
	editorquery->Create(wnd);
	editorquery->SetTabEditorptr(this);  
	
	m_peditorbase = editorquery;
	
	return wyTrue;
}

wyBool
TabEditor::CreateAdvEdit(MDIWindow *wnd, HTREEITEM hitem, wyString *strhitemname)
{
	EditorProcs *editorproc = new EditorProcs(m_hwnd);

	editorproc->Create(wnd, hitem, strhitemname);
	editorproc->SetAdvancedEditor(wyTrue);
	editorproc->SetTabEditorptr(this);  // sets tabEditor pointer in EditorBase
	m_peditorbase = editorproc;

	return wyTrue;
}

// Function to create the Result Tab ('TabMgmt')
VOID 
TabEditor::CreateTabMgmt(MDIWindow * wnd)
{
	m_pctabmgmt = new TabMgmt(m_hwnd, wnd);

	m_pctabmgmt->SetTabEditorPtr(this); 
	m_pctabmgmt->Create();
	m_pctabmgmt->ChangeTitles();

	return;
}

//Function to Resize the tab
void
TabEditor::Resize(wyBool issplittermoved)
{
	RECT rcthwnd;

	VERIFY(GetWindowRect(m_hwnd, &rcthwnd));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcthwnd, 2));

	m_pcetsplitter->Resize(rcthwnd);	
    
    //For getting the previous splitter position.
    pGlobals->m_pcmainwin->m_toppercent = m_pcetsplitter->m_leftortoppercent;

	//if(issplittermoved == wyTrue)
	//	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);

	m_peditorbase->Resize();
	m_pctabmgmt->Resize();	
	return;
}

//Show tab contents while closing other
void 
TabEditor::ShowTabContent(wyInt32 tabindex, wyBool status)
{
	BOOL        val = (status == wyTrue)?TRUE:FALSE;
    
	ShowWindow(m_peditorbase->m_hwnd , val);
	ShowWindow(m_peditorbase->m_hwndhelp, val);

	ShowWindow(m_pcetsplitter->m_hwnd, val);

	ShowWindow(m_pctabmgmt->m_hwnd, val);

	if (val)
		SetFocus(m_peditorbase->m_hwnd);

	return;
}

void
TabEditor::OnTabSelChanging()
{

}
void
TabEditor::OnTabSelChange()
{
	MDIWindow		*pcmdiwindow = m_parentptr->m_parentptr;
	EditorBase      *pceditorbase;
    CQueryObject    *obj;
    HWND            hwndtoolbar = pGlobals->m_pcmainwin->m_hwndtool;
	HWND            hwndsecondtool = pGlobals->m_pcmainwin->m_hwndsecondtool; 
    wyInt32         image;
	wyString		history;

	/// Destroy if Find dilaog box is present
	if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

    pceditorbase = pcmdiwindow->GetActiveTabEditor()->m_peditorbase;     

	HandleEditMenu();	

	pcmdiwindow->HandleToolBar();
	pcmdiwindow->SetQueryWindowTitle();

    /// If the tab type is advance tab then we need to disable the execute button 
    if(pceditorbase->GetAdvancedEditor() == wyTrue)
	{ 
		SendMessage(hwndtoolbar, TB_SETSTATE, (WPARAM)IDM_EXECUTE, TBSTATE_INDETERMINATE);
		//SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)ID_FORMATCURRENTQUERY, TBSTATE_INDETERMINATE);
		SendMessage(hwndtoolbar, TB_SETSTATE, (WPARAM)ACCEL_QUERYUPDATE, TBSTATE_INDETERMINATE);
        SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)ID_FORMATCURRENTQUERY, TBSTATE_ENABLED);
	}
    else
	{
        SendMessage(hwndtoolbar, TB_SETSTATE,(WPARAM)IDM_EXECUTE, TBSTATE_ENABLED);
#ifndef COMMUNITY
	if(pcmdiwindow->m_conninfo.m_isreadonly == wyTrue)
	{
		SendMessage(hwndtoolbar, TB_SETSTATE,ACCEL_QUERYUPDATE, TBSTATE_INDETERMINATE);
	}
	else
#endif
		SendMessage(hwndtoolbar, TB_SETSTATE, (WPARAM)ACCEL_QUERYUPDATE, TBSTATE_ENABLED);
		SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)ID_FORMATCURRENTQUERY, TBSTATE_ENABLED);
	}

    //Enabling tool buttons
    obj = pcmdiwindow->m_pcqueryobject;    
    image	= obj->GetSelectionImage();
    if(image == NTABLE)
        SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_OBJECT_INSERTUPDATE, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_OBJECT_VIEWDATA, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_TABLE_OPENINNEWTAB, TBSTATE_ENABLED);
    m_pctabmgmt->OnTabSelChange();
	return;
}

wyInt32
TabEditor::OnTabClosing(wyBool ismanual)    
{
	wyString		msg, caption;
	wyInt32			ret;
	MDIWindow		*pmdiwin	 = m_parentptr->m_parentptr;
	EditorBase		*peditorbase = m_parentptr->GetActiveTabType()->m_peditorbase;
	
	MDIWindow		*wnd = NULL;

    if(peditorbase->m_save == wyFalse && SendMessage(peditorbase->m_hwnd, SCI_GETTEXTLENGTH, 0, 0) == 0)
    {
        return 1;
    }
	//no dialog if wnd->m_ismdiclosealltabs == wyTrue and session save is running
	if(pGlobals->m_pcmainwin->m_iscloseallmdi && pGlobals->m_issessionsaveactive)
    {
        return 1;
    }
	if((peditorbase->m_edit == wyTrue)&& IsConfirmOnTabClose() == wyTrue 
			|| (peditorbase->m_save == wyTrue) && (peditorbase->m_edit == wyTrue))
	{
		VERIFY(wnd = GetActiveWin());

        if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
        {
            SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, TRUE, 0);
            InvalidateRect(pGlobals->m_pcmainwin->m_hwndconntab, NULL, TRUE);
	        UpdateWindow(pGlobals->m_pcmainwin->m_hwndconntab);
        }

		//To paint while closing all tabs in Con window
		if(wnd && wnd->m_ismdiclosealltabs == wyTrue)
		{
			SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
			
			//post 8.01 painting, Paint the tabs once the message box promt for 'Saving'
			InvalidateRect(m_hwnd, NULL, TRUE);
			UpdateWindow(m_hwnd);
		}

		if(peditorbase->m_save == wyFalse)					
			msg.Sprintf(_("The content of this tab has been changed.\n\nDo you want to save the changes?"));
		else
			msg.Sprintf(_("The content of the %s file has been changed.\n\nDo you want to save the changes?"), peditorbase->m_filename.GetString());

        if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue || wnd->m_ismdiclosealltabs == wyFalse)
            caption.SetAs(pGlobals->m_appname);
        else
            caption.Sprintf("%s - [%s]", pGlobals->m_appname.GetString(), wnd->m_title.GetString());

        if(wnd->m_mdisaveselection == -1)
            ret = CustomSaveMessageBox(m_hwnd, msg.GetAsWideChar(), caption.GetAsWideChar(), wnd->m_ismdiclosealltabs);
        else
            ret = wnd->m_mdisaveselection;
		
		switch(ret)
		{
			case IDCANCEL:
				return 0;
					
			case IDYES:
				if(pmdiwin->SaveFile(wyTrue) == wyFalse)
					return 0;
				break;

            case IDYESTOALL:
                                
                if(pmdiwin->SaveFile(wyTrue) == wyFalse)
                    return 0;

                wnd->m_mdisaveselection = ret;
                break;

            case IDNOTOALL:
                wnd->m_mdisaveselection = ret;
		}

		//Again block paint to avaoid flickering while closing All tabs in con- window
		if(wnd && wnd->m_ismdiclosealltabs == wyTrue)
			SendMessage(wnd->m_pctabmodule->m_hwnd, WM_SETREDRAW, FALSE, 0);		

        if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
        {
            SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, FALSE, 0);
        }
	}
					
	return 1;	
}

void
TabEditor::OnTabClose()
{
	MDIWindow	*pmdiwin	= m_parentptr->m_parentptr;
    HWND        hwndtoolbar = pGlobals->m_pcmainwin->m_hwndtool;
	HWND        hwndsecondtool = pGlobals->m_pcmainwin->m_hwndsecondtool; 
    EditorBase  *pceditorbase;
	
    pceditorbase = pmdiwin->GetActiveTabEditor()->m_peditorbase;
	
	//pmdiwin->SetQueryWindowTitle();

    /// If the tab type is advance tab then we need to disable the execute button and format current query button
    if(pceditorbase->GetAdvancedEditor() == wyTrue)
	{
		SendMessage(hwndtoolbar, TB_SETSTATE, (WPARAM)IDM_EXECUTE, TBSTATE_INDETERMINATE);
		SendMessage(hwndtoolbar, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE);
		SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)ID_FORMATCURRENTQUERY, TBSTATE_INDETERMINATE);
	}
	else
	{
		SendMessage(hwndtoolbar, TB_SETSTATE,(WPARAM)IDM_EXECUTE, TBSTATE_ENABLED);
		SendMessage(hwndtoolbar, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_ENABLED);
		SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)ID_FORMATCURRENTQUERY, TBSTATE_ENABLED);
	}
	
	//To enable executeall buttons when on closing tabs focus comes on query tab
	SendMessage(hwndtoolbar, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_ENABLED);
    
	return;
}

// Handle Menu items for each TabEditor (CTRL+L, CTRL+2, CTRL+3)
void
TabEditor::HandleEditMenu()
{
	TabModule		*ptabmodule = m_parentptr;

	if(m_istextresult == wyTrue)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyTrue , IDM_EDIT_RESULT_TEXT);
	else if(m_istextresult == wyFalse)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyFalse , IDM_EDIT_RESULT_TEXT);
   
	if(m_isresultwnd == wyTrue)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyFalse, IDC_EDIT_SHOWRESULT);
	else if(m_isresultwnd == wyFalse)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyTrue, IDC_EDIT_SHOWRESULT);
      
	if(m_iseditwnd == wyTrue)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyFalse, IDC_EDIT_SHOWEDIT);
	else if(m_iseditwnd == wyFalse)
		ptabmodule->HandleCheckMenu(ptabmodule->m_parentptr, wyTrue, IDC_EDIT_SHOWEDIT);

	return;
}

// Function for closing a 'TabEditor'
wyBool
TabEditor::CloseTab(wyInt32 index)
{	
   // delete this;

    return wyTrue;
}


/// HAndles when MDIWindow is closing.
wyInt32
TabEditor::OnWmCloseTab()
{
	wyString		msg;
	wyInt32			ret;
	MDIWindow		*pmdiwin	 = m_parentptr->m_parentptr;
	
	if(((m_peditorbase->m_edit)&& IsConfirmOnTabClose())||((m_peditorbase->m_save)&&(m_peditorbase->m_edit)))
	{
		if(m_peditorbase->m_save == wyFalse)
			msg.Sprintf(_("The content of this tab has changed.\n\nDo you want to save the changes?"));
		else
			msg.Sprintf(_("The content of the %s file has changed.\n\nDo you want to save the changes?"), 
               m_peditorbase->m_filename.GetString());
	
		ret = yog_message(pmdiwin->m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_HELP);
						
		switch(ret)
		{
		case IDCANCEL:
				return 0;

		case IDYES:
				if(pmdiwin->SaveFile() == wyFalse)
					return 0;
				break;
		}
	}
	SetEvent(pGlobals->m_pcmainwin->m_sessionchangeevent);
	return 1;
}

VOID
TabEditor::HandleTabControls(wyInt32 tabcount, wyInt32 selindex)
{
	ConnectionBase *pconnection	= pGlobals->m_pcmainwin->m_connection;	
	
	pconnection->HandleEditorControls(m_peditorbase->m_hwnd, m_peditorbase->m_hwndhelp, 
										NULL, m_pcetsplitter->m_hwnd, 
										m_pctabmgmt->m_hwnd, m_peditorbase->m_save,	tabcount,selindex);
	
	return;
}	

 VOID	
TabEditor::HandleFlicker()
 {
	MDIWindow	*wnd = NULL;
	RECT		rcted, rctin, rctscroll, rctstatic, rcttabmgmt;
	wyInt32		scrollwidth = 0, height = 0;

	wnd = GetActiveWin();

	scrollwidth = GetSystemMetrics(SM_CXHTHUMB); 
	
	GetClientRect(m_hwnd, &rctin); //Tab cordinate

	//Editor cordinates
	GetWindowRect(m_peditorbase->m_hwnd, &rcted);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcted, 2);
	
	//Tabmgmt cordinate
	GetWindowRect(m_pctabmgmt->m_hwnd, &rcttabmgmt);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttabmgmt, 2);
		    
	//Area btw editor botttom and Tabmgmt
	rctscroll.left = rcted.left;
	rctscroll.right = rcted.right;
	rctscroll.top = rcted.bottom - scrollwidth;

	rctscroll.bottom = rcted.bottom + scrollwidth + height;

	InvalidateRect(m_hwnd, &rctscroll, TRUE);
	UpdateWindow(m_hwnd);
				
	//Left side before editor
	rctstatic.left = rctin.left;
	rctstatic.right = rcted.left + 5;
	rctstatic.top = rctin.top;
	rctstatic.bottom = rctin.bottom;
	InvalidateRect(m_hwnd, &rctstatic, TRUE);

	//Right-side after editor
	rctstatic.left = rcted.right - 5;
	rctstatic.right = rctin.right;
	rctstatic.bottom = rctin.bottom;
	InvalidateRect(m_hwnd, &rctstatic, TRUE);

	//Down after grid
	rctstatic.top = rcttabmgmt.bottom - scrollwidth;
	rctstatic.right = rcttabmgmt.right;
	rctstatic.left = rcttabmgmt.left;
	rctstatic.bottom = rcttabmgmt.bottom;
	InvalidateRect(m_hwnd, &rctstatic, TRUE);

	//Top of the tabs
	rctstatic.top = rctin.top;
	rctstatic.right = rctin.right;
	rctstatic.left = rctin.left;

	rctstatic.bottom = rcted.top + 3;
	InvalidateRect(m_hwnd, &rctstatic, TRUE);
		
	//For avoiding painting issue when editor is hidden by resizing window
	if(wnd && wnd->m_hwnd)
	{
		if(rcted.bottom - rcted.top == 0)
			InvalidateRect(wnd->m_hwnd, &rctin, TRUE);
	}

 }


