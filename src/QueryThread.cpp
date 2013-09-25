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
#include "QueryThread.h"
#include "SQLTokenizer.h"
#include "CommonHelper.h"
#include "pcre.h"
#include "SQLFormatter.h"

#ifndef COMMUNITY
#include "HelperEnt.h"
#endif

extern	PGLOBALS		 pGlobals;

#define WARNINGMSG		_("Note: To see complete list of warning(s), enable Tools -> Preferences -> General -> Show Warning(s) in Messages Tab")
#define SPACE_15		15
#define			NO_QUERY_EXECUTED		_("No query(s) were executed. Please enter a query in the SQL window or place the cursor inside a query.")

void
AddErrorOrMsg(wyInt32 errstatus, wyString& buffer, Tunnel * tunnel, PMYSQL mysql, const wyChar* query, 
				wyInt32 executestatus, wyUInt32 timetaken, wyUInt32 transfertime , wyUInt32 totaltime, wyBool isembedformat)
{
	wyUInt32		errnum = 0, affectedrows = 0, warningcount = 0;
    wyString        msg, exectimestr, transfertimestr, totaltimestr, timestr, querystr, tempdump;
    SQLFormatter    formatter;

	errnum = tunnel->mysql_errno(*mysql);
	
	//convering milliseconds to a formatted time (hh:mm:ss:mss) 
	GetTime(timetaken, exectimestr);
	GetTime(transfertime, transfertimestr);
	GetTime(totaltime, totaltimestr);

	if(tunnel->IsTunnel() == false)//if it is http we are not displaying execution time &transfer time
		timestr.Sprintf("\r\n%-*s: %s\r\n%-*s: %s", SPACE_15,_("Execution Time"), exectimestr.GetString(), SPACE_15, _("Transfer Time"), transfertimestr.GetString());

	timestr.AddSprintf("\r\n%-*s: %s\r\n", SPACE_15, _("Total Time"), totaltimestr.GetString());

    if(query)
    {
        tempdump.SetAs(query);
        formatter.GetQueryWtOutComments(&tempdump, &querystr);
        querystr.LTrim();
        querystr.RTrim();
        //querystr.SetAs(query);
        if(!querystr.GetLength())
        {
            querystr.SetAs(query);
        }
        FormatQueryByRemovingSpaceAndNewline(&querystr);

	    //truncate the query, if it is big
	    if(querystr.GetLength() > SIZE_128)
	    {
		    querystr.Strip(querystr.GetLength() - SIZE_128);
		    querystr.Add("...");
	    }
    }

	switch(errstatus)
	{
	case ERROR_ERROR:
		if(!errnum)
        {
            break;
        }

        if(isembedformat == wyTrue)
        {
            msg.SetAs("<e>");
        }

        if(query)
        {
	        msg.AddSprintf(_("Query: %s"), querystr.GetString());
        }

        msg.Add("\r\n\r\n");
		msg.AddSprintf(_("Error Code: %d\r\n%s"), errnum, tunnel->mysql_error(*mysql));
		msg.AddSprintf("\r\n%s", timestr.GetString());	
		break;

	case ERROR_RESULT:
        if(isembedformat == wyTrue)
        {
            msg.SetAs("<r>");
        }

        if(query)
        {
	        msg.AddSprintf(_("Query: %s"), querystr.GetString());
        }

        msg.Add("\r\n\r\n");
        affectedrows = tunnel->mysql_affected_rows(*mysql);
		msg.AddSprintf(_("%lu row(s) returned"), affectedrows);
		msg.AddSprintf("\r\n%s", timestr.GetString());
		break;

	case ERROR_NONRESULT:
        affectedrows = tunnel->mysql_affected_rows(*mysql);
		warningcount = tunnel->mysql_warning_count(*mysql);

        if(isembedformat == wyTrue)
        {
            msg.SetAs(warningcount ? "<w>" : "<n>");
        }

        if(query)
        {
	        msg.AddSprintf(_("Query: %s"), querystr.GetString());
        }

        msg.Add("\r\n\r\n");
        msg.AddSprintf(_("%lu row(s) affected"), affectedrows);
		
		if(warningcount > 0)
			msg.AddSprintf(_(", %lu warning(s)"), warningcount);
		
		msg.AddSprintf("\r\n%s", timestr.GetString());

		//If warning is present, show warning if checked in preferences
		if(warningcount > 0)
			GetWarning(tunnel, mysql, &msg);

		break;

	default:
		VERIFY(0);		/* should never reach here */
		break;
	}

    if(query && buffer.GetLength() && msg.GetLength())
	{
        buffer.Add("--------------------------------------------------\r\n\r\n");
	}

	buffer.Add(msg.GetString());
}

/* executes query, initialisez the correct structure and adds it to the query list */
wyInt32		
HelperExecuteQuery(QUERYTHREADPARAMS * param, const wyChar* query)
{
	wyUInt32        len;
	wyInt32         ret;
	MYSQL_RES       *myres;
	QueryResultElem *elem = NULL;
	wyInt64			exectime =0, transfertime = 0, totaltime = 0;
	wyBool			isselectrange = wyFalse, ismultiresult = wyFalse;
	wyString		querytemp;
    const wyChar*   ptrquery;
		
	/* just check if query has been asked to stop */
	len = strlen(query);
    
	if(!len)
        return 0;

#ifndef COMMUNITY	
	//Show status variables for PQA before executing 'select' query
	if(pGlobals->m_ispqaenabled == wyTrue 
		&& param->m_iseditor == wyTrue
		&& param->isprofile == wyTrue 
		&& param->wnd->m_isselectquery == wyFalse &&
        param->isadvedit == wyFalse)

	{
		SetPQABeforeExecutionOptions(param, param->wnd, query);

		if(IsQueryStopped(param) == wyTrue)
		{			
			return 0;
		}
	}

#endif
    	
	querytemp.SetAs(query);

	//if(param->m_highlimitvalue != -1 && param->m_lowlimitvalue != -1)
	if(param->m_islimitpresent == wyFalse)
		isselectrange = wyTrue;

	//Append the LIMIT explicitly
	if(isselectrange == wyTrue && param->m_lowlimitvalue!= -1 && param->m_highlimitvalue != -1)
		querytemp.AddSprintf(" LIMIT %d, %d", param->m_lowlimitvalue, param->m_highlimitvalue);		

    ptrquery = querytemp.GetString();
	param->wnd->SetThreadBusy(wyTrue);
	ret		= my_query(param->wnd, param->tunnel, param->mysql, querytemp.GetString(), querytemp.GetLength(), 
                                        wyFalse, wyFalse, param->stop);
	param->wnd->SetThreadBusy(wyFalse);
	
	if(ret)
	{
		elem = new QueryResultElem;
		elem->param = NULL;
        totaltime = param->wnd->m_lastquerytime;
		if(param->executestatus != EXECUTE_SELECTED_EXPLAIN && param->executestatus != EXECUTE_SELECTED_EXTENDED 
			&& param->executestatus != EXECUTE_CURRENT_EXPLAIN && param->executestatus != EXECUTE_CURRENT_EXTENDED)
		{
			AddErrorOrMsg(ERROR_ERROR, *param->str, param->tunnel, param->mysql, ptrquery, param->executestatus, param->endpos, 0, param->wnd->m_lastquerytime, param->m_iseditor);	
		}
		else
		{
			ShowMySQLError(param->wnd->m_hwnd, param->wnd->m_tunnel, &param->wnd->m_mysql, querytemp.GetString());
		}

		param->list->Insert(elem);
	} 
	else 
	{
		do
		{
            if(elem)
            {
                ismultiresult = wyTrue;

                if(elem->param)
                {
                    elem->param->m_ispartofmultiresult = ismultiresult;
                }
            }

			/* whatever the result might be we have to allocate a queryresult elem */
			elem = new QueryResultElem;
			elem->param = NULL;

			myres	= param->tunnel->mysql_store_result(*param->mysql, false, false, (void*)param->wnd);
			param->wnd->m_transfertime = GetHighPrecisionTickCount();
			
			if(param->tunnel->IsTunnel() == wyTrue)
			{
				totaltime = param->wnd->m_lastquerytime;
			}
			else
			{   
				exectime = param->wnd->m_lastquerytime;
				transfertime = param->wnd->m_transfertime - param->wnd->m_execendtime;
				totaltime = param->wnd->m_transfertime - param->wnd->m_execstarttime;
			}
			
			if(IsQueryStopped(param))
            {
                if(myres)
                    param->tunnel->mysql_free_result(myres);
				delete elem;
				return 0;
            }

			if(!myres && (param->tunnel->mysql_affected_rows(*param->mysql)) != -1)
            {
                if(IsMySQL563(param->wnd->m_tunnel, &param->wnd->m_mysql) && IsQueryDeleteInsertReplaceUpdate(query))
                {
                    elem->param = new MySQLResultDataEx(param->wnd, query, len);
                    elem->param->m_isselectquery = IsQuerySELECT(query);
                    elem->param->m_islimit = wyFalse;
                    elem->param->m_datares = NULL;
                    elem->param->m_totaltime = totaltime;
                    elem->param->m_exectime = exectime;
                    elem->param->m_isquerylimited = wyFalse;
                    elem->param->m_isExplain = wyTrue;
                }

				AddErrorOrMsg(ERROR_NONRESULT, *param->str, param->tunnel, param->mysql, ptrquery, param->executestatus, exectime, transfertime, totaltime, param->m_iseditor);
            }
			else if(myres && (param->tunnel->mysql_field_count(*param->mysql)) != 0)
			{
                elem->param = new MySQLResultDataEx(param->wnd, query, len);
                elem->param->m_datares = myres;
                elem->param->m_totaltime = totaltime;
                elem->param->m_exectime = exectime;
				elem->param->m_isselectquery = IsQuerySELECT(query);
                elem->param->m_islimit = (isselectrange == wyTrue && param->m_lowlimitvalue != -1 && param->m_highlimitvalue != -1) ? wyTrue : wyFalse;
                elem->param->m_isquerylimited = isselectrange;
                elem->param->m_ispartofmultiresult = ismultiresult;

                if(elem->param->m_islimit == wyTrue)
				{
                    elem->param->m_startrow = param->m_lowlimitvalue;
                    elem->param->m_limit = param->m_highlimitvalue;
				}
                	
				AddErrorOrMsg(ERROR_RESULT, *param->str, param->tunnel, param->mysql, ptrquery, param->executestatus, exectime, transfertime, totaltime, param->m_iseditor);
				
			} 
			else if(param->tunnel->mysql_error(*param->mysql)[0] != '\0')
			{
				if(param->executestatus != EXECUTE_SELECTED_EXPLAIN && param->executestatus != EXECUTE_SELECTED_EXTENDED 
					&& param->executestatus != EXECUTE_CURRENT_EXPLAIN && param->executestatus != EXECUTE_CURRENT_EXTENDED)
				{
					AddErrorOrMsg(ERROR_ERROR, *param->str, param->tunnel, param->mysql, ptrquery, param->executestatus, exectime, transfertime, totaltime, param->m_iseditor);		
				}
				else
				{
	/*				PostMessage(param->wnd->m_hwnd, UM_SHOWERROR, param->tunnel->mysql_errno(param->wnd->m_mysql), (LPARAM)param->query);
					param->tunnel->mysql_error*/
					ShowMySQLError(param->wnd->m_hwnd, param->wnd->m_tunnel, &param->wnd->m_mysql, querytemp.GetString());
				}
				ret = 1;	/* signifies false */
				
			}

			ChangeContextDB(param->tunnel, param->mysql, query, wyTrue);

			/* add the element to the linked list of result */
			param->list->Insert(elem);

		} while(param->tunnel->mysql_next_result(*param->mysql, FALSE, FALSE)== 0);

        if((*param->mysql)->net.last_errno != 0 && ret != 1)
        {
			AddErrorOrMsg(ERROR_ERROR, *param->str, param->tunnel, param->mysql, ptrquery, param->executestatus, exectime, transfertime, totaltime, param->m_iseditor);		
			ret = 1;	/* signifies false */
        }
	}

    if(param->m_iseditor == wyTrue)
    {
        //show execution time and total time on query execution
        param->wnd->m_pcquerystatus->AddTickCount(param->wnd->m_tunnel, exectime, totaltime);
        param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumofexectime += exectime;
        param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumoftotaltime += totaltime;
    }

	return ret;
}

/* removes all the QueryResultElem from the list */

void
FreeList(QueryResultList * queryresult, wyInt32 freeparam /*= false*/)
{
	// we write back the details and free all the buffer.
	QueryResultElem		*elem, *next;

	elem = (QueryResultElem*)queryresult->GetFirst();
	while(elem)
	{
		VERIFY(elem = (QueryResultElem*)queryresult->GetFirst());
		queryresult->Remove(elem);
		next =(QueryResultElem*)elem->m_next;

		if(freeparam)
			delete elem->param;

		delete elem;
		elem = next;
	}

	delete queryresult;
	queryresult = NULL;
}

/* main function that executes the queries that required to be executed */
void		
ExecuteQuery(QUERYTHREADPARAMS * param)
{	
	const wyChar        *token;
	wyChar              *selquery = NULL, *dump1 = NULL, *dump2;
	wyChar              delimiter[256] = {0};
	const wyChar        *query;
	wyInt32             isdel = 0;
	wyUInt32		    length = 0, numquery = 0;
	wyUInt32		    len = 0, linenum = 0;
	SQLTokenizer		*tok;
	wyString			tempdump;
    wyInt32             querycount = 0;// used to implement the single query execution even if tyhe cursor is outside the query
    wyUInt32            sumoftotaltime = 0, sumofexectime = 0;
    wyString            explainQuery;
	wyChar *			querystr;

	querystr = (wyChar *)param->query->GetString();

#ifndef COMMUNITY
	wyBool				isselect = wyFalse;
#endif
   
	param->wnd->m_isreconnected = wyFalse;
	param->wnd->m_ispqacheck = wyFalse;
	param->wnd->m_myresstatusfirst = NULL;
	param->wnd->m_myresstatussecond = NULL;
    param->wnd->m_isprofilerrequire = wyFalse;
	param->wnd->m_isselectquery = wyFalse;

	/* if we need to execute only a part of query if we need to execute selected query */
	if(param->executestatus == EXECUTE_SELECTED)
    {
		if(param->startpos == param->endpos)
			return;
		else 
        {
			selquery = (wyChar*)calloc(sizeof(wyChar), (param->endpos - param->startpos) + 5);
			memcpy(selquery, querystr + param->startpos, (param->endpos - param->startpos));
			query = selquery;
			length = param->endpos - param->startpos;
		}
	}
    else 
    {
		query = querystr;
		length = strlen(querystr);
	}

	tok = new SQLTokenizer(param->tunnel, param->mysql, SRC_BUFFER, (void*)query, length);

	length		= 0;
	numquery	= 0;

    if(param->m_iseditor == wyTrue)
    {
        sumofexectime = param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumofexectime;
        sumoftotaltime = param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumoftotaltime;
        param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumofexectime = 0;
        param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumoftotaltime = 0;
    }

	while(token = tok->GetQuery(&len, &linenum, &isdel, delimiter))
	{
		param->m_islimitpresent = wyTrue;
		param->m_lowlimitvalue = -1;
		param->m_highlimitvalue = -1;

		dump1 = _strdup(token);
		
		dump2 = dump1;

		if(!dump2 && dump1)
        {
			free(dump1);
			continue;
		}

		linenum = param->linenum;

		if(!isdel)
        {

            if(param->executestatus == EXECUTE_SELECTED_EXPLAIN || param->executestatus == EXECUTE_SELECTED_EXTENDED)
			{
                param->executestatus == EXECUTE_SELECTED_EXPLAIN ? 
                    explainQuery.Sprintf("EXPLAIN %s", dump2) : 
                    explainQuery.Sprintf("EXPLAIN EXTENDED %s", dump2) ;
                free(dump1);
                dump1 = _strdup(explainQuery.GetString());
                dump2 = dump1;

		        if(!dump2 && dump1)
                {
			        free(dump1);
			        continue;
		        }

				if(HelperExecuteQuery(param, dump2))
				{
					(*param->error)++;
					param->wnd->m_isselectquery = wyFalse;
				}
                break;
			} 

			/* now depending upon the execute status we execute it */
			else if(param->executestatus == EXECUTE_SELECTED || 
				 param->executestatus == EXECUTE_ALL)
			{
				//Check if LIMIT append with SELECT query or not
				if(param->m_iseditor == wyTrue && pGlobals->m_resuttabpageenabled == wyTrue)
					HandleLimitWithSelectQuery(dump2, param);
                
				if(HelperExecuteQuery(param, dump2))
				{
					(*param->error)++;
					param->wnd->m_isselectquery = wyFalse;
				}
				
#ifndef COMMUNITY
				//SQLyog profiler
				else if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
				   && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
				   && isselect == wyFalse && (param->wnd->m_isselectquery == wyTrue|| param->wnd->m_isprofilerrequire == wyTrue)
				   && param->tunnel && !param->tunnel->IsTunnel() && IsQueryStopped(param) == wyFalse
                   && param->m_iseditor == wyTrue && param->isadvedit == wyFalse)
				{
					HandleShowStausProfilerQuery(param, dump2);

					isselect = param->wnd->m_isselectquery;
				}
#endif
			} 
            else 
            {
				/* it means that it is to execute current query so we need to check whether the
				caret lies in this query */

                /*
                Here we are adding one more condition such that if the editor contains 
                only one query and the cursor is not in inside.
                */
                if((param->startpos < (wyInt32)(length+len+(numquery+1))) || 
                    (querycount == 0 && (param->startpos < (wyInt32)(length + len + numquery + 1 + GetQuerySpaceLen(len, query)))))
                {
					if(param->executestatus == EXECUTE_CURRENT_EXPLAIN || param->executestatus == EXECUTE_CURRENT_EXTENDED)
                    {
                        param->executestatus == EXECUTE_CURRENT_EXPLAIN ? explainQuery.Sprintf("EXPLAIN %s", dump2) : explainQuery.Sprintf("EXPLAIN EXTENDED %s", dump2);
                        free(dump1);
                        dump1 = _strdup(explainQuery.GetString());
                        dump2 = dump1;
					
		                if(!dump2 && dump1)
                        {
			                free(dump1);
			                continue;
		                }

                        if(HelperExecuteQuery(param, dump2))
					    {
						    (*param->error)++;
						    param->wnd->m_isselectquery = wyFalse;
					    }
                        break;
                    }
                    else
                    {
					//Check if LIMIT append with SELECT query or not
					if(pGlobals->m_resuttabpageenabled == wyTrue)
						HandleLimitWithSelectQuery(dump2, param);
					
					if(HelperExecuteQuery(param, dump2))
					{
						(*param->error)++;
						param->wnd->m_isselectquery = wyFalse;
					}
#ifndef COMMUNITY	
					else if(param->m_iseditor == wyTrue 
				   &&pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
				   && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
				   && pGlobals->m_ispqaenabled == wyTrue 
                        && isselect == wyFalse && (param->wnd->m_isselectquery == wyTrue || param->wnd->m_isprofilerrequire == wyTrue)
				   && param->tunnel && !param->tunnel->IsTunnel() 
                   && IsQueryStopped(param) == wyFalse && param->isadvedit == wyFalse)					
					{
						HandleShowStausProfilerQuery(param, dump2);
						isselect = param->wnd->m_isselectquery;
					}
#endif
                    break;
				} 
				}
                querycount++;
			}

			if(IsQueryStopped(param))
            {
                if(param->m_iseditor == wyTrue)
                {
                    param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumofexectime = sumofexectime;
                    param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_pcquerymessageedit->m_sumoftotaltime = sumoftotaltime;
                    param->wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->OnTabSelChange(NULL);
                }

				break;
            }
		}

		free(dump1);
		dump1 = NULL;

		param->m_lowlimitvalue = -1;
		param->m_highlimitvalue = -1;

		/* increase the internal length of the query */	
		length += ((len)+ strlen(delimiter));

		/* increase the query count */
		numquery++;
	}

	/* if its selected query to execute then free the stuff */
	if(param->executestatus == EXECUTE_SELECTED && selquery)
		free(selquery);
    
	if(dump1)
	{
		free(dump1);
	}

    delete tok;
	return;
}

VOID
HandleLimitWithSelectQuery(wyChar *query, QUERYTHREADPARAMS *param)
{
	wyInt32 numrows = 0;

	if(!query || !param)
		return;

	//Check if LIMIT append with SELECT query or not
	if(IstoAddLimitClausewithSelect(query) == wyTrue)
	{
		param->m_islimitpresent = wyFalse;
		
		if(param->m_lowlimitvalue == -1 || param->m_highlimitvalue == -1)
		{
			param->m_lowlimitvalue = 0;//pGlobals->m_lowlimitglobal;
			param->m_highlimitvalue = pGlobals->m_highlimitglobal;
			
			if(pGlobals->m_retainpagevalue == wyTrue)
			{
				numrows = HandleQueryPersistance(query);
				if(numrows > 0)
					param->m_highlimitvalue = numrows;

				else if(numrows == -1)
				{
					param->m_islimitpresent = wyFalse;

					param->m_lowlimitvalue = -1; 
					param->m_highlimitvalue = -1;
				}								
			}
		}					
	}

	else
	{
		param->m_islimitpresent = wyTrue;

		param->m_lowlimitvalue = -1; 
		param->m_highlimitvalue = -1;
	}
}


void
AddExplainQueryResults(QueryResultList * queryresult, wyString &str, TabMgmt * tab, wyInt32 err, wyBool isExtended, MDIWindow *wnd,  wyBool isfirstexecute)
{
    wyInt32         count = 0, warningcount = 0, i;
    QueryResultElem *elem;	
    wyString        temp;
    wyInt32         index = -1;
    CTCITEM item = {0};
#ifndef COMMUNITY
	wyInt32			ret = 1;
	wyBool			ispqa = wyFalse, istunnel = wyFalse;
#endif
	
    if(!err)
    {

        SendMessage(tab->m_hwnd, WM_SETREDRAW, FALSE, 0);
        if(tab->m_pqa)
        {
            count = CustomTab_GetItemCount(tab->m_hwnd);
            for(i = 0; i < count && index == -1; i++)
            {
                item.m_mask = CTBIF_IMAGE;
                CustomTab_GetItem(tab->m_hwnd, i, &item);
                if(item.m_iimage == IDI_QAALERT)
                {
                    index = i;
                }
            }
        }
        count = 0;
        if(index != -1)
        {
            tab->DeleteTab(index);
            tab->m_pqa = NULL;
        }
        else
        {
            index = 0;
        }
        elem = (QueryResultElem*)queryresult->GetFirst();	
    
	    while(elem)
	    {
		
    #ifndef COMMUNITY
		
		//Creates the Analyzer tab for 1st Result tab
        if(wnd && elem->param && ispqa == wyFalse)
		{
		    istunnel = (elem->param->m_pmdi->m_tunnel->IsTunnel()) ? wyTrue : wyFalse;
				
			//Anylyzer tab index will be just after the 1st 'select' result tab
			ret = tab->AddExplainQueryAnalyzer(elem->param, elem->param->m_isselectquery, istunnel, index, isExtended);
                
				
			if(ret == 1 || ret == -1)
			{
				if(ret == 1)
                {
					count++;
                }

				ispqa = wyTrue;
				}			
			}
    
    #endif

		    elem = (QueryResultElem*)elem->m_next;
	    }
    }

	if(str.GetLength() == 0)
    {
		str.SetAs(_("No query(s)were executed. Please enter a query in the SQL window or place the cursor inside a query."));
	}
    else
    {
        for(i = -1, warningcount = 0; (i = str.Find("\r\n<w>", i + 1)) != -1; warningcount++);
        temp.Sprintf(_("%d queries executed, %d success, %d errors, %d warnings\r\n\r\n"), queryresult->GetCount(), queryresult->GetCount() - err, err, warningcount);
        str.Insert(0, (wyChar*)temp.GetString());
    }
	
	tab->m_pcquerymessageedit->AddText(str);
	tab->ChangeTitles();
    tab->Resize();

    if(err)
    {
        tab->SelectFixedTab(IDI_QUERYMESSAGE);
    }
    else
    {
        tab->SelectTab(index);
    }

	return;

}


/* function adds date to the tab with information from the list */
void
AddQueryResults(QueryResultList * queryresult, wyString &str, TabMgmt * tab, wyInt32 err, MDIWindow *wnd,  wyBool isfirstexecute)
{
	wyInt32         count = 0, warningcount = 0, i;
    QueryResultElem *elem;	
    wyString        temp;
    wyBool          isTabResultAdded = wyFalse;
    wyInt32         indexResultTab = 0;

#ifndef COMMUNITY
	wyInt32			ret = 1;
	wyBool			ispqa = wyFalse, istunnel = wyFalse;
#endif
			
    SendMessage(tab->m_hwnd, WM_SETREDRAW, FALSE, 0);
    tab->DeleteAllItem(wyFalse);
	elem = (QueryResultElem*)queryresult->GetFirst();	
    
	while(elem)
	{
		/* if its a result then add a result param */
        if(elem->param && !elem->param->m_isExplain)
		{
            indexResultTab = isTabResultAdded? indexResultTab : count;
            isTabResultAdded = wyTrue;
			tab->AddRecordTab(count++, elem->param);
#ifdef COMMUNITY
			if(count == 1)
			{
				tab->AddAnalyzerTab(1);
				count ++;
			}
#endif
		}

#ifndef COMMUNITY
		
		//Creates the Analyzer tab for 1st Result tab
        if(tab->m_tabeditorptr->m_peditorbase->m_isadvedit == wyFalse &&
            pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
		{	
			if(wnd && pGlobals->m_ispqaenabled == wyTrue && elem->param && ispqa == wyFalse)
			{
				istunnel = (elem->param->m_pmdi->m_tunnel->IsTunnel()) ? wyTrue : wyFalse;
				
				//Anylyzer tab index will be just after the 1st 'select' result tab
				ret = tab->AddQueryAnalyzer(elem->param, elem->param->m_isselectquery, istunnel, count);
				
				if(ret == 1 || ret == -1)
				{
					if(ret == 1)
                    {
						count++;
                    }

					ispqa = wyTrue;
				}			
			}
		}
#endif

		elem = (QueryResultElem*)elem->m_next;
	}
	
	if(str.GetLength() == 0)
    {
		str.SetAs(NO_QUERY_EXECUTED);
	}
    else
    {
        str.Insert(0, "\r\n");
        for(i = -1, warningcount = 0; (i = str.Find("\r\n<w>", i + 1)) != -1; warningcount++);
        str.LTrim();
        temp.Sprintf(_("%d queries executed, %d success, %d errors, %d warnings\r\n\r\n"), queryresult->GetCount(), queryresult->GetCount() - err, err, warningcount);
        str.Insert(0, (wyChar*)temp.GetString());
    }
	
	tab->m_pcquerymessageedit->AddText(str);
	tab->ChangeTitles();
    tab->Resize();

    if((err | (wnd->m_isprofilerrequire && !wnd->m_isselectquery)) )
    {
        tab->SelectFixedTab(IDI_QUERYMESSAGE);
    }
    else
    {
        tab->SelectTab(indexResultTab);
    }

	return;
}

QueryThread::QueryThread()
{
	m_evt		 = INVALID_HANDLE_VALUE;
	m_thd		 = INVALID_HANDLE_VALUE;
	m_finished	 = wyFalse;
	m_param		 = NULL;
}

QueryThread::~QueryThread()
{
	if(m_thd != INVALID_HANDLE_VALUE)
		VERIFY(CloseHandle(m_thd));

	if(m_evt != INVALID_HANDLE_VALUE)
		VERIFY(CloseHandle(m_evt));

	if(m_param)
		delete m_param;
}

/* thread to execute the queries and add results */
/* basically we create a linked list of thread result and in one shot we add it up */
/* so basically it works like a transaction, either all queries work or all fail */

unsigned __stdcall 
QueryThread::ExecuteThread(LPVOID lpparam)
{
	QUERYTHREADPARAMS       *param =(QUERYTHREADPARAMS*)lpparam;
    QUERYFINISHPARAMS       *queryparams = NULL;

	if(param->m_iseditor)
	{    
		queryparams = new QUERYFINISHPARAMS;
		queryparams->list = param->list;
		queryparams->error = param->error;
		queryparams->mysql = param->mysql;
		queryparams->tmpmysql = param->tmpmysql;
		queryparams->str = param->str;
		queryparams->query = param->query;
		queryparams->executeoption = param->executeoption;
		queryparams->isedit = param->isedit;
	}

    if(param && param->tunnel && !param->tunnel->IsTunnel())
    {
        mysql_thread_init();
    }
    
    ExecuteQuery(param);			

	if(param->m_iseditor)
	{
		//Post Message to reset the Execute button
		PostMessage(param->wnd->m_hwnd, UM_QUERYCOMPLETED, (WPARAM)queryparams, NULL);
	}
	else
	{
	/* set the event and exit the thread */
	SetEvent(param->event);
	}
	
    if(param && param->tunnel && !param->tunnel->IsTunnel())
    {
        mysql_thread_end();
    }

    delete param;
	
    _endthreadex(1);
    
	return(1);
}

/*
	We create an event and start a thread that does all the processing */

HANDLE
QueryThread::Execute(QUERYTHREADPARAMS *param)
{
	wyUInt32 thdid;

	VERIFY(m_evt = CreateEvent(NULL, TRUE, FALSE, NULL));

	/* set the event in the param values */
	param->event = m_evt;

	//connect event
	if(param->wnd)
		param->wnd->m_isreconnected = wyFalse;

	/*	Create the thread and send it for execution. The thread will execute all
		the necessary query and add it to the tab control */
	VERIFY(m_thd =(HANDLE)_beginthreadex(NULL, 0, QueryThread::ExecuteThread, param, 0, &thdid));

	return m_evt;
}

void 
InitializeExecution(QUERYTHREADPARAMS *param)
{
    // Request ownership of the critical section.
    EnterCriticalSection(param->lpcs);

    // Access the shared resource.
    (*param->stop) = 0; // query execution is started

     // Release ownership of the critical section.
    LeaveCriticalSection(param->lpcs);

    return;
}

wyBool 
IsQueryStopped(QUERYTHREADPARAMS *param)
{
    wyBool ret = wyFalse;

    // Request ownership of the critical section.
    EnterCriticalSection(param->lpcs);

    // Access the shared resource.
    if(*param->stop)
        ret = wyTrue;

     // Release ownership of the critical section.
    LeaveCriticalSection(param->lpcs);

    return ret;   
}

//Add warnings to message tab
wyBool
GetWarning(Tunnel * tunnel, PMYSQL mysql, wyString * msg)
{
	MYSQL_ROW   row;
	MYSQL_RES	*res;
    wyInt32     levelval, codeval, messageval;
	wyString	query;

	if(IsShowWarnings() == wyFalse)
	{
		msg->AddSprintf("\r\n%s\r\n", WARNINGMSG);
		return wyTrue;
	}

	query.SetAs("SHOW WARNINGS");
	
	res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
	if(!res)
		return wyFalse;

	levelval	=	GetFieldIndex(tunnel, res, "level");
    codeval		=	GetFieldIndex(tunnel, res, "code");
	messageval	=	GetFieldIndex(tunnel, res, "message");
	
	while(row = tunnel->mysql_fetch_row(res))
	{
		if(row[levelval])
			msg->AddSprintf("\r\n%s Code :", row[levelval]);
		
        if(row[codeval])
			msg->AddSprintf(" %s", row[codeval]);

		if(row[messageval])
			msg->AddSprintf("\r\n%s\r\n", row[messageval]);
	}
	return wyTrue;
}

void FreeQueryExeFInishParams(QUERYFINISHPARAMS *queryparams, wyInt32 freeparam)
{
	if(queryparams->error)
	{
		delete queryparams->error;
		queryparams->error = NULL;
	}

	if(queryparams->query)
	{
		delete queryparams->query;
		queryparams->query = NULL;
	}

	if(queryparams->str)
	{
		delete queryparams->str;
		queryparams->str = NULL;
	}

	/* free the list and param if necessary..the secons parameter of FreeList is very important */
    FreeList(queryparams->list, freeparam);

	free(queryparams);
	return;
}