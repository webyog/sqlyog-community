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

#ifndef __QUERYTHREAD__
#define __QUERYTHREAD__

#include <windows.h>
#include <string>
#include "Global.h"
#include "TabMgmt.h"
#include "List.h"
#include "ResultView.h"

#define		EXECUTE_ALL					1
#define		EXECUTE_SELECTED			2
#define		EXECUTE_CURRENT				3	
#define     EXECUTE_CURRENT_EXPLAIN     4
#define     EXECUTE_SELECTED_EXPLAIN    5
#define     EXECUTE_SELECTED_EXTENDED   6
#define     EXECUTE_CURRENT_EXTENDED    7
#define		QUERYTHREAD					"QueryThread"
#define		ERROR_ERROR					1
#define		ERROR_RESULT				2
#define		ERROR_NONRESULT				3


class QueryResultList : public List
{
public:
	QueryResultList(){}
	~QueryResultList(){}
};



class QueryResultElem	: public wyElem
{
public:

    /// Result
	wyBool              result;

    /// Table record parameter
	MySQLResultDataEx* param;
};

/*! \struct QUERYTHREADPARAMS
	\brief parameters for the query thread

    \param wyInt32          startpos;               // Stating position
	\param wyInt32	        endpos;                 // Ending position
	\param wyInt32	        linenum;                // Line number
	\param wyInt32	        executestatus;          // Execution status
	\param wyInt32         *stop;                   // Flag to stop query execution 
	\param wyChar          *query;                  // Query
	\param wyInt32         *error;                  // Error number
	\param wyBool           isadvedit;              // Whether advance editor
	\param TabMgmt         *tab;	                // Query table

	\param QueryResultList *list;                   // Query result list
	
	\param Tunnel          *tunnel;                 // Tunnel pointer
	\param PMYSQL           mysql;                  // Pointer to mysql pointer

	\param wyString         str;                    // String
	\param HANDLE		    event;                  // Event
	
*/

struct QUERYTHREADPARAMS
{
	wyInt32     startpos;
	wyInt32	    endpos;
	wyInt32	    linenum;
	wyInt32	    executestatus;
	wyInt32     *stop;
	wyString    *query;
	wyInt32     *error;
	wyBool      isadvedit;
	TabMgmt     *tab;	

	wyInt32		m_lowlimitvalue;
	wyInt32		m_highlimitvalue;

	wyBool		m_islimitpresent;
	wyBool		m_iseditor;

	QueryResultList *list;
	
	Tunnel      *tunnel;
	PMYSQL      mysql;
	PMYSQL      tmpmysql;
	wyString    *str;
	HANDLE		event;

    MDIWindow   *wnd;

	wyBool		isprofile;

    LPCRITICAL_SECTION lpcs;
	EXECUTEOPTION executeoption;
	wyBool		isedit;
};

struct QUERYFINISHPARAMS
{
	QUERYFINISHPARAMS(){
		list	= NULL;
		error	= NULL;
		mysql	= NULL;
		str		= NULL;
		query	= NULL;
		tmpmysql= NULL;
		executeoption = SINGLE;
		isedit = wyFalse;
	}
    QueryResultList *list;
    wyInt32     *error;
    PMYSQL      mysql;
    wyString   * str;
    wyString    *query;
	PMYSQL      tmpmysql;
	EXECUTEOPTION executeoption;
	wyBool		isedit;
};

/// Returns the error or the success messages
/**
@param errstastus   : Error status
@param buffer       : Buffer
@param tunnel       : Tunnel pointer
@param mysql        : Pointer to mysql pointer
@param query		: Query
@param exectime     : Execution time
@param transfertime : Transfer time
@param totaltime    : Total time

*/
void        AddErrorOrMsg(wyInt32 errstastus, wyString &buffer, Tunnel *tunnel, PMYSQL mysql, const wyChar* query, 
						  wyInt32 executestatus, wyUInt32 exectime, wyUInt32 transfertime = 0, wyUInt32 tottime = 0, wyBool isembedformat = wyFalse);

/// Removes all the QueryResultElem from the list 
/**
@param list         : Query result list
@param freeparam    : Free parameters
*/
void        FreeList(QueryResultList *list, wyInt32 freeparam = 0);

/// Function adds date to the tab with information from the list
/**
@param list         : Query result list
@param str          : Result string
@param tab          : Query tab
@param err          : Error number
*/
void        AddQueryResults(QueryResultList *list, wyString& str, TabMgmt * tab, wyInt32 err, MDIWindow *wnd, wyBool isfirstexecute = wyTrue);


void        AddExplainQueryResults(QueryResultList * queryresult, wyString &str, TabMgmt * tab, wyInt32 err, wyBool isExtended, MDIWindow *wnd,  wyBool isfirstexecute = wyTrue);

/// It executes the queries that required to be executed 
/**
@param param        : Query thread parameters
*/
void        ExecuteQuery(QUERYTHREADPARAMS * param);

///Handle the SELECT query with LIMIT clause for Result tab paging
/**
@param query : IN Query to be handled
@param param : IN pointer to QUERYTHREADPARAMS
@return VOID
*/
VOID		HandleLimitWithSelectQuery(wyChar *query, QUERYTHREADPARAMS *param);

/// Executes query, initializes the correct structure and adds it to the query list
/**
@param param        : Query thread parameters
@param query        : Query
*/
wyInt32     HelperExecuteQuery(QUERYTHREADPARAMS * param, const wyChar *query);

// critcal section functions
wyBool      IsQueryStopped(QUERYTHREADPARAMS *param);
void        InitializeExecution(QUERYTHREADPARAMS *param);

///Add warnings to message tab
/**
@param tunnel    : Tunnel. 
@param mysql     : Mysql pointer.
@param msg       : In/Out message string
*/
wyBool		GetWarning(Tunnel * tunnel, PMYSQL mysql, wyString * msg);

/// Removes all the QueryResultElem from the list 
/**
@param list         : Query result list
@param freeparam    : Free parameters
*/
void        FreeQueryExeFInishParams(QUERYFINISHPARAMS *params, wyInt32 freeparam = 0);

//// getting the exact query length; 
//// this is used to implement the single 
//// query execution even if tyhe cursor is outside the query
//wyInt32 GetQuerySpaceLen(int orglen, const wyChar *query);

class QueryThread
{

private:

    /// Event HANDLEs
	HANDLE				 m_evt;

    /// Thread HANDLE
	HANDLE				 m_thd;

    /// Thread finished ?
	wyBool				 m_finished;

    /// Query thread parameters
	QUERYTHREADPARAMS	*m_param;

    /// Query tab
	TabMgmt			*m_tab;

public:

    /// Thread to execute the queries and add results 
    /**
    @param lpparam      : Query thread parameter
    */
	static	unsigned __stdcall	ExecuteThread(LPVOID lpparam);

	
    /// Main function to execute query 
    /**
    @param param      : Query thread parameter
    */
	HANDLE  Execute(QUERYTHREADPARAMS * param);

	QueryThread();
	~QueryThread();

};

#endif
