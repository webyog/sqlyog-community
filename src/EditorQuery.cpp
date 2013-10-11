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


#include <scilexer.h>

#include "Scintilla.h"
#include "MDIWindow.h"
#include "Global.h"
#include "EditorQuery.h"
#include "FrameWindowHelper.h"
#include "string.h"
#include "stdio.h"
#include "ExportMultiFormat.h"
#include "PreferenceBase.h"
#include "QueryThread.h"
#include "EditorFont.h"
#include "CommonHelper.h"
#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;

EditorQuery::EditorQuery(HWND hwnd) : EditorBase(hwnd)
{
	m_hwnd = NULL;
	m_hwndparent =   hwnd;
	m_hwndhelp = NULL;
	m_hfont = NULL;
}

EditorQuery::~EditorQuery()
{
	if(m_hfont)
		DeleteFont(m_hfont);

	m_hfont = NULL;
}

wyBool
EditorQuery::Create(MDIWindow * wnd)
{
	CreateQueryEdit(wnd, m_hwndparent);

    memset(&m_lastfind, 0, sizeof(FINDREPLACE));
	
	return wyTrue;
}

//function to create the richedit window.
HWND
EditorQuery::CreateQueryEdit(MDIWindow * wnd, HWND hwnd)
{
    if(CreateEditor(wnd, hwnd) == NULL)
        return NULL;

	return m_hwnd;
}

/** 
    This function executes all the query in the query window.
    It strtoks through the text with taking ; as a separator and executes each sql statement
    as it gets one.
*/
wyBool
EditorQuery::ExecuteAllQuery(wyInt32 *stop)
{
	wyInt32          curpos, curline, start, end, len;
	wyString		 query;
    wyChar           *tmp;
    wyBool           selquery = wyFalse;
	
	MDIWindow		 *wnd;
	HWND			 hwnd;
	TabMgmt			 *ptabmgmt = NULL;
	
	/*no text then forget it */
	if(!SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0))
		return wyTrue;

    //SetGroupProcess(wyTrue);

    /* see if we require to execute selected query */
	start = SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end   = SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0);

	if((start-end) != 0) 
        selquery = wyTrue;

	/* get the query wnd */
	//VERIFY(wnd = GetActiveWin());
    wnd = this->m_pctabeditor->m_parentptr->m_parentptr;

	/* change the status message */
	wnd->m_pcquerystatus->ShowInformation(_(L"Executing Query(s)..."));

	hwnd = StartExecute(wnd, ALL);
	ptabmgmt = wnd->GetActiveTabEditor()->m_pctabmgmt;

	/* resent all the content of combobox */
	//SendMessage(ptabmgmt->m_pcdataviewquery->m_hwndcombo, CB_RESETCONTENT, 0, 0);                                

	curpos	= SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
	curline = SendMessage(m_hwnd, SCI_LINEFROMPOSITION, curpos, 0);
	
    /*	starting from v5.1 RC1, we use this function for both selected and all query execution */
	if(selquery == wyFalse)
    {
		GetCompleteText(query);
		tmp = AllocateBuff(query.GetLength() + 1);
		strcpy(tmp, query.GetString());
    }
	else
    {
		len = SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0);
        tmp = AllocateBuff(len + 1);
		SendMessage(m_hwnd, SCI_GETSELTEXT, 0, (LPARAM)tmp);
    }
    // Commented Because to Solve an Extra '\n' appending bug in http://www.webyog.com/forums//index.php?showtopic=3556
    ChangeCRToLF(tmp);
    query.SetAs(tmp);
    free(tmp);

	/* set the flag to executing */
	wnd->SetExecuting(wyTrue);
	*stop = wyFalse;
	wnd->m_lastfocus = m_hwnd;

	ExecuteQueryThread(query.GetString(), stop, wnd, curline);

	return wyTrue;
}


wyBool
EditorQuery::ExecuteExplainQuery(wyInt32 * stop, wyBool isExtended)
{
    wyString		 *query = new wyString;
    HWND             hwnd;
	HANDLE			 evt;
	QueryThread	 	 thd;
	MDIWindow		 *wnd;
    QueryResultList	 *list = new QueryResultList;
    wyString		 *str = new wyString;
	PMYSQL			 tmpmysql;
	QUERYTHREADPARAMS	*param    = NULL;
	TabMgmt				*ptabmgmt = NULL;
    wyInt32				*err	= new wyInt32;
	wyUInt32			start, end, curpos, curline; 
	TabEditor			*tabeditor;
    wyChar              *tmp;

	*err	= 0;
    param					= new QUERYTHREADPARAMS;

	/*no text then forget it */
	if(!SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0))
		return wyTrue;

	/* see if we require to execute selected query */
	start	= SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end		= SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0);
    curpos	= SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
	curline = SendMessage(m_hwnd, SCI_LINEFROMPOSITION, curpos, 0);
	
	if((start-end) > 1)
    {
        tmp = AllocateBuff(end - start + 1);
		SendMessage(m_hwnd, SCI_GETSELTEXT, 0, (LPARAM)tmp);
        query->SetAs(tmp);
        free(tmp);
        param->startpos = 0; 
        param->endpos = 0; 
        param->executestatus = isExtended == wyTrue? EXECUTE_SELECTED_EXTENDED : EXECUTE_SELECTED_EXPLAIN;
    }
    else
    {
        GetCompleteText(*query);
        param->startpos = curpos; 
        param->endpos = 0; 
        param->executestatus = isExtended == wyTrue? EXECUTE_CURRENT_EXTENDED : EXECUTE_CURRENT_EXPLAIN;
    }

    wnd = this->m_pctabeditor->m_parentptr->m_parentptr;
	
	/* change the status message */
	wnd->m_pcquerystatus->ShowInformation(_(L" Executing Explain..."));

	hwnd = StartExecute(wnd, SINGLE);
	ptabmgmt = wnd->GetActiveTabEditor()->m_pctabmgmt;	

	tabeditor = (TabEditor*)ptabmgmt->m_tabeditorptr;

	/* set the flag to executing */
	wnd->SetExecuting(wyTrue);
	
	*stop   =  wyFalse;
	wnd->m_stopmysql=  wnd->m_mysql;

    tmpmysql = &wnd->m_stopmysql;

    param->linenum = curline, 
    param->query = query;
    param->stop = stop; 
    param->list = list; 
    param->str = str;

	param->tab = wnd->GetActiveTabEditor()->m_pctabmgmt;
    param->tunnel = wnd->m_tunnel; 
    param->mysql = &wnd->m_mysql; 
	param->tmpmysql = tmpmysql; 
    param->error = err; 
    param->isadvedit = wyFalse; 
    param->lpcs = &wnd->m_cs;
    param->wnd  = wnd;
	param->isprofile = wyTrue;
	param->m_highlimitvalue = -1;
	param->m_lowlimitvalue = -1;
	param->m_iseditor = wyTrue;
	param->executeoption = SINGLE;
	param->isedit = wyFalse;
	param->isexplainextended = isExtended;
	param->isexplain = wyTrue;
	
    InitializeExecution(param);

	evt = thd.Execute(param);
	
	return wyTrue;
}
//Function executes the current query. i.e the query in which
//the cursor is at the moment. 
wyBool
EditorQuery::ExecuteCurrentQuery(wyInt32 * stop, wyBool isedit)
{	
    wyString		 *query;
    HWND             hwnd;
	HANDLE			 evt;
	QueryThread	 	 thd;
	MDIWindow		 *wnd;
    QueryResultList	 *list;
    wyString		 *str;
	PMYSQL			 tmpmysql;
	QUERYTHREADPARAMS	*param    = NULL;
	TabMgmt				*ptabmgmt = NULL;
    wyInt32				*err = new wyInt32;
	wyUInt32			start, end, curpos, curline; 
	TabEditor			*tabeditor;

    str = new wyString;
    query = new wyString;
    list = new QueryResultList;

	/*no text then forget it */
	if(!SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0))
		return wyTrue;

	/* see if we require to execute selected query */
	start	= SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end		= SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0);
	
	if((start-end) > 1)
		return ExecuteSelQuery(stop);

    *err = 0;

	/* get the query wnd */
	//VERIFY(wnd = GetActiveWin());
    wnd = this->m_pctabeditor->m_parentptr->m_parentptr;
	
	/* change the status message */
	wnd->m_pcquerystatus->ShowInformation(_(L" Executing Query(s)..."));

	hwnd = StartExecute(wnd, SINGLE);
	ptabmgmt = wnd->GetActiveTabEditor()->m_pctabmgmt;	

	tabeditor = (TabEditor*)ptabmgmt->m_tabeditorptr;

	/* resent all the content of combobox */
	//SendMessage(ptabmgmt->m_pcdataviewquery->m_hwndcombo, CB_RESETCONTENT, 0, 0);                                 

	curpos	= SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
	curline = SendMessage(m_hwnd, SCI_LINEFROMPOSITION, curpos, 0);

    GetCompleteText(*query);
	//ChangeCRToLF ( query );

	/* set the flag to executing */
	wnd->SetExecuting(wyTrue);
	
	*stop   =  wyFalse;
	wnd->m_stopmysql=  wnd->m_mysql;

    tmpmysql = &wnd->m_stopmysql;

    param					= new QUERYTHREADPARAMS;

    param->startpos = curpos; 
    param->endpos = 0; 
    param->linenum = curline, 
    param->executestatus = EXECUTE_CURRENT;
    param->query = query;
    param->stop = stop; 
    param->list = list; 
    param->str = str;

	param->tab = wnd->GetActiveTabEditor()->m_pctabmgmt;
    param->tunnel = wnd->m_tunnel; 
    param->mysql = &wnd->m_mysql; 
	param->tmpmysql = tmpmysql; 
    param->error = err; 
    param->isadvedit = wyFalse; 
    param->lpcs = &wnd->m_cs;
    param->wnd  = wnd;
	param->isprofile = wyTrue;
	param->m_highlimitvalue = -1;
	param->m_lowlimitvalue = -1;
	param->m_iseditor = wyTrue;
	param->executeoption = SINGLE;
	param->isedit = isedit;
	param->isexplain = wyFalse;

    InitializeExecution(param);

	evt = thd.Execute(param);
    return wyTrue;
}

wyBool
EditorQuery::ExecuteSelQuery(wyInt32 *stop)
{
    //starting from v5.1, we dont discriminate between select and all query
    //and they both go thru the same function 
    return ExecuteAllQuery(stop);
}

wyBool 
EditorQuery::HandleQueryExecFinish(wyInt32 * stop, WPARAM wparam)
{
    MDIWindow		*wnd;
    TabEditor		*tabeditor;
    TabMgmt			*ptabmgmt = NULL;
	
    QUERYFINISHPARAMS   *queryparams = (QUERYFINISHPARAMS*) wparam;

    /* get the query wnd */
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

		//query execution is successful or not
		wnd->m_querysuccessful = *(queryparams->error);

		/* we keep the tab control from painting to avoid flickering */
		SendMessage(ptabmgmt->m_hwnd, WM_SETREDRAW, FALSE, 0);

		//Diabe the Form view option in read-only mode
		/*if(isedit == wyFalse &&	ptabmgmt && ptabmgmt->m_pcdataviewquery)
		{
			ptabmgmt->m_pcdataviewquery->m_isformview = wyFalse;
		}*/
		 

		if(queryparams->isexplain == wyFalse){
			AddQueryResults(queryparams->list, (*queryparams->str), ptabmgmt, *(queryparams->error), wnd);
		}
		else{
			AddExplainQueryResults(queryparams->list, (*queryparams->str), ptabmgmt, *(queryparams->error), queryparams->isexplainextended, wnd);
		}
		
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

    EndExecute(wnd, wnd->m_hwnd, queryparams->executeoption);

	//from 4.05 BETA 3 we select the last edited table by default if found 
    if(queryparams->isedit == wyTrue)
        SelectFirstTableToEdit(queryparams->isedit);
           
	FreeQueryExeFInishParams(queryparams, *stop);
			
	// set the focus to the correct control 
	SetFocusToEditor(wnd, m_hwnd);
	
	return wyTrue;
}
