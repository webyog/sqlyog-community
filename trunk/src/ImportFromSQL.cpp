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
#include <winsock.h>
#include <mysql.h>
#include <time.h>
#include <io.h>
#include <assert.h>

#include "ImportFromSQL.h"
#include "Tunnel.h"
#include "SQLTokenizer.h"
#include "CommonHelper.h"
#include "MySQLVersionHelper.h"
#include "SQLMaker.h"
#include "Global.h"
#include "GUIHelper.h"
#include "ClientMySQLWrapper.h"

#define  FKCHKIMPORT_DEFAULT 1

extern	PGLOBALS		pGlobals;

ImportFromSQL::ImportFromSQL()
{
	m_errno = (wyInt32)0; m_port = (wyInt32)0;  

    m_numbytes = (wyInt32)0; m_numquery = (wyInt32)0;
		
	m_line_number = (wyInt32)0;	
    
    m_gui_routine = NULL;

	m_isbase64encode = wyFalse;

	m_abort_on_error = wyTrue;//by default abort on error is true. 

	m_errorfree = wyTrue;
	
	Initialize();
}

ImportFromSQL::~ImportFromSQL()
{
	if ( m_tunnel )
      delete m_tunnel;

    m_tunnel = NULL;
}

void	
ImportFromSQL::SetAbortFlag(wyBool abort_on_error)
{
	m_abort_on_error = abort_on_error;
}

void
ImportFromSQL::SetTunnel(wyBool val)
{
	m_mode_of_tunnel = val ;

	if ( !m_mode_of_tunnel )
		m_tunnel_host_site.SetAs("");

    m_tunnel = CreateTunnel(m_mode_of_tunnel);  
}

void
ImportFromSQL::SetErrorFile(wyString &errorfile)
{
	if(errorfile.GetLength())
		m_errfile.SetAs(errorfile);
}

void 
ImportFromSQL::SetImportFile(wyString &importfile)
{
	if(importfile.GetLength())
		m_file.SetAs(importfile);
}

void
ImportFromSQL::SetTunnelHostSite(wyString hostsite)
{
	m_tunnel_host_site.SetAs(hostsite.GetString());
}

void 
ImportFromSQL::SetHostInfo(wyString host, wyString user, 
						 wyString pass, wyString dbname, int port)
{
	if(host.GetLength())
		m_host.SetAs(host);
	
	if(user.GetLength()) 
		m_user.SetAs(user);

	if(pass.GetLength()) 
		m_pass.SetAs(pass);
	
	if(dbname.GetLength())
		m_db.SetAs(dbname);
	
	m_port	=	port;
}

void
ImportFromSQL::SetContentType(wyString contenttype)
{
	if(contenttype.GetLength())
		m_contenttype.SetAs(contenttype);
}

void
ImportFromSQL::SetImportEncodingScheme(bool isbase64encode)
{
	m_isbase64encode = (isbase64encode == true) ? wyTrue : wyFalse;
}


IMPORTERROR
ImportFromSQL::Import(wyUInt32 *num_bytes, wyUInt32 *num_queries, 
				   LPGUI_UPDATE_ROUTINE gui_routine, void * lpParam, wyBool * stopimport, MDIWindow *wnd)
{

	IMPORTERROR		err;

	if (!MySQLConnect())
		return MYSQLERROR;
	
    if (!OpenImportFile())
		return FILEERROR;

	if(!gui_routine)
		return GUIROUTINEERROR;

	m_gui_routine	= gui_routine;
	m_lp_param		= lpParam;

	err = StartImport(stopimport, wnd);
	
	*num_bytes		= m_numbytes;
	*num_queries	= m_numquery;
	
	SafeExit(); 

	return err;
}

void
ImportFromSQL::Initialize()
{
	m_errno				 =	0;
	m_error				 =	NULL;
	m_importfile		 =	NULL;
	m_tunnel			 =	NULL;
	m_lp_param			 =  NULL;
}

wyBool 
ImportFromSQL::SafeExit()
{
	m_importfile = NULL;

	if(m_mysql)
		m_tunnel->mysql_close(m_mysql);

	m_mysql = NULL;
		
	return wyTrue;
}

wyBool
ImportFromSQL::OnError(const wyChar * errmsg)
{
   wyString timeString;

	m_errno = m_tunnel->mysql_errno(m_mysql);
	m_error = m_tunnel->mysql_error(m_mysql);

	if(m_errfile.GetLength())
    {
	    /* append the query, error number and error code to the errorfile */
		FILE		*err;

		err = _wfopen(m_errfile.GetAsWideChar(), L"a");
		if(!err)
			return wyFalse;

		GetTimeString(timeString, ":");
		
		if(m_tunnel->IsTunnel())
			fprintf(err, _("Query:\n%s\n\nError occured at:%s\nError Code: %d - %s\r\n\r\n"), 
                                                    errmsg, timeString.GetString(), 
                                                    m_errno, m_error);
		else
			fprintf(err, _("Query:\n%s\n\nError occured at:%s\nLine no.:%d\nError Code: %d - %s\r\n\r\n"), 
                                                    errmsg, timeString.GetString(), 
                                                    m_line_number, m_errno, m_error);

		fclose(err);
	}

	return wyFalse;
}

wyBool
ImportFromSQL::MySQLConnect()
{
	m_mysql = m_tunnel->mysql_init((MYSQL*)NULL);
	assert(m_mysql);
	MDIWindow	*wnd = GetActiveWin();
	wyString	strtimeout;
    
	VERIFY(!(m_tunnel->mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8")));

	//Session wait_timeout(to avoid of server gone away)
	m_tunnel->mysql_options(m_mysql, MYSQL_INIT_COMMAND, "/*!40101 set @@session.wait_timeout=28800 */");

    /// Set up source SSL
    if(wnd->m_conninfo.m_issslchecked == wyTrue)
    {
        if(wnd->m_conninfo.m_issslauthchecked == wyTrue)
        {
            mysql_ssl_set(m_mysql, wnd->m_conninfo.m_clikey.GetString(), wnd->m_conninfo.m_clicert.GetString(), 
                            wnd->m_conninfo.m_cacert.GetString(), NULL, 
                            wnd->m_conninfo.m_cipher.GetLength() ? wnd->m_conninfo.m_cipher.GetString() : NULL);
        }
        else
        {
            mysql_ssl_set(m_mysql, NULL, NULL, wnd->m_conninfo.m_cacert.GetString(), NULL, 
                wnd->m_conninfo.m_cipher.GetLength() ? wnd->m_conninfo.m_cipher.GetString() : NULL);
        }
    }

	if(!m_tunnel->mysql_real_connect(m_mysql, m_host.GetString(), m_user.GetString(), m_pass.GetString(), NULL
                                         , m_port, NULL, CLIENT_MULTI_RESULTS, 
                                         m_tunnel_host_site.GetString()))
	{
	     return OnError(_("Unable to establish connection"));   
	}
	
	//we are setting Content-type for tunnel from the import structure 
	m_tunnel->SetContentType(m_contenttype.GetAsWideChar());
	if(m_tunnel->IsTunnel() == wyTrue)
	{
		m_tunnel->SetEncodingScheme(m_isbase64encode); //Set the encoding scheme Base64 or not
		m_tunnel->GetMySQLVersion(m_mysql);
	}
	
	//SetCharacterSet(m_tunnel, m_mysql, "utf8");

	if(m_db.GetLength() && UseDatabase(m_db, m_mysql, m_tunnel) == wyFalse)
	{
		 return OnError(_("Unable to issue `use` statement"));
	}
//	SetCharacterSet(m_tunnel, m_mysql, "latin1");

	if(m_tunnel->IsTunnel() && !m_tunnel->CheckCorrectTunnelVersion(m_mysql))
        return OnError(_("Unable to get Tunnel version")); 

	
	return wyTrue;
}

wyBool
ImportFromSQL::OpenImportFile()
{
  	if((m_importfile = _wfopen(m_file.GetAsWideChar(), L"r")) == NULL)
		return wyFalse;

	/* we just do the above to test file opening..we then close it */
	fclose(m_importfile);

	return wyTrue;
}

IMPORTERROR
ImportFromSQL::StartImport(wyBool * stopquery, MDIWindow *wnd)
{
	wyBool   istransaction = wyFalse, isfkchkhttp = wyFalse;
	wyInt32	 fkcheck = 0;
	wyWChar  *lpfileport=0;
	wyWChar  directory[MAX_PATH + 1] = {0};
	wyString dirstr, importdb;
 	/* set readbytes to zero */
	m_bytesread = 0;
	m_line_number = 0;	

	// if enable Transaction support (preference option)is selected and if it is not HTTP tunnel then execute 
	// start transaction by setting autocommit = 0
	if(IsStartTransactionEnable() == wyTrue  && m_tunnel->IsTunnel() == wyFalse)
	{
		StartTransaction(m_tunnel, m_mysql);	
		istransaction = wyTrue;
	}

	//As per preference, Set FK Check value for http during import
	if(m_tunnel->IsTunnel() == wyTrue)
	{
		if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
		{	
			dirstr.SetAs(directory);
			fkcheck = wyIni::IniGetInt(GENERALPREFA, "FKcheckImport", FKCHKIMPORT_DEFAULT, dirstr.GetString());		
		}
		else
			fkcheck = 1;

		isfkchkhttp = (fkcheck == 1) ? wyTrue : wyFalse;
	}

	SetGroupProcess(wnd, wyTrue);
	
	if(!DoImport(stopquery, isfkchkhttp))
    {
		if(*stopquery)
			return IMPORTSTOPPED;
		else
			return MYSQLERROR;
	} 

    else
	{
		GetSelectDB(importdb);
		
		if(importdb.GetLength() != 0)// import is unsuccess ful no need to rebuild
			SpecificDBRebuild((wyChar*)importdb.GetString());	

		//if enable Transaction support (preference option)is selected then execute commit query
		if(istransaction == wyTrue)
			EndTransaction(m_tunnel, m_mysql);

		//check for the result affected by the abort on error option.
		if(m_errorfree)
			return SUCCESS;
		else
			return SUCCESSWITHERROR;
	}
}

wyBool
ImportFromSQL::DoImport(wyBool * stopquery, wyBool isfkchkhttp)
{
	wyInt32				isdeli = 0;	
	wyString            query, importdb;			
	wyUInt32		    len = 0;
	wyChar				deli[12] = {0};
	wyBool				test;
	SQLTokenizer		*tok;
	
	
	m_bytesread = 0;
	m_numquery	= 0;

	tok = new SQLTokenizer(m_tunnel, &m_mysql, SRC_FILE, (void*)m_file.GetString(), m_file.GetLength());
	
	while(1)
	{
        wyChar *str;

        str = (wyChar*)tok->GetQuery(&len, (wyUInt32*)&m_line_number, &isdeli, deli);

		if(str == NULL)
            break;
		
		if(!*stopquery)
        {
			if(isdeli)
            {
				/* now we add delimiter to query if and only if its tunnel as then we have to
				   send the complete query in batch to the PHP */	
				if(m_tunnel->IsTunnel() == wyFalse)
					continue;
			}
			query.SetAs(str);
			test = ExecuteQuery(query, len, true, isfkchkhttp);

			if(!test && !m_abort_on_error)//checking if there is an error then setting the errorfree flag off.
				m_errorfree=wyFalse;
				
			if(!test && m_abort_on_error)
            {
				delete tok;
				return wyFalse;
			}

			if(isdeli)
            {
				m_tunnel->SetDelimiter(deli);
			}

			/* send the gui update routine issues */
			m_bytesread += len;
			m_gui_routine(m_lp_param, m_bytesread, m_numquery);
		}
        else if(isdeli && (*stopquery == wyFalse))
        {
            delete tok;
			m_tunnel->SetDelimiter(deli);
		}
		else if(*stopquery == wyTrue)
		{
			delete tok;
			return wyFalse;
		}		
	}

	/* if its tunnel then we need to execute the last part of query that might be left
	   because of batch processing */
	if(m_tunnel->IsTunnel() && m_tunnel->IsQueryLeft())
	{
		wyInt32			qret;
		MYSQL_RES		*res;

		qret = HandleMySQLRealQuery(m_tunnel, m_mysql, NULL, 0, wyTrue, wyTrue, wyTrue);

		res = m_tunnel->mysql_store_result(m_mysql, wyTrue);
		/* we specifically ignore empty queries */
		if(res == NULL && m_mysql->affected_rows == -1 && m_tunnel->mysql_errno(m_mysql) != ER_EMPTY_QUERY)
        {
			delete tok;
			return OnError(_("Unable to get resultset"));
        }
        		
		m_tunnel->mysql_free_result(res);
	}
	delete tok;
		
	return wyTrue;
}

void
ImportFromSQL::GetSelectDB(wyString& importdb)
{
	MYSQL_RES			*result;
	MDIWindow           *wnd = GetActiveWin();
	MYSQL_ROW           row;
	wyString            query;

	query.SetAs("select database()");
	result = ExecuteAndGetResult(wnd, m_tunnel, &m_mysql, query, wyFalse, wyTrue, wyTrue, true, true);
	
	if(!result)
		return;

	row = m_tunnel->mysql_fetch_row(result);	

    if((!row) || (!row[0]))
        return;

	importdb.SetAs(row[0]);
	m_tunnel->mysql_free_result(result);
}


wyBool
ImportFromSQL::ExecuteQuery(wyString& query, wyUInt32 str_length, bool isread, wyBool isfkchkhttp)
{
	MYSQL_RES			*result;
	
	/*isfkchkhttp = wyTrue: during http import dump*/

	m_numquery++;
	m_numbytes += query.GetLength();

	result = ExecuteAndGetResult(GetActiveWin(), m_tunnel, &m_mysql, query, wyFalse, wyTrue, wyTrue, isread, false, wyTrue, 0, isfkchkhttp); 
	
	if(result == NULL && m_mysql->affected_rows == -1 && m_tunnel->mysql_errno(m_mysql) != ER_EMPTY_QUERY)
			return OnError(query.GetString());

	m_tunnel->mysql_free_result(result);

	/* now if its tunnel then we need to set the db for each query otherwise tunnel mode will return error */
	if(m_tunnel->IsTunnel())
		ChangeContextDB(query);

	return wyTrue;
} 

wyBool
ImportFromSQL::ChangeContextDB(wyString& query)
{
	wyString padded(LeftPadText(query.GetString()));

	if(padded.GetLength() && (_strnicmp(padded.GetString(), "use", 3) == 0)) 
    {
        wyString db;
		
		GetDBFromQuery(query, db);

		m_tunnel->SetDB(db.GetString());
	}
	return wyTrue;
}

void
ImportFromSQL::GetDBFromQuery(wyString& query, wyString& db)
{
	wyChar  *token;
    wyChar  *querytoparse;
    wyUInt32 querylen;

    querylen = query.GetLength();

    querytoparse = (wyChar*)calloc(sizeof(char), querylen + 1);
	strncpy(querytoparse, query.GetString(), querylen);

	token = strtok(querytoparse, " ");
	token = strtok(NULL, " ");

	if(token)
    {
		wyUInt32    len;

		/* it might be that the database is sorrounded by ` so we have remove it */
		len = (wyUInt32)strlen(token);

		if(token[0] == '`')
        { 
			token[len - 1] = 0;
			token++;
		}
		
		db.SetAs(token);
	}
}

const wyChar * 
ImportFromSQL::LeftPadText(const wyChar * text)
{
	const wyChar * temp = text;

	while(temp)
	{
		if (isspace(*temp) == wyFalse)
			break;
		temp++;
	}

	return temp;
}

/* function checks whether there a unprintable character in between */
/* this function is required for tunneling as we need to send the data in base 64 encoding */

wyBool
ImportFromSQL::IsBadforXML(const wyChar * text)
{
	wyUInt32 i = 0;

	for(; text[i]; i++)
	{
		/* for range check out take from http://www.w3.org/TR/REC-xml/#sec-well-formed */
		if(!(*(text+i) >= 32 && *(text + i) < 55295))
			return wyTrue;
	}

	return wyFalse;
}

wyBool
ImportFromSQL::IsSpace(const wyChar * buff)
{
	wyUInt32 i = 0;

	for (; buff[i]; i++) 
    {
		if(!isspace(buff[i]))
			return wyFalse;
	}

	return wyTrue;
}
