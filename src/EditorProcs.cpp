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


#include <stdio.h>
#include <string>
#include <scilexer.h>

#include "Scintilla.h"
#include "MDIWindow.h"
#include "Global.h"
#include "EditorProcs.h"
#include "FrameWindowHelper.h"
#include "string.h"
#include "stdio.h"
#include "ExportMultiFormat.h"
#include "Scintilla.h"
#include "PreferenceBase.h"
#include "QueryThread.h"
#include "MDIWindow.h"
#include "EditorFont.h"
#include "CommonHelper.h"
#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;

EditorProcs::EditorProcs(HWND hwnd) : EditorBase(hwnd)
{
	m_hwndparent =   hwnd;
	
}

EditorProcs::~EditorProcs()
{
    if(m_hfont)
		DeleteFont(m_hfont);
}

wyBool
EditorProcs::Create(MDIWindow *wnd, HTREEITEM hitem, wyString *strhitemname)
{
	CreateAdvEdit(wnd, m_hwndparent, hitem, strhitemname);

	memset(&m_lastfind, 0, sizeof (FINDREPLACE));

	return wyTrue;
}

//function to create the richedit window.
HWND
EditorProcs::CreateAdvEdit(MDIWindow *wnd, HWND hwnd, HTREEITEM hitem, wyString *strhitemname)
{
	if (CreateEditor(wnd, hwnd) == NULL)
		return NULL;

	m_hitem = hitem;
	if (strhitemname) {
		m_hitemname.SetAs(strhitemname->GetString());
	}
	else
		m_hitemname.Clear();

	m_dbname.SetAs(wnd->m_pcqueryobject->m_seldatabase);

	m_nodeimage = GetItemImage(wnd->m_pcqueryobject->m_hwnd, TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd));

	switch (m_nodeimage)
	{
	case NTABLE:
		m_nodeimage = NTABLES;
		break;
	case NVIEWSITEM:
		m_nodeimage = NVIEWS;
		break;
	case NEVENTITEM:
		m_nodeimage = NEVENTS;
		break;
	case NSPITEM:
		m_nodeimage = NSP;
		break;
	case NFUNCITEM:
		m_nodeimage = NFUNC;
		break;
	case NTRIGGERITEM:
		m_nodeimage = NTRIGGER;
		break;
	case NFOLDER:
		break;
	case NEVENTS:
	case NVIEWS:
	case NSP:
	case NFUNC:
	case NTABLES:
	case NTRIGGER:
		break;
	case NDATABASE:
		break;
	}

	return m_hwnd;
}

/**
  This function executes all the query in the query window.
  It strtoks through the text with taking ; as a separator and executes each sql statement
  as it gets one.
*/
wyBool
EditorProcs::ExecuteAllQuery(wyInt32 * stop)
{
	wyInt32          curpos, curline;
	wyString		 query;
	
	MDIWindow		 *wnd;
	HWND			 hwnd;

	/*no text then forget it */
	if (!::SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0))
		return wyTrue;

	/* get the query wnd */
	//VERIFY(wnd = GetActiveWin());
    wnd = this->m_pctabeditor->m_parentptr->m_parentptr;

	/* change the status message */
	wnd->m_pcquerystatus->ShowInformation(_(L" Executing Query(s)..."));

	hwnd = StartExecute(wnd, ALL);

	/* reset all the content of combobox */
	//::SendMessage(wnd->GetActiveTabEditor()->m_pctabmgmt->m_pcdataviewquery->m_hwndcombo, CB_RESETCONTENT, 0, 0); 


	curpos	= ::SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
	curline = ::SendMessage(m_hwnd, SCI_LINEFROMPOSITION, curpos, 0);

	GetCompleteText(query);
    wyChar *tmp = AllocateBuff(query.GetLength() + 1);
    strcpy(tmp,query.GetString());
	wyString temp;
    /*
    All stored procedures created via workbench will contain /n as new line character between BEGIN and END which will 
    disturb the formatting when altered via SQLyog hence changing all \n to \r\n and then changing \r\r back to \r
    Previously this was done via function AddCRToLF/ChangeCRtoLF
    */
	temp.SetAs(tmp);
	temp.FindAndReplace("\n","\r\n");
	temp.FindAndReplace("\r\r","\r");
	//wyChar *tmp1 = AllocateBuff(query.GetLength() * 2 + 1);
	//AddCRToLF(tmp, tmp1);
	//ChangeCRToLF(tmp);
    //query.SetAs(tmp1);
	query.SetAs(temp.GetString());
    free(tmp);
	//free(tmp1);
	/* set the flag to executing */
	wnd->SetExecuting(wyTrue);
	*stop = 0;

	ExecuteQueryThread(query.GetString(), stop, wnd, curline, wyTrue);

	return wyTrue;
}

/**
   Function executes the current query. i.e the query in which the cursor is at the moment. 
*/
wyBool
EditorProcs::ExecuteCurrentQuery(wyInt32 * stop, wyBool isexecute)
{	
	return 	ExecuteAllQuery(stop);
}

// Function executes query for updation. Gets the current query and calls the insertupdate  
/**
   This function executes selected query.
   Logic same as above two functions.
 */
wyBool
EditorProcs::ExecuteSelQuery(wyInt32 * stop)
{
    return ExecuteAllQuery(stop);
}

 /// Function to execute the current query
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
wyBool
EditorProcs::HandleQueryExecFinish(wyInt32 * stop, WPARAM wparam)
{
    MDIWindow		 *wnd;
	HWND			 hwnd = NULL;
	QUERYFINISHPARAMS *queryparams = (QUERYFINISHPARAMS *)wparam;
	TabEditor		*tabeditor;
    TabMgmt			*ptabmgmt = NULL;

    /* get the query wnd */
	//VERIFY(wnd = GetActiveWin());
    wnd = this->m_pctabeditor->m_parentptr->m_parentptr;
	
	ptabmgmt = wnd->GetActiveTabEditor()->m_pctabmgmt;	
    tabeditor = (TabEditor*)ptabmgmt->m_tabeditorptr;

	wnd->SetExecuting(wyFalse);

	//now depending upon whether the user had asked to stop the query or not,
	//we need to perform operation accordingly
	if(!*stop) 
	{		
		//(from 6.2) whether resultwindow is hidden or not - issue reported here http://code.google.com/p/sqlyog/issues/detail?id=366
		if(tabeditor->m_isresultwnd == wyFalse)
			tabeditor->m_peditorbase->ShowResultWindow();

		/* we keep the tab control from painting to avoid flickering */
		SendMessage(ptabmgmt->m_hwnd, WM_SETREDRAW, FALSE, 0);

		AddQueryResults(queryparams->list, (*queryparams->str), ptabmgmt, *(queryparams->error), wnd);
		
        wnd->m_pcquerystatus->AddQueryResult((*queryparams->error)?wyFalse:wyTrue);
		SendMessage(ptabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
				
		InvalidateRect(ptabmgmt->m_hwnd, NULL, FALSE);
	} 
	else if(!wnd->m_tunnel->IsTunnel())
	{
		/*if its a tunnel the we dont need to close it */
		if(*queryparams->tmpmysql)        
			wnd->m_tunnel->mysql_close(*queryparams->tmpmysql);
	}

    EndExecute(wnd, hwnd, ALL);

	SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE, IDM_EXECUTE, TBSTATE_INDETERMINATE);
	SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE, ACCEL_QUERYUPDATE, TBSTATE_INDETERMINATE);
	/* set the focus to the correct control */
	if(IsEditorFocus())
		PostMessage(m_hwnd, UM_FOCUS, 0, 0);

	FreeQueryExeFInishParams(queryparams, *stop);

    return wyTrue;
}