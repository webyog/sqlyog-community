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

/*
	This is a cross platform class to export data to SQL file like MySQL Dump. It is used by SQLyog as well as SJA.
	Since SJA does not support tunneling, we wrapper functions for MySQL API starting with sja_ that ifdefs the 
	OS and uses tunnel or direct whichever required *
	
	YogExport is made cross platform starting from 4.07.
	
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef _WIN32

	#ifndef _CONSOLE
	#include "SQLMaker.h"
	#include "GUIHelper.h"
	#endif

	#ifdef _CONSOLE
	#include "Mail.h"
	#include "Main.h"
	#include "ScheduleExport.h"
	#endif

#else
	
	#include <sys/stat.h>
	#include <mysql/mysql.h>
	#include <string.h>         /* for memset as string.h defines memset */
	#include <stdlib.h>         /* for atoi which is included in stdlib.h in Linux */
	#include "Mail.h"
	#include "Main.h"
	#include "ScheduleExport.h"

#endif


#ifdef _WIN32
	#include <winsock.h>
	#include <mysql.h>
#endif

#include "ExportAsSQL.h"
#include "Tunnel.h"
#include "ClientMySQLWrapper.h"
#include "MySQLVersionHelper.h"
#include "CommonHelper.h"

#if !defined _WIN32 || defined _CONSOLE 
#include "wyZip.h"
#endif

#define DEF_DUMP_TUNNEL_TIMEOUT (100 * 1000)
#define SIZE_8K  (8*1024)

#define  CHECKSTOP()	   if(IsExportingStopped() == wyTrue) return wyFalse;

extern	 FILE*     logfilehandle;
extern	 FILE*     sessionerror;
extern	 wyWChar   logfilename[];


MySQLDump::MySQLDump()
{
    m_optdelayed = wyFalse;
    m_usedb = wyFalse;
    m_createdatabase = wyFalse;
    m_disablekeys = wyFalse;
    m_locktablewrite = wyFalse;
	m_alldb = wyFalse;
	m_expmysql = wyFalse;
	m_allobjects = wyFalse;
    m_modeoftunnel = wyFalse; 
    m_firstslave = wyFalse; 
    m_flushmaster = wyFalse; 
    m_flushlog = wyFalse;
    m_tablestructure = wyFalse; 
    m_completeinsert = wyFalse; 
    m_locktables = wyFalse;
    m_dataonly = wyFalse;
    m_bulkinsert = wyFalse;  
	m_dropobjects = wyFalse;
    m_setfkchecks = wyFalse; 
    m_autocommit = wyFalse; 
	m_includeversion = wyTrue;
    m_individualtablefiles = wyFalse;
	m_individualdbfiles = wyFalse;
	m_dumpalldbs = wyFalse;
    m_filemodeappend = wyFalse;
    m_createfunctions = wyFalse; 
    m_createprocedures = wyFalse; 
	m_createtriggers = wyFalse; 
    m_createviews = wyFalse;
    m_alltables = wyFalse;
	m_allviews = wyFalse;
	m_allprocedures = wyFalse;
	m_allfunctions = wyFalse;
	m_allEvents  = wyFalse;
	m_alltables = wyFalse;
    m_filemodetimestamp = wyFalse; 
    m_utf8charset = wyFalse;
    m_inquery = wyFalse; 
	m_completecount = wyTrue; 
    m_basicquery = wyTrue;
    m_fileerr.SetAs(_("Error opening ExportFile. Please check details of your schema file."));
	m_iserr = wyFalse;
	m_chunkinsert = wyFalse;
	m_singletransaction = wyFalse;
	m_rowperline=wyFalse;

    m_stopexport = 0; 
    m_mysql = NULL; 
    m_tunnel = NULL;
    m_lpparam = NULL; 
    m_escapeddata = NULL;

	m_insertcount = 0; 
    m_insertcountcomplete = 0; 
    m_querylength = 0;  
    m_chunklimit = 0;
	m_bulksize = 0; 
	m_escapeddatalen = 0; 
	m_rowcount = 0;
    m_endrow = 0;
	m_routine = 0;	
	m_startrow = 0;
	m_rowcount = 0;
	m_startpose = 0;
	m_iscommit = wyFalse;
	m_isremdefiner = wyFalse;

	m_abortonerr = wyTrue;
	m_compress = wyFalse;
	
    memset(&m_auth, 0, sizeof(m_auth));
	
	/* SSH init, Because the ssh session is already created no need to mention the localport, 
	   just connect to the given port(in case SSh session it will be localport)*/
	m_issshchecked = wyFalse;

	m_mysqlwaittimeout.SetAs(WAIT_TIMEOUT_SERVER);
	m_compressedprotocol = wyTrue;
 	/* initialize MySQLDump and set to default values */
	Initialize();
}

MySQLDump::~MySQLDump() 
{
	SafeExit();

}

 /*	This function Initializes all flag values and sets up default values*/
void
MySQLDump::Initialize()
{
	m_errno = 0;
	m_expfile =	NULL;
    m_error.Clear();
}

const wyChar * 
MySQLDump::IsNewMySQL()
{
	wyString    ver; 
	wyString    major, minor;
	wyChar	    seps[] = ".";
	wyInt32		majorver, minorver;

	ver.SetAs(sja_mysql_get_server_info(m_tunnel, m_mysql));

	major.SetAs(strtok((wyChar *)ver.GetString(), seps));
	minor.SetAs(strtok(NULL, seps));

	majorver = atoi((wyChar *)major.GetString());
	minorver = atoi((wyChar *)minor.GetString());

	if((majorver > 3) || (majorver == 3 && minorver > 22))
		return "`";
	else 
		return NULL;
}

void
MySQLDump::SetExpFile(const wyChar *outputfile)
{
	if(outputfile)
		m_file.SetAs(outputfile);

	if(m_dumpalldbs ==wyTrue)
		m_individualdbfiles = wyFalse;
	else
        m_individualtablefiles = wyFalse;
}


wyBool 
MySQLDump::SetExpFilePath(wyChar *filepath, wyBool timestampdir, wyBool timestampzip)
{
	wyInt32     retcode;
	wyString    buffer;
    wyString    timestring;

	buffer.SetAs(filepath);

    /* depending upon platform, the directory type changes */

#ifdef _WIN32
    if(buffer.GetCharAt(buffer.GetLength() - 1) == '\\')
		buffer.Strip(1);
#else
    if(buffer.GetCharAt(buffer.GetLength() - 1) == '/')
		buffer.Strip(1);
#endif


	if(m_compress == wyFalse)
	{
#ifdef _WIN32
		m_expfilepath.Sprintf("%s\\", buffer.GetString());
#else
		m_expfilepath.Sprintf("%s/", buffer.GetString());
#endif
	}

	if(timestampdir)
	{		
		if(m_compress == wyFalse)
		{
			GetTimeString(timestring, "-");
			m_expfilepath.Add(timestring.GetString());

#ifdef _WIN32
			m_expfilepath.Add("\\");
			retcode = CreateDirectory(m_expfilepath.GetAsWideChar(), NULL);
			if(!retcode)
				return wyFalse;
#else
			m_expfilepath.Add("/");
			retcode = mkdir(m_expfilepath.GetString(), 0);  //0 or NULL
			if(retcode == -1) // here 0 means SUCCESS
				return wyFalse;
#endif
		}		
	}

#if !defined _WIN32 || defined _CONSOLE 
	if(m_compress == wyTrue)
	{	
		m_expfilepath.Sprintf("%s", buffer.GetString());

		if(timestampzip == wyTrue)
			GetFormattedFilePath(m_expfilepath);

		if(m_zip.OpenZip(m_expfilepath.GetAsWideChar(), (m_filemodeappend == wyTrue) ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE) == wyFalse)
				return wyFalse;
	}
#endif

	if(m_dumpalldbs ==wyTrue)
		m_individualdbfiles = wyTrue;
	else
		m_individualtablefiles = wyTrue ;
    return wyTrue;
}

void
MySQLDump::SetTunnelMode(wyBool val)
{
	m_modeoftunnel = val;

	return;
}

void
MySQLDump::SetAppName(const wyChar *appname)
{
	if(appname)
		m_appname.SetAs(appname);

	return;
}
// By default Utf8 or if the user selects from the combo in scheduled Export charset would be set to that.
void
MySQLDump::SetCharSet(const wyChar *charset)
{
	if(charset)
		m_charset.SetAs(charset);

	return;
}

void 
MySQLDump::SetTunnelHostSite(const wyChar *hostsite)
{
	if(hostsite)
		m_tunnelhostsite.SetAs(hostsite);

	return;
}

void
MySQLDump::SetAppendFileMode(wyBool val)
{
	m_filemodeappend = val;	
	return;
}

void
MySQLDump::SetTimeStampMode(wyBool val)
{
	m_filemodetimestamp =	val;	
	return;
}

void
MySQLDump::SetAutoCommit(wyBool val)
{
	m_autocommit =	val;	
	return;
}

void
MySQLDump::SetVersionInfo(wyBool val)
{
	m_includeversion = val;
	return;
}

void
MySQLDump::SetOneRowPerLine(wyBool val)
{
	m_rowperline = val;
	return;
}

void
MySQLDump::SetCreateDatabase(wyBool val)
{
	m_createdatabase = val;
	return;
}

void
MySQLDump::SetCompleteInsert(wyBool val)
{
	m_completeinsert = val;
	return;
}

void
MySQLDump::SetUseDb(wyBool val)
{
	m_usedb = val;
	return;
}

void
MySQLDump::SetFKChecks(wyBool val)
{
	m_setfkchecks = val;
	return;
}


void
MySQLDump::SetHexBlob(wyBool val)
{
	m_sethexblob = val;
	return;
}

//void
//MySQLDump::SetFlushSlave(wyBool val)
//{
//	m_firstslave = val;
//	return;
//}

void
MySQLDump::SetLockTable(wyBool val)
{
	m_locktables = val;
	return;
}

void
MySQLDump::SetFlushMaster(wyBool val)
{
	/*Delete logs on master after backup. 
	This will automagically enable --first-slave.*/
	m_flushmaster = val; 
	return;
}

void
MySQLDump::SetFlushLogs(wyBool val)
{
	m_flushlog	= val;
	return;
}

void
MySQLDump::SetDisableKeys(wyBool val)
{
	m_disablekeys =	val;
	return;
}

void
MySQLDump::SetInsertWriteLocks(wyBool val)
{
	m_locktablewrite = val;
	return;
}

void
MySQLDump::SetBulkInsert(wyBool val)
{
	m_bulkinsert = val;
	return;
}

void
MySQLDump::SetChunkInsert(wyBool val)
{
	m_chunkinsert = val;
	return;
}

void
MySQLDump::SetChunkLimit(wyInt32 val)
{
	m_chunklimit = val;
	return;
}

void
MySQLDump::SetBulkSize(wyInt32 val)
{
	m_bulksize =  val;
	return;
}

void
MySQLDump::SetSingleTransaction(wyBool val)
{
	m_singletransaction = val;
	return;
}

void
MySQLDump::SetDumpCharSet(wyString &val)
{
    m_charset.SetAs(val);
	return;
}

void
MySQLDump::SetDropObjects(wyBool val)
{
	m_dropobjects = val;
	return;
}

void
MySQLDump::SetAllObjects(wyBool val)
{
	if(m_alltables == wyTrue && m_allviews == wyTrue && 
		m_allprocedures == wyTrue && m_allfunctions == wyTrue &&
		m_alltriggers == wyTrue && m_allEvents == wyTrue )
		val = wyTrue;
	m_allobjects = val;

	return;
}
void
MySQLDump::SetAllTables(wyBool val)
{
	m_alltables	= val;
	return;
}
void
MySQLDump::SetAllViews(wyBool val)
{
	m_allviews	= val;
	return;
}
void
MySQLDump::SetAllProcedures(wyBool val)
{
	m_allprocedures	= val;
	return;
}
void
MySQLDump::SetAllFunctions(wyBool val)
{
	m_allfunctions	= val;
	return;
}

void
MySQLDump::SetDumpAllDbs(wyBool val)
{
	m_dumpalldbs =	val;
	return;
}

void			
MySQLDump::SetToCompress(wyBool val)
{
	m_compress = val;
	return;
}

void
MySQLDump::SetDumpMysql(wyBool val)
{
	m_expmysql  =	val;
	return;
}
void
MySQLDump::SetAllTriggers(wyBool val)
{
	m_alltriggers =	val;
	return;
}
void
MySQLDump::SetAllEvents(wyBool val)
{
	m_allEvents =	val;
	return;
}

void
MySQLDump::SetCreateViews(wyBool val)
{
	m_createviews	=	val;
	return;
}

void
MySQLDump::SetCreateProcedures(wyBool val)
{
	m_createprocedures	= val;
	return;
}

void
MySQLDump::SetCreateFunctions(wyBool val)
{
	m_createfunctions	= val;
	return;
}

void
MySQLDump::SetCreateTriggers(wyBool val)
{
	m_createtriggers =	val;
	return;
}

void
MySQLDump::SelectedTables(SelectedObjects *names)
{
	if(names)
		m_seltables.Insert(names);
}
void
MySQLDump::SelectedProcedures(SelectedObjects *names)
{
	if(names)
		m_selprocedures.Insert(names);
}
void
MySQLDump::SelectedFunctions(SelectedObjects *names)
{
	if(names)
		m_selfunctions.Insert(names);
}
void
MySQLDump::SelectViews(SelectedObjects *names)
{
	if(names)
		m_selviews.Insert(names);
}
void
MySQLDump::SelectedTriggers(SelectedObjects *names)
{
	if(names)
		m_seltriggers.Insert(names);
}
void
MySQLDump::SelectedEvents(SelectedObjects *names)
{
	if(names)
		m_selevents.Insert(names);
}

void
MySQLDump::SetDumpStructureOnly(wyBool val)
{
	m_tablestructure =	val;
	return;
}

void
MySQLDump::SetDumpDataOnly(wyBool val)
{
	m_dataonly = val;
	return;
}



void
MySQLDump::SetInsertDelayed(wyBool val)
{
    m_optdelayed =	val;
	return;
}

void
MySQLDump::SetDefiner(wyBool flag)
{
	m_isremdefiner = flag;
	return;
}

#ifdef _WIN32
void     
MySQLDump::SetEntInstance(HINSTANCE hinst)
{
    m_entinst = hinst;
	return;
}

#endif

void	
MySQLDump::SetAdvExpValues(SCHDEXTRAOPTIONS *opt)
{
	m_createviews       = opt->m_createview;
	m_createfunctions   = opt->m_createfunction;
	m_createprocedures  = opt->m_createprocedure;
	m_createtriggers    = opt->m_createtrigger;

	return;
}


/*This function sets database name, and its called by the constructor initially*/
void
MySQLDump::SetDatabase(const wyChar *db)
{
	if(db)
		m_db.SetAs(db);

	return;
}

void
MySQLDump::SetHostInfo(const wyChar *hostname, const wyChar *uname, 
					   const wyChar *password, wyInt32 port, TUNNELAUTH *auth)
{
	if(hostname)
		m_hostname.SetAs(hostname);

	if(uname)
		m_uname.SetAs(uname);
   
	if(password)
		m_password.SetAs(password);
	
	if(auth)
		memcpy(&m_auth, auth, sizeof(m_auth));
    
	m_port	=	port;
		
	return;
}

void
MySQLDump::SetHostInfo(ConnectionInfo *conn, TUNNELAUTH *auth)
{
	if(conn->m_host.GetLength())
		m_hostname.SetAs(conn->m_host);

	if(conn->m_user.GetLength())
		m_uname.SetAs(conn->m_user);
   
	if(conn->m_pwd.GetLength())
		m_password.SetAs(conn->m_pwd);

	if(auth)
		memcpy(&m_auth, auth, sizeof(m_auth));
    
	// the connection structure is initialized with localport if it is ssh
	m_port	=	conn->m_port; 
	return;
}

void	
MySQLDump::SetAbortOnErr(wyBool flag)
{
	m_abortonerr = flag;
}




/* This function Opens Individual Text File for each table */
wyBool
MySQLDump::OpenIndividualExpFileForEachTable(const wyChar *table)
{
	wyString    filenameBuff;
    wyString    timeString;  

	if(!table)
	{
		return wyFalse;
	}

	GetTimeString(timeString, "-");	
	
	if(m_compress == wyFalse)
	{

		if(m_filemodetimestamp == wyTrue)
			filenameBuff.Sprintf("%s%s_obj_%s.sql", m_expfilepath.GetString(), timeString.GetString(), table);

		else
			filenameBuff.Sprintf("%sobj_%s.sql", m_expfilepath.GetString(), table);

#ifdef _WIN32
	// opens individual text file , and each opend file will be closed in function Footer()
	
		if(m_expfile)
		{
			fclose(m_expfile);
			m_expfile = NULL;
		}
		m_expfile = _wfopen(filenameBuff.GetAsWideChar(), m_filemodeappend ? TEXT(APPEND_MODE) : TEXT(WRITE_MODE)) ;
#else
		m_expfile = fopen(filenameBuff.GetString(), m_filemodeappend ? APPEND_MODE : WRITE_MODE) ;
#endif

		if(!m_expfile) 
			return wyFalse;
	}

	else
	{		
		filenameBuff.Sprintf("obj_%s.sql",table);

#if !defined _WIN32 || defined _CONSOLE 
		if(m_zip.OpenFileInZip(filenameBuff.GetAsWideChar(), 0)!= ZIP_OK)
		{
			return wyFalse;
		}
#endif
		
	}

	return wyTrue;	
}

 /* This function connects to the host */ 
wyBool
MySQLDump::ConnectToDataBase()
{
    m_tunnel = CreateTunnel(m_modeoftunnel);  
    
#ifdef _WIN32	
	/* tunnel auth only available in Windows */
     ConnectToDataBaseWin32Def();
#endif
     if(ConnectToDataBaseNext() == wyFalse)
         return wyFalse;
	
#ifdef _WIN32
	/* not required in Linux as direct connection, server version is already available */
	if(!m_tunnel->GetMySQLVersion(m_mysql))
    {
		OnError();

        m_tunnel->mysql_close(m_mysql);

        m_mysql = NULL;

        return wyFalse;
    }
#endif
  
	return wyTrue;
}

wyBool
MySQLDump::ConnectToDataBaseWin32Def()
{
#ifdef _WIN32
    /* set the proxy info */
	if(m_auth.isproxy) 
    {
		m_tunnel->SetProxy(wyTrue);
		m_tunnel->SetProxyInfo(m_auth.proxy, m_auth.proxyport, m_auth.proxyusername, m_auth.proxypwd);
	} else 
		m_tunnel->SetProxy(wyFalse);
	
	if(m_auth.ischallenge) 
    	m_tunnel->SetChallengeInfo(wyTrue, m_auth.chalusername, m_auth.chalpwd);	
	else 
		m_tunnel->SetChallengeInfo(wyFalse);
	
	/* We set the timeout to a big value.
	   It will not work for more then 100 minutes anyway */
	m_tunnel->SetTimeOut(DEF_DUMP_TUNNEL_TIMEOUT);		

#endif

    return wyTrue;  
}

wyBool
MySQLDump::ConnectToDataBaseNext()
{
    ConnectionInfo  conn;
    wyString        error;

	InitConnectionDetails(&conn);

    m_mysql = sja_mysql_init(m_tunnel, (MYSQL *)NULL);
	assert(m_mysql);
	
    /// mysql sertver details
    conn.m_host.SetAs(m_hostname);
    conn.m_user.SetAs(m_uname);
    conn.m_pwd.SetAs(m_password); 
    conn.m_charset.SetAs(m_charset);
    conn.m_port = m_port;
	conn.m_iscompress = m_compressedprotocol;
	conn.m_strwaittimeout.SetAs(m_mysqlwaittimeout);
	
	/// Tunnel info
	conn.m_url.SetAs(m_tunnelhostsite);
	conn.m_ishttp = m_modeoftunnel;
	conn.m_isssh = m_issshchecked;

#if defined _WIN32 //|| defined _CONSOLE 
	// if http tunnel set time out value 
	if(m_tunnel && m_modeoftunnel)
	{	
		conn.m_timeout = DEF_DUMP_TUNNEL_TIMEOUT;	
		m_tunnel->SetContentType(m_auth.content_type); //Content-Type(HTTP header)
		m_tunnel->SetEncodingScheme(m_auth.isbase64encode);
	}

#endif

    /// SSL details
    conn.m_issslchecked = m_issslchecked;
    conn.m_issslauthchecked = m_issslauthchecked;

    conn.m_cacert.SetAs(m_cacert);
    conn.m_clicert.SetAs(m_clicert);
    conn.m_clikey.SetAs(m_clikey);
    conn.m_cipher.SetAs(m_cipher);

	if(!m_tunnel->IsTunnel())
		SetMySQLOptions(&conn, m_tunnel, &m_mysql, wyFalse);

//Content-Type is included in m_auth.
#if !defined _WIN32 || defined _CONSOLE 

    if(m_tunnel = ConnectToMySQL(&conn, &m_mysql, m_tunnel, &m_auth, error))
        return wyTrue;
    else
    {
        return wyFalse;
    }
#else
    if(pGlobals->m_pcmainwin->m_connection->ConnectToMySQL(&conn, m_tunnel, m_mysql, error))
        return wyTrue;
    else
    {
		OnError();
                return wyFalse;
    }

#endif

}

void
MySQLDump::SetSSLInfo(ConnectionInfo *conn)
{
    m_issslchecked      = conn->m_issslchecked;
    m_issslauthchecked  = conn->m_issslauthchecked;

    m_clikey.SetAs(conn->m_clikey);
    m_clicert.SetAs(conn->m_clicert);
    m_cacert.SetAs(conn->m_cacert);
    m_cipher.SetAs(conn->m_cipher);
}

//Sets the compress protocol and wait_timeout
void
MySQLDump::SetOtherConnectionInfo(ConnectionInfo *conn)
{
	if(conn->m_strwaittimeout.GetAsInt32())
	{
		m_mysqlwaittimeout.SetAs(conn->m_strwaittimeout);
	}
	else
	{
		m_mysqlwaittimeout.SetAs(WAIT_TIMEOUT_SERVER);
	}

	m_compressedprotocol = conn->m_iscompress;	
}

/* This function allow user to select tables to be dumped*/
wyBool
MySQLDump::DumpSelectedTables(wyString * buffer, wyString &db)
{
	SelectedObjects  *table;
	wyInt32		err = 0;
	
	table = (SelectedObjects*)m_seltables.GetFirst();

	//setting the progress bar range to total no of selected tables
	m_routine(m_lpparam, m_db.GetString(), m_seltables.GetCount(), SETOBJECTCOUNT);

	for(; table; table = (SelectedObjects*)table->m_next)
	{
		if(!DumpTable(buffer, db.GetString(), table->m_selobject.GetString(), &err)) 
       {
		   if(err)
           {
			   /* set special error */
			   m_error.SetAs(m_fileerr.GetString());
			   
		   }
		   else if(m_error.GetLength() == 0)
			   OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle); //printing error
		   AppendErrMessage(m_error.GetString(), m_error.GetString()); //adding error to mail message
		   m_iserr = wyTrue; //if an error occured m_iserr= wyTrue 
		   
#endif
		   /* We stops exporting when an error occured.
		   If m_abortonerr is true or if user prees top button or any file error . 
		   These three cases we are stopping the process */
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err) 
		   	   return wyFalse;
		   
	   }

	   //If size of buffer is more than 8K, write to file
	   if(WriteBufferToFile(buffer) == wyFalse)
		   return wyFalse;

	}
	return wyTrue;
}	

wyBool
MySQLDump::DumpViews(wyString * buffer, const wyChar *db, wyBool isviewstructure)
{

	MYSQL_RES	     *res = NULL;
	MYSQL_ROW	     row = NULL;
	wyString         query, view;
	wyInt32          err = 0;
		
	if(IsMySQL5010(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	m_routine(m_lpparam, "views", 0, FETCHDATA);

	GetSelectViewStmt(db, query);
    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
	if(!res)
		return OnError();

	//setting the progress bar range to no of views
	m_routine(m_lpparam, m_db.GetString(), res->row_count , SETOBJECTCOUNT);

	if(m_individualtablefiles == wyFalse && (isviewstructure == wyTrue))
	{
		row = sja_mysql_fetch_row(m_tunnel, res);
		while(row)
		{
			view.SetAs(row[0]);
			
			if(DumpTableStructureOnView(buffer, view.GetString(), wyTrue) == wyFalse)
			{
#if !defined _WIN32 || defined _CONSOLE 		  
				PRINTERROR(m_error.GetString(), logfilehandle);
				AppendErrMessage(m_error.GetString(), m_error.GetString());
				m_iserr = wyTrue; //if any error occur,set m_iserr = wyTrue
#endif
				//we stops exporting when an error occured.
				if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue) 
				{
					sja_mysql_free_result(m_tunnel, res);
					return wyFalse;
				}
			}
			row = sja_mysql_fetch_row(m_tunnel, res);
		}
		sja_mysql_data_seek(m_tunnel, res, 0);
	}
	row = sja_mysql_fetch_row(m_tunnel, res);
	while(row)
	{
		view.SetAs(row[0]);
		if(DumpView(buffer, db, view.GetString(), &err) == wyFalse)
		{
			if(err) 
			{
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
			} 
			else if(m_error.GetLength() == 0)
					OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle); 
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue; //if any error occur,set m_iserr = wyTrue
#endif
		   /* if m_abortonerr is true or if user prees top button or any file error . 
		   these three cases we r stopping the process */
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err) 
		   {	
				sja_mysql_free_result(m_tunnel, res);
				return wyFalse;
		   }
		}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;

		row = sja_mysql_fetch_row(m_tunnel, res);
	}

    sja_mysql_free_result(m_tunnel, res);

	return wyTrue;
}

wyInt32
MySQLDump::GetmySQLCaseVar()
{
	wyString	query, dbname, casevalue;
	wyInt32		fldindex, value;
	wyBool		ismysql41 = wyFalse; 
	MYSQL_RES*	res;
	MYSQL_ROW	row;


	ismysql41 = IsMySQL41(m_tunnel, &m_mysql);  

	query.Sprintf("show variables like 'lower_case_table_names'");
	res =SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 
	//res = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!res && sja_mysql_affected_rows(m_tunnel,m_mysql) == -1)
	{
		return 0;
	}

	fldindex = GetFieldIndex(m_tunnel, res, "Value"); 
    row =  sja_mysql_fetch_row(m_tunnel, res);
	if(!row)
	{
		sja_mysql_free_result(m_tunnel, res);
		return 0;
	}
	
	casevalue.SetAs(row[fldindex], ismysql41);
	sja_mysql_free_result(m_tunnel, res);

	value = casevalue.GetAsInt32();

	return value; 
}


wyBool
MySQLDump::IsLowercaseFS()
{
	wyString	query, dbname, casevalue;
	wyInt32		fldindex;
	wyBool		ismysql41 = wyFalse; 
	MYSQL_RES*	res;
	MYSQL_ROW	row;
	wyBool		islowercasefs;


	ismysql41 = IsMySQL41(m_tunnel, &m_mysql);  

	query.Sprintf("show variables like 'lower_case_file_system'");
	res =SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 
	//res = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
	{
		return wyTrue;
	}

	fldindex = GetFieldIndex(m_tunnel, res, "Value"); 
    row =  sja_mysql_fetch_row(m_tunnel, res);
	if(!row)
	{
		sja_mysql_free_result(m_tunnel, res);
		return wyTrue;
	}
	
	casevalue.SetAs(row[fldindex], ismysql41);
	if(casevalue.CompareI("ON") == 0)
		islowercasefs = wyTrue;
	else
		islowercasefs = wyFalse;

	sja_mysql_free_result(m_tunnel, res);
	return islowercasefs; 
}


wyBool
MySQLDump::DumpProcedures(wyString * buffer, const wyChar *db)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query;
    wyInt32     err = 0;
	wyBool		iscollate = wyFalse;
	if(IsMySQL5010(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	m_routine(m_lpparam, "stored procedures", 0, FETCHDATA);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVar() == 0)
		if(!IsLowercaseFS())
			iscollate = wyTrue;
	GetSelectProcedureStmt(db, query, iscollate);

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
	if(!res)
		return OnError();

	//setting the progress bar range to total no of procedures
	m_routine(m_lpparam, m_db.GetString(), res->row_count , SETOBJECTCOUNT);

    row = sja_mysql_fetch_row(m_tunnel, res);
	while(row)
	{
		if(!DumpProcedure(buffer, db, row[0], &err))
		{
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
				
			}
	        else if(m_error.GetLength() == 0)
					OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle); 
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue; //if any error occur,set m_iserr = wyTrue
#endif
		   /* Stop exporting if an error occured & abortonerror is true. 
			  If m_abortonerr is true or if user prees top button or any file error .
		      These three cases we r stopping the process */
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err)
		   {
			   sja_mysql_free_result(m_tunnel, res);
			   return wyFalse;
		   }
       	}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
		 
		 row = sja_mysql_fetch_row(m_tunnel, res);
	}
	
	sja_mysql_free_result(m_tunnel, res);

	return wyTrue;
}	

wyBool
MySQLDump::DumpFunctions(wyString * buffer, const wyChar *db)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query;
    wyInt32     err = 0;
	wyBool		iscollate = wyFalse;
	if(IsMySQL5010(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	m_routine(m_lpparam, "functions", 0, FETCHDATA);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVar() == 0)
		if(!IsLowercaseFS())
			iscollate = wyTrue;
	GetSelectFunctionStmt(db, query, iscollate);

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 

	if(!res)
		return OnError();

	//setting the progress bar range to total no of functions
	m_routine(m_lpparam, m_db.GetString(), res->row_count, SETOBJECTCOUNT);

	row = sja_mysql_fetch_row(m_tunnel, res);

	while(row)
	{
		if(!DumpFunction(buffer, db, row[0], &err))
		{
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
				
			} 
            else if(m_error.GetLength() == 0)
                 OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle); 
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue; //if any error occur,set m_iserr = wyTrue
#endif
		   /* if m_abortonerr is true or if user prees top button or any file error . 
		      These three cases we are stopping the process */
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue ||err) 
		   {
			   sja_mysql_free_result(m_tunnel, res);
			   return wyFalse;
		   }
       	}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;

		row = sja_mysql_fetch_row(m_tunnel, res);
	}		

	sja_mysql_free_result(m_tunnel, res);

	return wyTrue;
}
// dump all events
wyBool
MySQLDump::DumpEvents(wyString * buffer, const wyChar *db)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query;
    wyInt32     err = 0;
	wyBool      seteventschdule;
	wyBool		iscollate = wyFalse;
	if(IsMySQL516(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	m_routine(m_lpparam, "events", 0, FETCHDATA);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVar() == 0)
		if(!IsLowercaseFS())
			iscollate = wyTrue;	
	GetSelectEventStmt(db, query, iscollate);

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 

	if(!res)
		return OnError();

	//setting the progress bar range to total no of events
	m_routine(m_lpparam, m_db.GetString(), res->row_count , SETOBJECTCOUNT);

	row = sja_mysql_fetch_row(m_tunnel, res);

	seteventschdule = wyTrue;

	
	while(row)
	{
		if(!DumpEvent(buffer, db, row[0],seteventschdule, &err))
		{
					
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());				
				
			} 
			else if(m_error.GetLength() == 0)
                 OnError();

#if !defined _WIN32 || defined _CONSOLE 	 
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue; //if an error occured set m_iserr= wyTrue

#endif
		   /* if m_abortonerr is true or if user prees top button or any file error . 
		      These three cases we r stopping the process. */
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err) 
		   {
			   sja_mysql_free_result(m_tunnel, res);
			   return wyFalse;
		   }
		}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
		 
		 row = sja_mysql_fetch_row(m_tunnel, res);
	}		

	sja_mysql_free_result(m_tunnel, res);

	return wyTrue;
}
wyBool
MySQLDump::DumpTrigger(wyString * buffer, const wyChar * db, const wyChar * trigger, wyInt32 * fileerr)
{
	MYSQL_RES	      *res = NULL,*res1=NULL;
	MYSQL_ROW 	      row = NULL,row1=NULL;
	wyString	      query, def, definer, deftemp,query1,bodyoftrigger;
	wyChar			  *ch = NULL;
	wyInt32			  pos;
	wyBool			  ismysql5017;
	wyBool      isshowcreateworked=wyTrue;

	ismysql5017 = IsMySQL5017(m_tunnel, &m_mysql);
    		
	if(m_individualtablefiles == wyTrue)
	{
		if(OnIndividualFiles(buffer, trigger, fileerr) == wyFalse)
			return wyFalse;
	}


	m_routine(m_lpparam, trigger, 0, TRIGGERSTART);
	
	if(m_flushlog == wyTrue) 
	{
		if(OnFlushLog() == wyFalse)
			return wyFalse;
	}	
	//There are mutiple issues with show triggers and show create trigger in MySQL servers--
	//with show create trigger there are following issues-
	//1-SHOW CREATE TRIGGER was added in MySQL 5.1.21 so it will not work for older versions
	//2-This bug reprot-http://bugs.mysql.com/bug.php?id=58996
	//With show triggers there are following issues--
	//1-Curruption of quotes refer-http://forums.webyog.com/index.php?showtopic=7625&hl= and http://bugs.mysql.com/bug.php?id=75685.
	//So now here is the logic for getting things correct up to maximum extent--
	//fire both queries and
	//First try to use show create trigger if query works then get the body of trigger from the result
	//if show create trigger fails use show triggers--because user can have trigger without quotes
	//Below is the implementation of this logic.
	query.Sprintf("show triggers from `%s` where `trigger` = '%s'", m_db.GetString(), trigger);
	query1.Sprintf("show create trigger `%s`. `%s`", m_db.GetString(), trigger);

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 

	if(!res)
		return OnError();

	CHECKSTOP()
		
	sja_mysql_free_result(m_tunnel,res);

    res = ExecuteQuery(query);
	res1= ExecuteQuery(query1);

	if(!res)
		return OnError();
	if(!res1)
		isshowcreateworked=wyFalse;
	row	= sja_mysql_fetch_row(m_tunnel, res);
	//bug http://bugs.mysql.com/bug.php?id=75685. We have to fire show create trigger to get the body of trigger
	if(isshowcreateworked)
	row1=sja_mysql_fetch_row(m_tunnel, res1);
	if(!row)
		return OnError();
	//body of trigger
	if(isshowcreateworked && row1[2])
	{
		bodyoftrigger.SetAs(row1[2]);
	if(GetBodyOfTrigger(&bodyoftrigger)==-1)
	{
	sja_mysql_free_result(m_tunnel,res);
	sja_mysql_free_result(m_tunnel,res1);
	
	}
	}

	//Definer
	if(!m_isremdefiner)
		if(ismysql5017 == wyTrue)
		{
			if(!row[7])
				return OnError();

			def.SetAs(row[7]);	

			if(def.GetLength())
			{
				ch = strrchr((wyChar*)def.GetString(), '@');
			
				pos = ch - def.GetString();
			
				if(pos > 0)
				{	
					def.Substr(0, pos);

					ch++;

					deftemp.Sprintf("'%s'@'%s'", def.Substr(0, pos), ch);

					//Keeps the definer
					definer.Sprintf("/*!50017 DEFINER = %s */", deftemp.GetString()); 
				}
			}
		}
		
    buffer->AddSprintf("%s/* Trigger structure for table `%s` */%s", 
        m_strnewline.GetString(), row[2], m_strnewline.GetString());
	
    buffer->AddSprintf("%sDELIMITER $$%s", m_strnewline.GetString(), m_strnewline.GetString());

	// drop trigger if exist in the importing db
	if(m_dropobjects == wyTrue)
	{
        buffer->AddSprintf("%s/*!50003 DROP TRIGGER*/", m_strnewline.GetString());
		
		buffer->AddSprintf("/*!50032 IF EXISTS */ /*!50003 `%s` */$$%s",  trigger, m_strnewline.GetString());
	}
	
	if(m_isremdefiner)
	{
		if(isshowcreateworked && bodyoftrigger.GetLength()!=0)
		buffer->AddSprintf("%s/*!50003 CREATE */ /*!50003 TRIGGER `%s` %s %s ON `%s` FOR EACH ROW %s */$$%s%s", 
						m_strnewline.GetString(),
						row[0], /* Trigger */
						row[4], /* Timing */
						row[1], /* Event */
						row[2], /* table */
						bodyoftrigger.GetString(), /*  Statement */
                        m_strnewline.GetString(),
                        m_strnewline.GetString());
		else
			buffer->AddSprintf("%s/*!50003 CREATE */ /*!50003 TRIGGER `%s` %s %s ON `%s` FOR EACH ROW %s */$$%s%s", 
						m_strnewline.GetString(),
						row[0], /* Trigger */
						row[4], /* Timing */
						row[1], /* Event */
						row[2], /* table */
						row[3], /*  Statement */
                        m_strnewline.GetString(),
                        m_strnewline.GetString());

	}
	else
	{
		if(isshowcreateworked && bodyoftrigger.GetLength()!=0)
		buffer->AddSprintf("%s/*!50003 CREATE */ %s /*!50003 TRIGGER `%s` %s %s ON `%s` FOR EACH ROW %s */$$%s%s", 
						m_strnewline.GetString(),
                        definer.GetString(), /*Definer*/
						row[0], /* Trigger */
						row[4], /* Timing */
						row[1], /* Event */
						row[2], /* table */
						bodyoftrigger.GetString(), /*  Statement */
                        m_strnewline.GetString(),
                        m_strnewline.GetString());
		else
			buffer->AddSprintf("%s/*!50003 CREATE */ %s /*!50003 TRIGGER `%s` %s %s ON `%s` FOR EACH ROW %s */$$%s%s", 
						m_strnewline.GetString(),
                        definer.GetString(), /*Definer*/
						row[0], /* Trigger */
						row[4], /* Timing */
						row[1], /* Event */
						row[2], /* table */
						row[3], /*  Statement */
                        m_strnewline.GetString(),
                        m_strnewline.GetString());

	}
	buffer->AddSprintf("%sDELIMITER ;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	sja_mysql_free_result(m_tunnel, res);
	sja_mysql_free_result(m_tunnel, res1);

/*	if(m_flushmaster == wyTrue) 
        OnFlushMaster();*/	

	if( m_individualtablefiles == wyTrue)
	{
		if(Footer(buffer) == wyFalse)
			return wyFalse;

		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}
	return wyTrue;
}

wyBool
MySQLDump::DumpTriggers(wyString * buffer, const wyChar *db)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query;
    wyInt32     err = 0;
	
	
	if(IsMySQL5010(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	m_routine(m_lpparam, "triggers", 0 , FETCHDATA);
	
	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where \
		//			`TRIGGER_SCHEMA` = '%s'", 
			//		m_db.GetString());
	
	query.Sprintf("SHOW TRIGGERS FROM `%s`", m_db.GetString());

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 
	if(!res)
		return OnError();

	//setting the progress bar range to total no of triggers
	m_routine(m_lpparam, m_db.GetString(), res->row_count , SETOBJECTCOUNT);

	row = sja_mysql_fetch_row(m_tunnel, res);	
	while(row)
	{
		if(!DumpTrigger(buffer, db, row[0], &err))
		{
			if(err) 
			{
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());

			} 
			else if(m_error.GetLength() == 0)
                 OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		    m_iserr = wyTrue; //if any error occur,set m_iserr = wyTrue
#endif
		   /* if m_abortonerr is true or if user prees top button or any file error . 
			  These three cases we r stopping the process */
			if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue ||err ) 
		   {
			   sja_mysql_free_result(m_tunnel, res);
			   return wyFalse;
		   }
		}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
		   return wyFalse;

		row = sja_mysql_fetch_row(m_tunnel, res);
	}		

	sja_mysql_free_result(m_tunnel, res);
	return wyTrue;
}	


/* This  function Dumps all the table if m_dumpAllTables option is set to wyTrue.*/

wyBool
MySQLDump::DumpAllTables(wyString * buffer, const wyChar *db)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query, tempstr, tablestr;
    wyInt32     err = 0;
	wyBool		ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	wyString    database;

	m_routine(m_lpparam, "tables", 0, FETCHDATA);
	
		/*	IMP: we need to export all tables and views as tables first then we will delete all the views at the end,
		this is b'cos, some views may refer to some other views, so when we are executing the dump it may give error... 
		this we are doing as in "mysqldump.exe" */
	
	database.SetAs(db);
	if(database.GetLength() == 0)
		return wyFalse;

	PrepareShowTable(m_tunnel, &m_mysql, database, query); 

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 
	if(!res)
		return OnError();

	//setting the progress bar range to total no of tables
	m_routine(m_lpparam, database.GetString(), res->row_count , SETOBJECTCOUNT);

    row = sja_mysql_fetch_row(m_tunnel, res);

	while(row)
	{
		if(row[0])
			tempstr.SetAs(row[0], ismysql41);
        //if(ismysql41 == wyTrue)
            tablestr.SetAs(tempstr.GetString());
        //else
         //   tablestr.SetAs(tempstr.GetAsAnsi());

		if(!DumpTable(buffer, db, tablestr.GetString(), &err))
		{
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
							
			} 
            else if(m_error.GetLength() == 0)
			    OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		    m_iserr = wyTrue;
#endif
			/* if m_abortonerr is true or if user prees top button or any file error . 
			   These three cases we r stopping the process. */
			if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err) 
			{
				sja_mysql_free_result(m_tunnel, res);
				return wyFalse;
			}
			
		}
		
		//If size of buffer is more than 8K, write to file
		if(WriteBufferToFile(buffer) == wyFalse)
		   return wyFalse;

		row = sja_mysql_fetch_row(m_tunnel, res);
	}		

	sja_mysql_free_result(m_tunnel, res);
	return wyTrue;
}

/* This function gets called if in case there is any error related to mysql database.
it will display errors and finally closes the export file*/
wyBool
MySQLDump::OnError()
{
	m_errno = sja_mysql_errno(m_tunnel, m_mysql);
	m_error.SetAs(sja_mysql_error(m_tunnel, m_mysql));

	return wyFalse;
}

wyInt32
MySQLDump::GetErrorNum()
{
	return m_errno;
}

const wyChar*
MySQLDump::GetError()
{
	return m_error.GetString();
}

/* This function initializes dumping */
EXPORTERROR
MySQLDump::Dump(LPGUI_EXPORT_UPDATE_ROUTINE routine, void *lpparam, wyInt32 *stopexport)
{
	wyBool err;

	if(!ConnectToDataBase())
		return MYSQLERROR;
	
	if(!m_individualtablefiles && ! m_individualdbfiles)
    {
	    if(!OpenDumpFile())
		    return FILEERROR;
    }
		
	/* get the quote character */
	m_maxallowedsize = Get_Max_Allowed_Packet_Size(m_tunnel, &m_mysql) ;

	m_lpparam		= lpparam;
	m_routine		= routine;
	m_stopexport	= stopexport;

    err = StartDump();

	/*if abortonerr is true, if an error occured while exporting, it will stop exporting and 
	  return wyFalse(err= wyFalse) and m_iserr= wyTrue.	
	  if abortonerr is false, if an error occured while exporting ,it continues exporting, 
	  and after exporting it returns wyTrue(err= wyTrue) and m_iserr = wyTrue.
	  m_iserr = wyTrue means an error occured while exporting , by default m_iserr is wyFalse.
	  if exporting completed successfully, then it returns wyTrue and m_iserr = wyFalse */
	return(err == wyTrue && m_iserr == wyFalse)?(SUCCESS):(MYSQLERROR);
}	

/* This function opens the output file,
    Table structure and the data will be dumped in to the file opened by this function.*/
wyBool
MySQLDump::SafeExit()
{	
#if !defined _WIN32 || defined _CONSOLE 
	if(m_compress == wyTrue)
	{
		m_zip.CloseFileInZip();
		m_zip.CloseZip();
	}
#endif

	if(m_expfile)
		fclose(m_expfile);
	
	m_expfile = NULL;
	
	if(m_mysql && m_tunnel)
		sja_mysql_close(m_tunnel, m_mysql);

	m_mysql = NULL;

	if(m_tunnel)
        delete m_tunnel;
   
	m_tunnel = NULL;
	m_escapeddatalen = 0;

	//free the memory
	if(m_escapeddata)
		free(m_escapeddata);

	return wyTrue;
}

/* This function Opens output file and assigns name , that was 
  set by user using SetExpFile() function.*/
wyBool
MySQLDump::OpenDumpFile()
{		
	if(m_filemodetimestamp)
		GetFormattedFilePath(m_file);
	
	if(m_compress == wyFalse)
	{
#ifdef _WIN32
		if(m_file.GetLength())
			m_expfile = _wfopen(m_file.GetAsWideChar(), m_filemodeappend ? TEXT(APPEND_MODE) : TEXT(WRITE_MODE));
		else
			return wyFalse;
#else
		if(m_file.GetLength())
			m_expfile = fopen(m_file.GetString(), m_filemodeappend ? APPEND_MODE : WRITE_MODE);
#endif
		if(!m_expfile) 
			return wyFalse;
	}

#if !defined _WIN32 || defined _CONSOLE 
	else if(m_compress == wyTrue && m_file.GetLength())
	{		
		wyString		zipfile,tmp;
		wyString		strtemp;		
		wyInt32			filelen =  0, extpos = 0;

		if(m_zip.OpenZip(m_file.GetAsWideChar(),APPEND_STATUS_CREATE) != wyTrue)
			return wyFalse;

#ifdef _WIN32
		WIN32_FIND_DATA ff32;
		HANDLE			hFile = INVALID_HANDLE_VALUE;

		hFile=FindFirstFile(m_file.GetAsWideChar(),&ff32);
		if(hFile==INVALID_HANDLE_VALUE)
		{
			m_zip.CloseZip();
			return wyFalse;
		}
		else
		{
			zipfile.SetAs(ff32.cFileName);

			if(hFile != INVALID_HANDLE_VALUE)
				FindClose(hFile);
		}

#else
		zipfile.SetAs(m_file);
		GetFormattedFilePath(zipfile, wyFalse);
#endif
			
		//Trimming the .zip extension and make it as .sql
		zipfile.LTrim();
		zipfile.RTrim();
		filelen = zipfile.GetLength();
		
		if(filelen <= 4)
		{
			m_zip.CloseZip();
			return wyFalse;
		}

		for(extpos = filelen; extpos; extpos--)
		{
			if(zipfile.GetCharAt(extpos) == '.')
				break;
		}
        
		if(extpos == 0)
		{
			m_zip.CloseZip();
			return wyFalse;
		}
		        
		extpos++;

		if(extpos == filelen)
		{
			m_zip.CloseZip();
			return wyFalse;
		}

		//Exract the extension name
		strtemp.SetAs(zipfile.Substr(extpos, filelen));
        
		if(strtemp.CompareI("zip"))
		{
			m_zip.CloseZip();
			return wyFalse;		
		}

		strtemp.SetAs(zipfile.Substr(0, extpos));
		m_file.SetAs(strtemp);
				
		//Append the .sql extension
		m_file.Add("sql");
		m_zip.OpenFileInZip(m_file.GetAsWideChar(), 0);                                   
			
	}
#endif

	else 
		return wyFalse;	

	return wyTrue;
}

wyBool
MySQLDump::Title(wyString * buffer)
{
#ifdef _WIN32
	wyBool	ismysql41 = IsMySQL41(m_tunnel, &m_mysql);


	/* get the mysql version if its tunnel mode */
	if(!m_tunnel->GetMySQLVersion(m_mysql))
        return wyFalse;

	/* include the top header showing the server and database informations */
	buffer->AddSprintf("/*%s%s\nMySQL - %s : Database - %s%s%s%s*/\r\n",
                         m_strnewline.GetString(),
                         m_appname.GetString(),
						 m_mysql->server_version,
                         ismysql41?m_db.GetString():m_db.GetAsAnsi(),
                         m_strnewline.GetString(),
						 "*********************************************************************",
                         m_strnewline.GetString()
						 );
	
	#else
	buffer->AddSprintf("/*%s%s\nMySQL - %s : Database - %s%s%s%s*/\r\n",
                         m_strnewline.GetString(),
						 m_appname.GetString(),
						 m_mysql->server_version,
                         m_db.GetString(),
                         m_strnewline.GetString(),
						 "*********************************************************************",
                         m_strnewline.GetString()
						 );	
#endif

return wyTrue;
}

/* This function initiates Dumping of All tables or selected tables.*/  
/* Prints header information at begining of the exp file */
wyBool
MySQLDump::Header(wyString * buffer)
{
/* It is normal that when inserting 0, MySQL takes it as NULL and inserts the next valid value which is 1.
 From SQLyog v5.1 BETA onwards we are using NO_AUTO_VALUE_ON_ZERO option to override the issue
 and keeping Mysql mode in variable called OLD_SQL_MODE
fprintf(m_expfile, "\nSET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;\n");*/
	
	buffer->AddSprintf("%s/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;", m_strnewline.GetString());

	if(m_setfkchecks == wyTrue) 
		buffer->AddSprintf("%s/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;", m_strnewline.GetString());
		
	buffer->AddSprintf("%s/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;", m_strnewline.GetString());
    buffer->AddSprintf("%s/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;", m_strnewline.GetString());
	
	return wyTrue;
}

/* prints Footer information at the end of the Exp file*/
wyBool
MySQLDump::Footer(wyString * buffer)
{	
	buffer->AddSprintf("%s/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	if(m_setfkchecks == wyTrue)
		buffer->AddSprintf("/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;%s", m_strnewline.GetString());

	buffer->AddSprintf("/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;%s", m_strnewline.GetString());
	buffer->AddSprintf("/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;%s", m_strnewline.GetString());
		
	return wyTrue;
	
}

/* Initiates dumping for all or selected database.
	First data is added into a buffer of size 8K 
	and then it is written to file*/
wyBool 
MySQLDump::StartDump()
{	
	wyString	buffer(SIZE_8K);
    
    if(m_compress == wyTrue)
    {
        m_strnewline.SetAs("\r\n");
    }
    else
    {
        m_strnewline.SetAs("\n");
    }

	//Add data to buffer
	if(m_dumpalldbs == wyTrue) 
    { 
		if(DumpAllDatabases(&buffer) == wyFalse)
			return wyFalse;
	}
	else 
    {
		if(DumpSelectedDatabases(&buffer, m_db) == wyFalse)
			return wyFalse;
	}


	//Write to file , if data is there in buffer
					if(WriteBufferToFile(&buffer, wyTrue) == wyFalse)
		return wyFalse;

	if(m_flushmaster == wyTrue)
	{
		if(OnFlushMaster() == wyFalse)
		{
				return wyFalse;
		}
	}

	return wyTrue;
}

/* This functions fetches  all data bases in to result set object*/
wyBool
MySQLDump::DumpAllDatabases(wyString * buffer)
{  
	MYSQL_RES		*res = NULL;
	MYSQL_ROW		row;
    wyString        query, dbname;
    wyBool	        success;
    wyBool          ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	wyBool			ismysql553 = IsMySQL553(m_tunnel, &m_mysql);
	wyBool			ismysql577 = IsMySQL577(m_tunnel, &m_mysql);
	wyInt32			err = 0;

	if(m_charset.CompareI("utf8") == 0)
	{
		if(DumpDatabaseOnUtf8() == wyFalse)
		{
            return wyFalse;	
		}
	}
	
	query.SetAs("show databases");

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
	if(!res) 
		return OnError();
  
	
	m_db.SetAs("(ALL)");


	if(m_individualdbfiles == wyFalse)
	{
        if(Title(buffer) == wyFalse)
		    return wyFalse;

        if(PrintDBHeaderInfo(buffer) == wyFalse)
	        return wyFalse;

		//exporting all db into a single file 

		if(m_dumpalldbs == wyTrue)
		{
			if(DumpAlldbsToSingleFile(buffer, res) == wyFalse)
			{
				sja_mysql_free_result(m_tunnel, res);
				return wyFalse;
			}

			sja_mysql_free_result(m_tunnel, res);
			if(m_individualdbfiles == wyFalse)
				if(Footer(buffer) == wyFalse)
				return wyFalse ;

			return wyTrue;
		}
	}

	sja_mysql_data_seek(m_tunnel, res, 0);    
	row = sja_mysql_fetch_row(m_tunnel, res);
    
	while(row)
	{
        if(row[0])
            dbname.SetAs(row[0], ismysql41);
		m_db.SetAs(dbname);
		
		if(stricmp(row[0], "INFORMATION_SCHEMA") == 0 ) //INFORMATION_SCHEMA is not exported
		{	
			if(IsMySQL5010(m_tunnel, &m_mysql) == wyTrue) 
			{	
				row = sja_mysql_fetch_row(m_tunnel, res);
				continue;
			}
		}
		else if(stricmp(row[0], "PERFORMANCE_SCHEMA") == 0) //PERFORMANCE_SCHEMA is not exported
		{	
			if(ismysql553 == wyTrue) 
			{	
				row = sja_mysql_fetch_row(m_tunnel, res);
				continue;
			}
		}
		else if(stricmp(row[0], "SYS") == 0) //SYS SCHEMA is not exported for Mysql 5.7.7 and higher version
		{	
			if(ismysql577 == wyTrue) 
			{	
				row = sja_mysql_fetch_row(m_tunnel, res);
				continue;
			}
		}
				
		else if(stricmp(row[0], "mysql") == 0 ) 
		{
			/*whether we need to export mysql or not. 
			  if we don't want to export mysql, then we are skipping mysql database.*/
			if(m_expmysql == wyFalse) 
			{
				row = sja_mysql_fetch_row(m_tunnel, res);
				continue;
			}
		}
		 m_routine(m_lpparam, dbname.GetString(), 0, DATABASESTART);

		if(ismysql41 == wyTrue)
			success = DumpDatabase(buffer, dbname.GetString(), &err);
		else
			success = DumpDatabase(buffer, dbname.GetAsAnsi(), &err);

		if(success == wyFalse)
		{
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
				
			
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
			} 
            
			/* if m_abortonerr is true or if user prees top button or any file error . 
			   these three cases we r stopping the process */
			if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue ||err)  
			{
				sja_mysql_free_result(m_tunnel, res);
				return wyFalse;
			}

		}
		
			m_routine(m_lpparam, row[0], 0, ENDSTART);

		row = sja_mysql_fetch_row(m_tunnel, res);
	}

	sja_mysql_free_result(m_tunnel, res);
	
	if(m_individualdbfiles == wyFalse)
		if(Footer(buffer) == wyFalse)
		return wyFalse ;

	//If size of buffer is more than 8K, write to file
	if(WriteBufferToFile(buffer) == wyFalse)
		return wyFalse;

	return wyTrue;
}

/*This function dumps only selected databases */
wyBool
MySQLDump::DumpSelectedDatabases(wyString * buffer, wyString &dbs)
{	
	wyChar      *startpointer, *tokpointer;
    wyString    db;
	
	/* converting multipule batabase name string in to single database name tokens
               	and pass  each database name to DumpDatabase()to start dump */
    startpointer = AllocateBuff(dbs.GetLength() + 1);
    strcpy(startpointer, dbs.GetString());

	tokpointer =  strtok(startpointer, SEPS);
	 
	while(tokpointer != NULL)
	{
		db.Sprintf("%s", tokpointer);

		tokpointer = strtok(NULL, SEPS);

		if(DumpDatabase(buffer, db.GetString()) == wyFalse)
        {
            if(startpointer)
                free(startpointer);
			return wyFalse;
        }
	}

    if(startpointer)
        free(startpointer);

	//If size of buffer is more than 8K, write to file
	if(WriteBufferToFile(buffer) == wyFalse)
		return wyFalse;
	
	return wyTrue;
}

/*  This function prints Database related header information 
    to the export file */
wyBool
MySQLDump::PrintDBHeaderInfo(wyString * buffer)
{
	///For 6.07 job files the <usecharset> is 'yes' or 'no' if its no m_charset would be empty
    if(m_charset.GetLength())
        buffer->AddSprintf("%s/*!40101 SET NAMES %s */;%s", m_strnewline.GetString(), m_charset.GetString(), m_strnewline.GetString());
		
	buffer->AddSprintf("%s/*!40101 SET SQL_MODE=''*/;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	if(Header(buffer) == wyFalse)
		return wyFalse;
	
	return wyTrue;
}
wyBool
MySQLDump::DumpAlldbsToSingleFile(wyString * buffer, MYSQL_RES *dbres)
{
	wyBool     ret = wyFalse;
	MYSQL_ROW  row;
	wyString   dbname;	
	wyBool     ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	wyBool     ismysql51 = IsMySQL5010(m_tunnel, &m_mysql);
	wyBool	   ismysql553 = IsMySQL553(m_tunnel, &m_mysql); 
	wyBool	   ismysql577 = IsMySQL577(m_tunnel, &m_mysql); 
	


	//dump all tables and view table strucure of all dbs
	while(row = sja_mysql_fetch_row(m_tunnel, dbres))
	{
		if(row[0])
            dbname.SetAs(row[0], ismysql41);

		m_db.SetAs(dbname);

		if((stricmp(dbname.GetString(), "INFORMATION_SCHEMA") == 0) && (ismysql51 == wyTrue))
			continue;

		if((stricmp(dbname.GetString(), "PERFORMANCE_SCHEMA") == 0) && (ismysql553 == wyTrue))
			continue;

		if((stricmp(dbname.GetString(), "SYS") == 0) && (ismysql577 == wyTrue))
			continue;
		
		if((stricmp(dbname.GetString(), "MYSQL") == 0) && (m_expmysql == wyFalse))
			continue;

#ifdef _WIN32
	/* set the db in the tunnel coz if its on HTTP then we need to select the db everytime */
	/* not required in Linux as its always a direct connections */
	m_tunnel->SetDB(dbname.GetString());
#endif

		//dump tables and view table strucures of db
		if(ismysql41 == wyTrue)
			ret = DumpTableAndViewTableStructure(buffer, dbname.GetString());
		else
			ret = DumpTableAndViewTableStructure(buffer, dbname.GetAsAnsi());

		if(ret == wyFalse)
			return wyFalse;
		
	}

	//dump db routines if mysql version is more than 5010
	if(IsMySQL5010(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;
	
	sja_mysql_data_seek(m_tunnel, dbres, 0);

	while(row = sja_mysql_fetch_row(m_tunnel, dbres))
	{
		if(row[0])
            dbname.SetAs(row[0], ismysql41);		

		if((stricmp(dbname.GetString(), "INFORMATION_SCHEMA") == 0) && (ismysql51 == wyTrue))
			continue;

		if((stricmp(dbname.GetString(), "PERFORMANCE_SCHEMA") == 0) && (ismysql553 == wyTrue))
			continue;

		if((stricmp(dbname.GetString(), "SYS") == 0) && (ismysql577 == wyTrue))
			continue;

		if((stricmp(dbname.GetString(), "MYSQL") == 0) && (m_expmysql == wyFalse))
			continue;

#ifdef _WIN32
	/* set the db in the tunnel coz if its on HTTP then we need to select the db everytime.
	   Not required in Linux as its always a direct connections */
	m_tunnel->SetDB(dbname.GetString());
#endif
		
		//dump use db statement
		if(ismysql41 == wyTrue)
            buffer->AddSprintf("%sUSE `%s`;%s", m_strnewline.GetString(), dbname.GetString(), m_strnewline.GetString());
		else 
			buffer->AddSprintf("%sUSE `%s`;%s", m_strnewline.GetString(), dbname.GetAsAnsi(), m_strnewline.GetString());
		
		//select the context db.
		ChangeContextDatabase(dbname.GetString());

		//dump db 5.x routines
		if(DumpDBRoutines(buffer, dbname.GetString()) == wyFalse)
			return wyFalse;
	
	}	

	return wyTrue;
    
}

//dump 5.x objects
wyBool
MySQLDump::DumpDBRoutines(wyString * buffer, const wyChar *db)
{

	m_db.SetAs(db);
	if(DumpTriggers(buffer, m_db.GetString()) == wyFalse)
		return wyFalse;
	if(DumpEvents(buffer, m_db.GetString()) == wyFalse)
		return wyFalse;
	if(DumpFunctions(buffer, m_db.GetString()) == wyFalse)
		return wyFalse;
	if(DumpProcedures(buffer, m_db.GetString()) == wyFalse)
		return wyFalse;
	if(DumpViews(buffer, m_db.GetString(), wyFalse) == wyFalse)
		return wyFalse;

	return wyTrue;
}
//fun will dump db create statement, table  and temporary tables of views
wyBool
MySQLDump::DumpTableAndViewTableStructure(wyString * buffer, const wyChar *db)
{

	MYSQL_RES  *tableres = NULL;
	MYSQL_ROW  row;
	wyString   query;
	wyString   view, tablestr, tempstr;
	wyInt32	   err = 0;
	wyBool     ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	wyBool     ismysql5010 = IsMySQL5010(m_tunnel, &m_mysql);

	if(m_charset.CompareI("utf8") == 0)
        if(DumpDatabaseOnUtf8() == wyFalse)
            return wyFalse;

	if(DumpDb(buffer, db) == wyFalse)
		return wyFalse;
		
#ifdef _WIN32
	/* set the db in the tunnel coz if its on HTTP then we need to select the db everytime */
	/* not required in Linux as its always a direct connections */
	m_tunnel->SetDB(db);
#endif

//	if(m_firstslave == wyTrue || m_flushmaster == wyTrue)
/*	if(m_flushmaster == wyTrue)
		DumpDatabaseOnMasterSlave();*/	

	ChangeContextDatabase(db);

	//here query is for retrieving tables (including views also) of a db. views are supporting from 5.x only
	//hence we are using two types of query formates for tables.
	
	query.Sprintf("SHOW %s TABLES FROM `%s`",  ((ismysql5010 == wyTrue)?("FULL"):""), m_db.GetString()); 

	tableres = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query); 

	if(!tableres)
		return OnError();

   	while(row = sja_mysql_fetch_row(m_tunnel, tableres))
	{
		if(row[0])
			tempstr.SetAs(row[0], ismysql41);

		if(ismysql5010 == wyTrue)
		{
			if(!row[1])
			{
				sja_mysql_free_result(m_tunnel, tableres);
				return wyFalse;
			}

			if(!(strcmp(row[1], "VIEW")))
				continue;
		}
       
		tablestr.SetAs(tempstr.GetString());       

		if(!DumpTable(buffer, db, tablestr.GetString(), &err))
		{
			if(err) 
            {
				/* set special error */
				m_error.SetAs(m_fileerr.GetString());
							
			} 
            else if(m_error.GetLength() == 0)
			    OnError();

#if !defined _WIN32 || defined _CONSOLE 	  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		    m_iserr = wyTrue;
#endif
			/* if m_abortonerr is true or if user prees top button or any file error . 
			   these three cases we r stopping the process */
			if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue || err) 
			{
				sja_mysql_free_result(m_tunnel, tableres);
				return wyFalse;
			}
			
		}       
	}

	//if mysql version is greater than 5010 dump table strucure for views.
	if(ismysql5010 == wyFalse)
	{
		sja_mysql_free_result(m_tunnel, tableres);
		return wyTrue;	
	}

	sja_mysql_data_seek(m_tunnel, tableres, 0);
    
	//dump temporary tables for views
	while(row = sja_mysql_fetch_row(m_tunnel, tableres))
	{
		if(!row[0] || !row[1])
		{
			sja_mysql_free_result(m_tunnel, tableres);
			return wyFalse;
		}		

		if(!(strcmp(row[1], "BASE TABLE")))
			continue;		

		view.SetAs(row[0]);

		if(DumpTableStructureOnView(buffer, view.GetString(), wyTrue) == wyFalse)
		{
			#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   sja_mysql_free_result(m_tunnel, tableres);
				return wyFalse;
		   }
		}
        
	}

	if(tableres)
		sja_mysql_free_result(m_tunnel, tableres);

	return wyTrue;		
}


	/* This function Dumps the data base with option selected tables or 
	it will dump all tables in the database or selected tables depending upon the options */
wyBool
MySQLDump::DumpDatabase(wyString * buffer, const wyChar *db, wyInt32 *fileerr)
{
    wyString    query;
		
	//exporting all db into a directory(separate file for each) 
	if(m_individualdbfiles == wyTrue)
        if(OnIndividualFiles(buffer, db, fileerr) == wyFalse)
			return wyFalse;
			
	if(m_charset.CompareI("utf8") == 0)
        if(DumpDatabaseOnUtf8() == wyFalse)
            return wyFalse;	

	if(m_singletransaction == wyTrue)
		OnSingleTransaction();

	//exporting single db
	if(m_individualtablefiles == wyFalse && m_dumpalldbs == wyFalse)
	{
        if(Title(buffer) == wyFalse)
		    return wyFalse;

        if(PrintDBHeaderInfo(buffer) == wyFalse)
	        return wyFalse;

		if(DumpDb(buffer, db) == wyFalse)
			return wyFalse;
	}
	

	
#ifdef _WIN32
	/* set the db in the tunnel coz if its on HTTP then we need to select the db everytime */
	/* not required in Linux as its always a direct connections */
	m_tunnel->SetDB(db);
#endif

//	if(m_firstslave == wyTrue || m_flushmaster == wyTrue)
   
    CHECKSTOP()

    ChangeContextDatabase(db);
	//m_db.SetAs(db);
   	
	if(DumpDatabaseStart(buffer) == wyFalse)
	{
		FreeMemory();
        return wyFalse;
	}
	FreeMemory();
	
	if(m_individualdbfiles == wyTrue ||( m_individualtablefiles == wyFalse && m_dumpalldbs == wyFalse))
	{
		if(Footer(buffer) == wyFalse)
			return wyFalse;

		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}
	 
	return wyTrue;
}	
void
MySQLDump::FreeMemory()
{
	ReleaseMemory(&m_seltables);
	ReleaseMemory(&m_selviews);
	ReleaseMemory(&m_selfunctions);
	ReleaseMemory(&m_selprocedures);
	ReleaseMemory(&m_seltriggers);
	ReleaseMemory(&m_selevents);
}


wyBool
MySQLDump::DumpDatabaseOnUtf8()
{
    MYSQL_RES   *res;
    wyString    query;

    query.SetAs("SET NAMES UTF8");
    
    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

#ifdef _WIN32

    m_tunnel->SetCharset("utf8");
    
#endif

    if(res)
		sja_mysql_free_result(m_tunnel, res);

    return wyTrue;
}

wyBool 
MySQLDump::ChangeContextDatabase(const wyChar *db)
{
    wyString    query;
    MYSQL_RES   *res;

	query.Sprintf("USE `%s`", db); 

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(res)
	    sja_mysql_free_result(m_tunnel, res);

    return wyTrue;
 }

wyBool
MySQLDump::DumpDatabaseOnMasterSlave()
{
    MYSQL_RES	*res;
    wyString    query;

    query.SetAs("FLUSH TABLES WITH READ LOCK");

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(res)
		sja_mysql_free_result(m_tunnel, res);

	m_locktables = wyFalse; // no other locks needed

    return wyTrue;
}


wyBool
MySQLDump::DumpDatabaseStart(wyString * buffer)
{
	// if all objects are selected 
   	if(m_allobjects == wyTrue || m_dumpalldbs == wyTrue)
	{
		if(DumpAllTables(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;	

		if(DumpTriggers(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;

		if(DumpEvents(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;

		if(DumpFunctions(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;

		if(DumpProcedures(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;

		if(DumpViews(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;

		return wyTrue;
	}		
	
	// if all tables are selected
	if(m_alltables == wyTrue)
	{
		if(DumpAllTables(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;
	}
	// if some tables are selected  for exporting
	else if(m_seltables.GetFirst())
	{
		if(DumpSelectedTables(buffer, m_db) == wyFalse)
			return wyFalse;
	}

	if(m_alltriggers == wyTrue)
	{
		if(DumpTriggers(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;
	}
	else if(m_seltriggers.GetFirst())
	{
		if(DumpSelectedTriggers(buffer) == wyFalse)
			return wyFalse;	
	}

	// if all events are selected
	if(m_allEvents == wyTrue)
	{
		if(DumpEvents(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;	
	}
	else if(m_selevents.GetFirst())
	{
		if(DumpSelectedEvent(buffer) == wyFalse)
			return wyFalse;	
	}
	
	// if all functions are selected
	if(m_allfunctions == wyTrue)
	{
		if(DumpFunctions(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;
	}
	// whether all functions are selected or nofunction is selected for exporting	
	else if(m_selfunctions.GetFirst())
	{
		if(DumpSelectedFunction(buffer) == wyFalse)
			return wyFalse;
	}

	// if all procedures are selected
	if(m_allprocedures == wyTrue)
	{
		if(DumpProcedures(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;
	}
	// whether all procedures are selected or no procedure  is selected for exporting
	else if(m_selprocedures.GetFirst())
	{
		if(DumpSelectedProcedure(buffer) == wyFalse)
		return wyFalse;
	}
	
	// if all views are selected  for exporting
	if(m_allviews == wyTrue)
	{
		if(DumpViews(buffer, m_db.GetString()) == wyFalse)
			return wyFalse;
	}
	// if some views are selected  for exporting
	else if(m_selviews.GetFirst())
	{
		 
		if(DumpSelectedViews(buffer) == wyFalse)
			return wyFalse;
	}

	return wyTrue;
}

/* This function Fetches the Structure of Specified Table
 * it uses  MYSQl_RES object to store  the structure  of the Table
 * finally it outputs the table structure to the m_expfile file object.*/
wyBool  
MySQLDump::DumpTableStructure(wyString * buffer, const wyChar *db, const wyChar *table)
{
   	wyString		query, myrowstr;
	MYSQL_RES		*res = NULL;			// result set  object to store table structure  
	MYSQL_ROW	  	row;
	MYSQL_FIELD		*field;
    wyChar			*tablech = NULL;

#ifdef _WIN32
	ConvertString	 conv;
	wyBool			 ismysql41 = IsMySQL41(m_tunnel, &m_mysql);	
#endif

	if(m_tunnel->IsTunnel())
		SetDefaultSqlMode();

	query.Sprintf("show create table `%s`.`%s`", db, table);
	CHECKSTOP()

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
	if(!res)
		return OnError();

	field= sja_mysql_fetch_field(m_tunnel, res);

	m_views = wyFalse;

	if(strcmp(field->name, "View") == 0)
    {
		m_views = wyTrue;

		if(m_createviews == wyFalse)
		  return wyTrue;
	}	

    /*  we don't want to dump the table structure(m_dataonly), 
	    we are executing this part to identify the object is table/view. */
	if(m_dataonly == wyTrue) 
        return wyTrue;
	
#ifdef _WIN32
	if(ismysql41 == wyFalse)
	{
		wyInt32	len = strlen(table);
		tablech = conv.Utf8toAnsi(table, len);
        buffer->AddSprintf("%s/*Table structure for table `%s` */%s", m_strnewline.GetString(), tablech, m_strnewline.GetString());
	}
	else
		buffer->AddSprintf("%s/*Table structure for table `%s` */%s", m_strnewline.GetString(), table, m_strnewline.GetString());
#else
	buffer->AddSprintf("%s/*Table structure for table `%s` */%s", m_strnewline.GetString(), table, m_strnewline.GetString());
	
#endif

    if(m_dropobjects == wyTrue)
    {
        if(strcmp(field->name, "View") != 0 || !m_tunnel->IsTunnel())
		{
#ifdef _WIN32
			if(ismysql41 == wyFalse)
        		buffer->AddSprintf("%sDROP TABLE IF EXISTS `%s`;%s", m_strnewline.GetString(), tablech, m_strnewline.GetString());
			else
				buffer->AddSprintf("%sDROP TABLE IF EXISTS `%s`;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
#else
	        buffer->AddSprintf("%sDROP TABLE IF EXISTS `%s`;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
#endif
		}
    }
	
    if(strcmp(field->name, "View") == 0)
    {
        sja_mysql_free_result(m_tunnel, res);

        return DumpTableStructureOnView(buffer, table);
    }

	row = sja_mysql_fetch_row(m_tunnel, res);
	
	buffer->AddSprintf("%s%s;%s", m_strnewline.GetString(), row[1], m_strnewline.GetString());
	
	sja_mysql_free_result(m_tunnel, res);
	
	return wyTrue;
}

wyBool
MySQLDump::DumpSelectedViews(wyString * buffer)
{
	SelectedObjects  *view, *tempview;
	
	view = (SelectedObjects*)m_selviews.GetFirst();

	//setting the progress bar range to total no of selected views
	m_routine(m_lpparam, m_db.GetString(), m_selviews.GetCount(), SETOBJECTCOUNT);
	
	// if individual file used for exporting database objects no need to export table structure for views
	if(m_individualtablefiles == wyFalse)
	{
		tempview = view;
		for( ; view; view = (SelectedObjects*)view->m_next)
		{
			if(DumpTableStructureOnView(buffer, view->m_selobject.GetString(), wyTrue) == wyFalse)
			{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   		return wyFalse;
		   
			}
		}
		view = tempview;	
	}
	for( ; view; view = (SelectedObjects*)view->m_next)
	{
		if(DumpView(buffer, m_db.GetString(), view->m_selobject.GetString()) == wyFalse)
		{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle); 
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   return wyFalse;			
		   }
		}
		
		//If size of buffer is more than 8K, write to file
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	}  

	return wyTrue;
}

wyBool
MySQLDump::DumpSelectedTriggers(wyString * buffer)
{
	SelectedObjects  *trigger;
	
	trigger = (SelectedObjects*)m_seltriggers.GetFirst();

	//setting the progress bar range to total no of selected triggers
	m_routine(m_lpparam, m_db.GetString(), m_seltriggers.GetCount() , SETOBJECTCOUNT);

	for( ; trigger; trigger = (SelectedObjects*)trigger->m_next)
	{
		if(DumpTrigger(buffer, m_db.GetString(), trigger->m_selobject.GetString()) == wyFalse)
		{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   return wyFalse;			
		   }	
		}
		
		//If size of buffer is more than 8K, write to file
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	}  

	return wyTrue;
}

wyBool
MySQLDump::DumpSelectedProcedure(wyString * buffer)
{
	SelectedObjects  *procedure;
	
	procedure = (SelectedObjects*)m_selprocedures.GetFirst();

	//setting the progress bar range to total no of selected procedures
	m_routine(m_lpparam, m_db.GetString(), m_selprocedures.GetCount(), SETOBJECTCOUNT);
	
	for( ; procedure; procedure = (SelectedObjects*)procedure->m_next)
	{
		if(DumpProcedure(buffer, m_db.GetString(), procedure->m_selobject.GetString()) == wyFalse )
		{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   return wyFalse;			
		   }
		}

		//If size of buffer is more than 8K, write to file
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	} 
	 return wyTrue;
}

wyBool
MySQLDump::DumpSelectedFunction(wyString * buffer)
{
	SelectedObjects  *function;	

	function = (SelectedObjects*)m_selfunctions.GetFirst();

	//setting the progress bar range to total no of selected function
	m_routine(m_lpparam, m_db.GetString(), m_selfunctions.GetCount() , SETOBJECTCOUNT);

	for( ; function; function = (SelectedObjects*)function->m_next)
	{
		if(DumpFunction(buffer, m_db.GetString(), function->m_selobject.GetString()) == wyFalse )
		{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle); 
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		   m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   return wyFalse;			
		   }
		}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	} 

	return wyTrue;
}
wyBool
MySQLDump::DumpSelectedEvent(wyString * buffer)
{
	SelectedObjects  *event;	
	wyBool          seteventschdule = wyTrue;

	// if mysql version is below 5.1 6 then it is not supports events
	if(IsMySQL516(m_tunnel, &m_mysql) == wyFalse)
		return wyTrue;

	event = (SelectedObjects*)m_selevents.GetFirst();

	//setting the progress bar range to total no of selected events
	m_routine(m_lpparam, m_db.GetString(), m_selevents.GetCount() , SETOBJECTCOUNT);

	for( ; event; event = (SelectedObjects*)event->m_next)
	{
		if(DumpEvent(buffer, m_db.GetString(), event->m_selobject.GetString(), seteventschdule) == wyFalse )
		{
#if !defined _WIN32 || defined _CONSOLE 		  
		   PRINTERROR(m_error.GetString(), logfilehandle);
		   AppendErrMessage(m_error.GetString(), m_error.GetString());
		    m_iserr = wyTrue;
#endif
		   if(m_abortonerr == wyTrue || IsExportingStopped() == wyTrue)
		   {
			   return wyFalse;			
		   }
		}

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	}
	return wyTrue;
}

wyBool  
MySQLDump::DumpTableStructureOnView(wyString * buffer, const wyChar *view, wyBool isview)
{
    MYSQL_RES   *res = NULL;
    //MYSQL_ROW   row;
    wyString    query;

    //If it is HTTP, we are not creating temporary table.
	//HTTP is a stateless connection(separate connection for each query).
	//if we create one table,and if we can't connect for next query,then that table will persist(we can't execute drop stmt for that table).
	if(m_tunnel->IsTunnel())
        return wyTrue;

	//if object is view then we do not create temporary table 
	query.Sprintf("SHOW FIELDS FROM `%s` ", view);

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1) 
		return OnError();
	
	//	query.Sprintf("CREATE TEMPORARY TABLE `%s` SELECT * FROM `%s` WHERE 0", view, view);
	//
	//	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	//	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1) 
	//		return OnError();

	//	/*Get CREATE statement for the temp table */
	//	query.Sprintf("SHOW CREATE TABLE `%s`", view);
	//	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	//	if(!res)
	//		return OnError();
    

	if(isview == wyTrue)
	{
        buffer->AddSprintf("%s/*Table structure for table `%s` */%s", m_strnewline.GetString(), view, m_strnewline.GetString());
		buffer->AddSprintf("%sDROP TABLE IF EXISTS `%s`;%s", m_strnewline.GetString(), view, m_strnewline.GetString());
	}

    if(m_dropobjects == wyTrue)
    {
		buffer->AddSprintf("%s/*!50001 DROP VIEW IF EXISTS `%s` */;", m_strnewline.GetString(), view);
		buffer->AddSprintf("%s/*!50001 DROP TABLE IF EXISTS `%s` */;%s", m_strnewline.GetString(), view, m_strnewline.GetString());
	}

	//Gets the field inforamtion of the view
	DumpViewStruct(buffer, m_tunnel, view, res);

	sja_mysql_free_result(m_tunnel,res);	
	
    return wyTrue;
}


// set SQL mode
void
MySQLDump::SetDefaultSqlMode()
{
	wyString		query;
	MYSQL_RES		*res;

	query.Sprintf("set sql_mode=''");

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

    if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1) 
		return;

	sja_mysql_free_result(m_tunnel, res);

	return;
}


/* This function gets rowcount */
wyUInt32 
MySQLDump::GetRowCount(const wyChar *db , const wyChar *table)
{
	wyString        rcountquery;
	wyInt32			rcount;
	MYSQL_ROW		rownum;	
	MYSQL_RES		*rowinfo = NULL;

	/* select count(*) to get the number of rows from desired table */ 
	rcountquery.Sprintf("select count(*) from `%s`.`%s`", db, table);
	CHECKSTOP()
	
    rowinfo = SjaExecuteAndGetResult(m_tunnel, &m_mysql, rcountquery);
	if(!rowinfo)
		return OnError();

	rownum = sja_mysql_fetch_row(m_tunnel, rowinfo);
	rcount = atoi(rownum[0]);

	sja_mysql_free_result(m_tunnel, rowinfo);
	
	return  rcount;
}



wyBool
MySQLDump::DumpTable(wyString * buffer, const wyChar *db, const wyChar *table, wyInt32 *fileerr)
{
	 wyString           query;
	 wyBool			    ret = wyTrue;	 
     		
	 if(m_individualtablefiles == wyTrue )
         if(OnIndividualFiles(buffer, table, fileerr) == wyFalse)
             return wyFalse;
     
	 m_routine(m_lpparam, table, 0, TABLESTART);
  
	 if(m_flushlog == wyTrue)  
	 {
        if(OnFlushLog() == wyFalse)
			return wyFalse;
	 }

	 //if lock table is true and single transaction is enabled then we are not executing lock tables.lock table true and single transaction are mutually exclusive
	 //if single transaction is false then only we are executing lock tables
	 if(m_locktables == wyTrue && m_singletransaction == wyFalse) 
        if(DumpTableOnLockTables(query, table) == wyFalse)
            return wyFalse;
	
	 if(m_dataonly == wyTrue) 
     {
        if(DumpTableStructure(buffer, db, table) == wyFalse)
            return wyFalse;

         // dumps table data @/IN : char  table / Table name
		if(m_views == wyFalse && DumpTableData(buffer, db, table) == wyFalse) 
            return wyFalse;
     }
	 else if(m_tablestructure == wyFalse)	 
        ret = DumpTableNotOnTableStructure(buffer, table);
	else	
	    ret = DumpTableOnTableStructure(buffer, table);	

	if(ret == wyFalse)
        return wyFalse;
		
	 //if lock table is true and single transaction is enabled then we are not executing lock tables.lock table true and single transaction are mutually exclusive
	 //if single transaction is false then only we are executing lock tables
	if(m_locktables == wyTrue && m_singletransaction == wyFalse)
        if(DumpTableOnLockTables() == wyFalse)
            return wyFalse;

    CHECKSTOP()
		
	if(m_autocommit == wyTrue && (m_iscommit == wyTrue)) 
        buffer->AddSprintf("%sCOMMIT;%s", m_strnewline.GetString(), m_strnewline.GetString());
		 	
	//if(m_flushmaster == wyTrue) 
	//    if(OnFlushMaster() == wyFalse)
 //           return wyFalse;
		
	if( m_individualtablefiles == wyTrue )
	{
    	if(Footer(buffer) == wyFalse)
			return wyFalse;
		
		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}
	
	m_routine(m_lpparam, table, 0, ENDSTART);

	return ret;
}

wyBool
MySQLDump::DumpTableOnLockTables()
{
    wyString    query;
    MYSQL_RES   *res;

    query.SetAs(UNLOCKTABLES);

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

    return wyTrue;
}

wyBool
MySQLDump::OnFlushMaster()
{
    wyString    query;
    MYSQL_RES   *res;
	MYSQL_ROW	row;
	wyString str;
	wyInt32 index_filename;
	query.SetAs("SHOW MASTER STATUS");
	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
	
	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();
	if(res->row_count <=0 )
		return wyTrue;
	
	row= mysql_fetch_row(res);
	index_filename = GetFieldIndex( m_tunnel, res, "File");
	str.SetAs(row[index_filename]);
	mysql_free_result(res);
	query.Clear();
	query.AddSprintf("PURGE BINARY LOGS TO '%s",str.GetString());
	query.Add("'");

	res= SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

	mysql_free_result(res);

    return wyTrue;
}

wyBool
MySQLDump::DumpTableOnTableStructure(wyString * buffer, const wyChar *table)
{
    wyBool ret;

    ret = DumpTableStructure(buffer, m_db.GetString(), table);

    if(ret == wyFalse)
        return wyFalse;
		
    return wyTrue;
}

wyBool
MySQLDump::DumpTableNotOnTableStructure(wyString * buffer, const wyChar *table)
{		
    if(DumpTableStructure(buffer, m_db.GetString(), table) == wyFalse)
        return wyFalse;

    if(m_views == wyFalse)
        if(DumpTableData(buffer, m_db.GetString(), table ) == wyFalse)
	        return wyFalse;
   
    return wyTrue;
}



wyBool
MySQLDump::DumpTableOnLockTables(wyString &query, const wyChar *table)
{
    MYSQL_RES *res;

    query.Sprintf("LOCK TABLES `%s` READ", table);
	
    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

    if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();
    return wyTrue;
}

wyBool
MySQLDump::OnIndividualFiles(wyString * buffer, const wyChar *table, wyInt32 *fileerr)
{
    if(m_dumpalldbs == wyTrue)
	{
		if(OpenIndividualExpFileForEachDb(table) == wyFalse) 
		{
			if(fileerr) 
			{
				*fileerr = 1;
				return wyFalse;
			}
		}
	}
	else
	{
	if(OpenIndividualExpFileForEachTable(table) == wyFalse) 
    {
        if(fileerr) 
        {
	        *fileerr = 1;
	        return wyFalse;
        }
    }
	}
    if(Title(buffer) == wyFalse)
	    return wyFalse;

    if(PrintDBHeaderInfo(buffer) == wyFalse)
	    return wyFalse;

	if(DumpDb(buffer, m_db.GetString()) == wyFalse)
		return wyFalse ;

    return wyTrue;
}

wyBool
MySQLDump::OnFlushLog()
{
    MYSQL_RES   *res;
    wyString    query;

    query.Sprintf("flush logs");
	
    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

    return wyTrue;
}


/*This function initiates dumping of View*/
wyBool
MySQLDump::DumpView(wyString * buffer, const wyChar *db, const wyChar *view, wyInt32 *fileerr)
{
	wyString		query, strview;
	wyBool			ret = wyTrue;
	MYSQL_RES		*res;
	MYSQL_ROW		row;
	wyString		dbname(db);
	wyString pattern("DEFINER=`.*`@`.*`\\sSQL\\sSECURITY");
	 if(m_individualtablefiles == wyTrue )
        if(OnIndividualFiles(buffer, view, fileerr) == wyFalse)
             return wyFalse;
	 
	m_routine(m_lpparam, view, 0, VIEWSTART);
  
	query.Sprintf("show create table `%s`.`%s`", dbname.GetString(), view);

    res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res)
		return OnError();
	
	row	= sja_mysql_fetch_row(m_tunnel, res);

	buffer->AddSprintf("%s/*View structure for view %s */%s", m_strnewline.GetString(), view, m_strnewline.GetString());
	
	buffer->AddSprintf("%s/*!50001 DROP TABLE IF EXISTS `%s` */;", m_strnewline.GetString(), view);
	
	if(m_dropobjects == wyTrue) 
		buffer->AddSprintf("%s/*!50001 DROP VIEW IF EXISTS `%s` */;%s", m_strnewline.GetString(), view, m_strnewline.GetString());
		
	strview.SetAs(row[1]);
	if(m_isremdefiner)
		RemoveDefiner(strview, pattern.GetString(), 12);
	//StripDatabase(strview, dbname.GetString());

	buffer->AddSprintf("%s/*!50001 %s */;%s", m_strnewline.GetString(), strview.GetString(), m_strnewline.GetString());   
	
	sja_mysql_free_result(m_tunnel, res);
	
	if(m_individualtablefiles == wyTrue )
	{	if(Footer(buffer) == wyFalse)
			return wyFalse;

		//Write to file , if data is there in buffer	
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}

    m_routine(m_lpparam, view, 0, ENDSTART);
	return ret;
}

/*This function initiates dumping of stored procedure*/
wyBool
MySQLDump::DumpProcedure(wyString * buffer, const wyChar *db, const wyChar *procedure, wyInt32 *fileerr)
{
	wyBool			ret = wyTrue;
	wyString		dbname(db);
	wyString		strprocedure, strmsg;
	wyString		pattern("DEFINER=`.*`@`.*`\\sPROCEDURE");
	//pattern.AddSprintf("`%s`",procedure);
	if(m_individualtablefiles == wyTrue )
    {
        if(OnIndividualFiles(buffer, procedure, fileerr) == wyFalse)
            return wyFalse;
    }

    m_routine(m_lpparam, procedure, 0, PROCEDURESTART);
  
	if(m_flushlog == wyTrue) 
	{
		if(OnFlushLog() == wyFalse)
			return wyFalse;
	}
   
	//Handling critical section
	CHECKSTOP()

	// Gets the 'create procedure' query. Function gets error message if occured
	ret = GetCreateProcedureString(m_tunnel, &m_mysql, dbname.GetString(), procedure, 
									strprocedure, strmsg);

	if(ret == wyFalse)
	{
		m_error.SetAs(strmsg);
		return wyFalse;
	}
	if(m_isremdefiner)
		RemoveDefiner(strprocedure, pattern.GetString(), 9);
	// writing to the file
	buffer->AddSprintf("%s/* Procedure structure for procedure `%s` */%s", m_strnewline.GetString(), procedure, m_strnewline.GetString());
	
	if(m_dropobjects == wyTrue) 
		buffer->AddSprintf("%s/*!50003 DROP PROCEDURE IF EXISTS  `%s` */;%s", m_strnewline.GetString(), procedure, m_strnewline.GetString());
		
    buffer->AddSprintf("%sDELIMITER $$%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	//StripDatabase(strprocedure, db);

	buffer->AddSprintf("%s/*!50003 ", m_strnewline.GetString());
	
	buffer->AddSprintf("%s */$$", strprocedure.GetString());
	
	buffer->AddSprintf("%sDELIMITER ;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
/*	if(m_flushmaster == wyTrue) 
        OnFlushMaster();*/	

	if( m_individualtablefiles == wyTrue )
	{
		if(Footer(buffer) == wyFalse)
			return wyFalse;
		
		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}

	m_routine(m_lpparam, procedure, 0, ENDSTART);
	return ret;
}

//dumping an event to buffer
wyBool
MySQLDump::DumpEvent(wyString * buffer, const wyChar * db, const wyChar * event,wyBool &seteventschdule, 
					 wyInt32 * fileerr)
{
	wyString		query, strevent;
	 wyBool			ret = wyTrue;
	 MYSQL_RES		*res;
	 MYSQL_ROW		row;
	 wyString		dbname(db);
	 wyInt32        index = 0;
	 wyString	pattern("DEFINER=`.*`@`.*`\\sEVENT");
	 if(m_individualtablefiles == wyTrue )
    {
        if(OnIndividualFiles(buffer, event, fileerr) == wyFalse)
            return wyFalse;
    }

	m_routine(m_lpparam, event, 0, EVENTSTART);
  
	 if(m_flushlog == wyTrue) 
	 {
        if(OnFlushLog() == wyFalse)
			return wyFalse;
	 }
   
	 query.Sprintf("show create event `%s`.`%s`", dbname.GetString(), event);

	CHECKSTOP()

    res = ExecuteQuery(query);

	if(!res)
		return OnError();
	row	= sja_mysql_fetch_row(m_tunnel, res);

	// getting  exact column index for create event field
	index = GetFieldIndex(m_tunnel, res, "Create Event");
	if(index == -1)
		return wyFalse;

	// set global varaible to ON only once other wise eventthread will not initialize
	if(seteventschdule == wyTrue)
	{
		buffer->AddSprintf("%s/*!50106 ", m_strnewline.GetString());
		
		buffer->AddSprintf("set global event_scheduler = 1*/;%s", m_strnewline.GetString());
		
		seteventschdule = wyFalse;

	}

	buffer->AddSprintf("%s/* Event structure for event `%s` */%s", m_strnewline.GetString(), event, m_strnewline.GetString());
	
	// drop event if exist in the importing db
	if(m_dropobjects == wyTrue)
		buffer->AddSprintf("%s/*!50106 DROP EVENT IF EXISTS `%s`*/;%s", m_strnewline.GetString(), event, m_strnewline.GetString());
		
	buffer->AddSprintf("%sDELIMITER $$%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	strevent.SetAs(row[index]);
	if(m_isremdefiner)
		RemoveDefiner(strevent, pattern.GetString(), 5);
	//StripDatabase(strevent, db);
	
	buffer->AddSprintf("%s/*!50106 ", m_strnewline.GetString());
	
	buffer->AddSprintf("%s */$$", strevent.GetString());
	
	buffer->AddSprintf("%sDELIMITER ;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	sja_mysql_free_result(m_tunnel, res);
						
/*	if(m_flushmaster == wyTrue) 
        OnFlushMaster();*/	

	if( m_individualtablefiles == wyTrue )
	{
		if(Footer(buffer) == wyFalse)
			return wyFalse;

		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}

	return ret;

}

/*This function initiates dumping of  stored function */
wyBool
MySQLDump::DumpFunction(wyString * buffer, const wyChar *db, const wyChar *function, wyInt32 *fileerr)
{
	wyString		strfunction, strmsg;
    wyBool			ret = wyTrue;
	wyString		dbname(db);
	wyString pattern("DEFINER=`.*`@`.*`\\sFUNCTION");
	//pattern.AddSprintf("`%s`",function);
	if(m_individualtablefiles == wyTrue )
     {
        if(OnIndividualFiles(buffer, function, fileerr) == wyFalse)
            return wyFalse;
     }
	
	 m_routine(m_lpparam, function, 0, FUNCTIONSTART);
  
	 if(m_flushlog == wyTrue)  
        OnFlushLog();
	   
	//Handling critical section
	CHECKSTOP()

	// Gets the 'create function' query. Function gets error message if occured
	ret = GetCreateFunctionString(m_tunnel, &m_mysql, dbname.GetString(), function, strfunction, strmsg);
	
	if(ret == wyFalse)
	{
		m_error.SetAs(strmsg);
		return wyFalse;
	}
	if(m_isremdefiner)
		RemoveDefiner(strfunction, pattern.GetString(), 8);
	//Adding to buffer
	buffer->AddSprintf("%s/* Function  structure for function  `%s` */%s", m_strnewline.GetString(), function, m_strnewline.GetString());
	
	if(m_dropobjects == wyTrue) 
		buffer->AddSprintf("%s/*!50003 DROP FUNCTION IF EXISTS `%s` */;",m_strnewline.GetString(), function);
		
	buffer->AddSprintf("%sDELIMITER $$%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	//StripDatabase(strfunction, db);

	buffer->AddSprintf("%s/*!50003 ", m_strnewline.GetString());
	
	buffer->AddSprintf("%s */$$", strfunction.GetString());
	
	buffer->AddSprintf("%sDELIMITER ;%s", m_strnewline.GetString(), m_strnewline.GetString());
	
	//if(m_flushmaster == wyTrue) 
	//    OnFlushMaster();

    if( m_individualtablefiles == wyTrue )
	{
		if(Footer(buffer) == wyFalse)
			return wyFalse;

		//Write to file , if data is there in buffer
		if(WriteBufferToFile(buffer, wyTrue) == wyFalse)
			return wyFalse;
	}
	
	m_routine(m_lpparam, function, 0, ENDSTART);
	return ret;
}

/*	This function prints header informatiuon before dumping table data
 *	for each table, information something like comments,disable keys locking and all
 */
wyBool
MySQLDump::TableHeader(wyString * buffer, const wyChar * db, const wyChar * table, wyInt32 fcount, 
					   wyUInt32 rcount, MYSQL_RES * res,wyBool *isvirtual)
{
        wyChar		*tablech = NULL;
		wyInt32		len;
		wyString	temp;

#ifdef _WIN32	

	wyBool	ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	ConvertString	conv;
	
	
	if(ismysql41 == wyFalse)
	{
		len = strlen(table);
		tablech = conv.Utf8toAnsi(table, len);
		if(m_startrow == 0)
			buffer->AddSprintf("%s/*Data for the table `%s` */%s", m_strnewline.GetString(), tablech, m_strnewline.GetString());
			

		if(m_disablekeys == wyTrue) 
			buffer->AddSprintf("%s/*!40000 ALTER TABLE `%s` DISABLE KEYS */;%s", m_strnewline.GetString(), tablech, m_strnewline.GetString());
			
		if(m_locktablewrite == wyTrue)
			buffer->AddSprintf("%sLOCK TABLES `%s` WRITE;%s", m_strnewline.GetString(), tablech, m_strnewline.GetString());
	}
	else
	{
		if(m_startrow == 0)
			buffer->AddSprintf("%s/*Data for the table `%s` */%s", m_strnewline.GetString(), table, m_strnewline.GetString());
			
		if(m_disablekeys == wyTrue) 
			buffer->AddSprintf("%s/*!40000 ALTER TABLE `%s` DISABLE KEYS */;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
			
		if(m_locktablewrite == wyTrue)
			buffer->AddSprintf("%sLOCK TABLES `%s` WRITE;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
	}

#else
        if(m_startrow == 0)
			buffer->AddSprintf("%s/*Data for the table `%s` */%s", m_strnewline.GetString(), table, m_strnewline.GetString());
			
		if(m_disablekeys == wyTrue) 
			buffer->AddSprintf("%s/*!40000 ALTER TABLE `%s` DISABLE KEYS */;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
			
		if(m_locktablewrite == wyTrue)
			buffer->AddSprintf("%sLOCK TABLES `%s` WRITE;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
			
#endif

    if(m_autocommit == wyTrue) 
	{
		buffer->AddSprintf("%sset autocommit=0;%s", m_strnewline.GetString(), m_strnewline.GetString());
		
		m_iscommit = wyTrue;
	}
	if(m_bulkinsert == wyTrue)
	{   
		if(rcount == 0)
            buffer->Add("");
		else
		{			       
			if(m_completeinsert == wyFalse) 
			{
#ifdef _WIN32
				if(ismysql41 == wyFalse)
					m_insertcount = temp.Sprintf("%sinsert %s into `%s` values ", 
														m_strnewline.GetString(), m_delayed.GetString(), tablech);
				else
					m_insertcount = temp.Sprintf("%sinsert %s into `%s` values ", 
									m_strnewline.GetString(), m_delayed.GetString(), table);
#else
                m_insertcount = temp.Sprintf("%sinsert %s into `%s` values ", 
								m_strnewline.GetString(), m_delayed.GetString(), table);
#endif
				buffer->Add(temp.GetString());
			
			}
			else				
				CompleteInsert(buffer, db, res, table, fcount,isvirtual);
		}
	} 

	return wyTrue;
}

/*  This function checks for the 16 mb query, if query size 
    exceeds maximam allowd size	it inserts one more insert statement */									
wyBool
MySQLDump::CheckQueryLength(wyString * buffer, const wyChar * db, const wyChar * table, 
							MYSQL_RES *res, wyInt32 fieldcount,wyBool *isvirtual)
{	
	/* get size of all fields,and add to get the total length it should not be more than 
	 * MAX_ALLOWED_LENGTH if ecxceeds we will insert one more insert statement */ 

	wyUInt32    lenlastrow = 0;
	wyULong		*fieldlengths;
    wyInt32     count; 
	wyChar		*tablech = NULL;
	wyInt32		len;
	wyString    temp;
	wyInt32		bulklimit = 0;
	
#ifdef _WIN32
	wyBool		ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
	
	ConvertString	conv;
#endif

	//if user defined size is less than the server default then we are setting user defined value otherwise max_allowed_length is server default
    if(m_bulksize && m_bulksize < m_maxallowedsize)
        m_maxallowedsize = m_bulksize;

	//max bulk insert size is restircting to 16 MB
	bulklimit = min(m_maxallowedsize *1024, BULK_SIZE);
		
    fieldlengths = sja_mysql_fetch_lengths(m_tunnel, res);

	if(fieldlengths == 0)
		 return wyFalse;	

	for(count = 0; count < fieldcount; count++)
		lenlastrow += fieldlengths[count];

	lenlastrow += (fieldcount * 3 + 3);//(fieldcount * 3 + 2);  // bac-ticks, comma, cloase brackets 
			
	m_querylength = m_querylength + lenlastrow;

	if(m_completeinsert == wyTrue)
    {
		m_querylength = m_querylength + (m_completecount ? m_insertcountcomplete : 0);
		m_completecount = wyFalse;
	}	 

	//get query length 
    if((m_querylength + m_insertcount >= bulklimit) &&	m_basicquery == wyFalse)
	{			
		 m_querylength = 0;
		 m_insertcount  = 0;

		//if query length is more than maxallowedsize, then start new insert statement
		 if(m_bulkinsert == wyTrue)
		{			
			if(m_completeinsert == wyTrue)
			{
				m_insertcount = m_insertcount + 1;
				buffer->Add(";");
				
				//inserts field names in to insert statement, in case complete insert option is wyTrue.
				CompleteInsert(buffer, db, res, table, fieldcount, isvirtual);
			} 
            else 				
			{
#ifdef _WIN32
				/*increment m_insertcount count with number of bytes being added to buffer.
				A temp buffer is used to get the count and then data is added to buffer.*/
				if(ismysql41 == wyFalse)
				{
					len = strlen(table);
					tablech = conv.Utf8toAnsi(table, len);
					
					m_insertcount = m_insertcount +	temp.Sprintf(";%sinsert %s into `%s` values ", 
                                    m_strnewline.GetString(), m_delayed.GetString(), tablech);
				}
				else
					m_insertcount = m_insertcount +	temp.Sprintf(";%sinsert %s into `%s` values ", 
									m_strnewline.GetString(), m_delayed.GetString(), table);
					
				//Add to buffer
				buffer->Add(temp.GetString());

#else
	            m_insertcount = m_insertcount +	temp.Sprintf(";%sinsert %s into `%s` values ", 
								m_strnewline.GetString(), m_delayed.GetString(), table);
				
				buffer->Add(temp.GetString());
				
#endif
			}
		
			//size of the last row
			if(lenlastrow)
				m_querylength = lenlastrow;

			m_querylength += m_insertcountcomplete; //Length of insert 'into ....'
						
			m_startpose = 1;
			m_inquery = wyTrue;
		}
	}
	return wyTrue;
}

/*	This function  prints each and every field value to the expoert file
 *	in order ti dump table data */ 											
wyBool
MySQLDump::PrintFieldValue(wyString * buffer, MYSQL_RES *res, MYSQL_ROW row, wyInt32 fcount, wyBool *isvirtual)
{
	wyInt32     *lengths;
	wyInt32     to, count, firstcol =0;
	MYSQL_FIELD	*field;
	int isBlob, i;
	wyString	blob_temp;
	wyString	hex_tmp;
	wyString	hex_data;
	wyUInt32      hex_datalen;
	lengths = (wyInt32 *)sja_mysql_fetch_lengths(m_tunnel, res);

	for(count = 0; count < fcount; count++)		
	{  
		field = sja_mysql_fetch_field_direct(m_tunnel, res, count);

		if(isvirtual[count]==1)
		{
		continue;
		}

		if(firstcol>0){
		buffer->Add(",");
		}
		/* if its NULL then just write NULL */
		if(!row[count]) 
			buffer->Add("NULL"); 
			
		else if(!IS_NUM_FIELD(field)) 
        {
			if(m_tunnel->IsTunnel())
				isBlob= (field->type == MYSQL_TYPE_BIT ||
						   field->type == MYSQL_TYPE_STRING ||
						   field->type == MYSQL_TYPE_VAR_STRING ||
						   field->type == MYSQL_TYPE_VARCHAR ||
						   field->type == MYSQL_TYPE_BLOB ||
						   field->type == MYSQL_TYPE_LONG_BLOB ||
						   field->type == MYSQL_TYPE_MEDIUM_BLOB ||
						   field->type == MYSQL_TYPE_TINY_BLOB) ? 1 : 0;
			else
						isBlob= (field->charsetnr == 63 &&
						  (field->type == MYSQL_TYPE_BIT ||
						   field->type == MYSQL_TYPE_STRING ||
						   field->type == MYSQL_TYPE_VAR_STRING ||
						   field->type == MYSQL_TYPE_VARCHAR ||
						   field->type == MYSQL_TYPE_BLOB ||
						   field->type == MYSQL_TYPE_LONG_BLOB ||
						   field->type == MYSQL_TYPE_MEDIUM_BLOB ||
						   field->type == MYSQL_TYPE_TINY_BLOB)) ? 1 : 0;
		
		if(m_sethexblob && isBlob)
		{
				hex_datalen=lengths[count];
				
				if(hex_datalen==0)
				{
					buffer->AddSprintf("''");
					firstcol++;
					continue;
				}

				buffer->AddSprintf("0x");
				hex_tmp.SetAs("");

				const char *ptr= row[count];
				
				for (i = 0; i < hex_datalen; i++) 
				{
					//MM: Test case?
					if((i+3) < hex_datalen) {
						if (*((unsigned char *)(ptr + i)) == '\\' && 
							*((unsigned char *)(ptr + i + 1)) =='r' &&
							*((unsigned char *)(ptr + i + 2)) =='\\' && 
							*((unsigned char *)(ptr + i + 3)) == 'n')
						{
							hex_tmp.Add("0D0A");
							i += 3;
							continue;
						}
					}

					hex_tmp.AddSprintf("%02x", *((unsigned char *)(ptr+i)));
				}
				buffer->AddSprintf(hex_tmp.GetString());
				firstcol ++;
				continue;
			}
			/*check wether the fieild value is a string 
			if it is string make it quoted */

            if(m_escapeddatalen < ((lengths[count] * 2) + 1))
            {
                /* need to realloc */
				if(m_escapeddata)
					free(m_escapeddata);

				m_escapeddatalen = ((lengths[count] * 2) + 1);
				m_escapeddata = (wyChar*)calloc(sizeof(wyChar), m_escapeddatalen);
			} 
            else if(!m_escapeddata ) 
            {
				m_escapeddatalen = ((lengths[count] * 2) + 1);
				m_escapeddata = (wyChar*)calloc(sizeof(wyChar), m_escapeddatalen);
			
			}

			to = sja_mysql_real_escape_string(m_tunnel, m_mysql, m_escapeddata, row[count], lengths[count]);

			m_escapeddata[to] = 0;
			buffer->AddSprintf("'%s'", m_escapeddata);
		} 
        else 
			buffer->AddSprintf("%s", row[count]);
			
	    firstcol ++;
	} 	
	
	sja_mysql_field_seek(m_tunnel, res , 0);
	
	return wyTrue;
}

/*  This function addes foorter informatoion after dumping table data 
 *	something like unlocking,and enable keys and all.
 */
wyBool
MySQLDump::TableFooter(wyString * buffer, MYSQL_ROW row , const wyChar *db, const wyChar *table)
{

	if(row)
		buffer->Add(")");
		
	if(m_rowcount)
		buffer->AddSprintf(";%s", m_strnewline.GetString());
		
	if(m_locktablewrite == wyTrue)  
		buffer->AddSprintf("%s%s;%s", m_strnewline.GetString(), UNLOCKTABLES, m_strnewline.GetString());
		
	if(m_disablekeys == wyTrue)
		buffer->AddSprintf("%s/*!40000 ALTER TABLE  `%s` ENABLE KEYS */;%s", m_strnewline.GetString(), table, m_strnewline.GetString());
	
	return wyTrue;
}

/*This function gets all primary key clumns from table and prepares pkstring, which can be
								used while exectuteing select tatement with order by pk cloumns*/
wyBool
MySQLDump::GetPrimaryKeyCols(const wyChar *db , const wyChar *table)
{
	wyString					 pkquery;
	MYSQL_RES					*res    	= NULL;
	MYSQL_ROW					 row    	= NULL;
		
	/* Get all Indexes or keys , and while fetching each row check for 3rd  column for 
		Primary key, if it is primary key then add the 5th column  of each row to the
		meber function m_pkstring and form primary key string with all colums */

	pkquery.Sprintf("show keys from `%s`.`%s`", db, table);

    res = ExecuteQuery(pkquery);
	if(!res)
		return wyFalse;

	while((row = sja_mysql_fetch_row(m_tunnel, res)) != NULL)
	{ 
		if(stricmp(row[2],"primary") == 0)
		{
			if(m_pkstring.GetLength() != 0)
				m_pkstring.Add(",");
		
			 m_pkstring.AddSprintf("`%s`", row[4]);
		}
	}
	
	sja_mysql_free_result(m_tunnel, res);
    return wyTrue;
}

    /*
    * MySQLDump::DumpTableData() This function fetches the data from from table
    * and outputs to the dump file(m_exprt File);
    * @ param char * table	/ Name of the table to be dumped;
    * @ param char * db	/ Name of the Data base to be dumped
    */
wyBool
MySQLDump::DumpTableData(wyString * buffer, const wyChar *db, const wyChar *table)
{					
	wyInt32     rcount = 0;		//row count
	wyString    query, strengine;
	MYSQL_ROW   row = NULL;
		
    m_inquery = wyFalse;
	m_basicquery = wyTrue;

	//to get table engine
	GetTableEngine(m_tunnel, m_mysql, table, db, &strengine);

	//We are not doing inserts for merge table and federated tables 
	if((strengine.CompareI("MRG_MyISAM") == 0) || (strengine.CompareI("FEDERATED") == 0))
		return wyTrue;

	
    if(m_optdelayed == wyTrue) 
        m_delayed.SetAs("DELAYED ");
    else 
        m_delayed.SetAs("");
      
	rcount = GetRowCount(db, table);
    m_rowcount = rcount;

    m_routine(m_lpparam, table, m_rowcount, TABLEROWS);

	if(GetPrimaryKeyCols(db , table) == wyFalse)
		return wyFalse;
			
    if(DumpTableDataRows(buffer, &row, db, table, rcount) == wyFalse)
        return wyFalse;

	m_pkstring.Clear();
    
    m_startrow = 0;
	m_endrow   = 0;

	if(TableFooter(buffer, row, db, table) == wyFalse)
		return wyFalse;

	return wyTrue;
}

wyBool
MySQLDump::DumpTableDataRows(wyString * buffer, MYSQL_ROW *row, const wyChar *db, const wyChar *table, 
                                 wyInt32 rcount)
{	
    MYSQL_RES   *res = NULL;
    wyInt32     fcount, no_rows, l=0;	
    wyString    query, virt_query;
    wyBool      intable = wyFalse;
    
	MYSQL_RES   *virt_res = NULL;
	MYSQL_ROW	virt_row;
	wyBool *isvirtual = NULL;
	
	virt_query.Sprintf("SHOW FIELDS FROM `%s`.`%s`",db, table);

	virt_res = SjaExecuteAndGetResult(m_tunnel, 
        &m_mysql, virt_query);

	if(!virt_res  || sja_mysql_num_rows(m_tunnel, virt_res) == -1) 
			return OnError();

		no_rows =  mysql_num_rows(virt_res);

		if(isvirtual != NULL)
		{
		free(isvirtual);
		isvirtual = NULL;
		}
		isvirtual = (wyBool *)calloc(no_rows,sizeof(wyBool));
		
		virt_row = sja_mysql_fetch_row(m_tunnel, virt_res);
		while(virt_row)
		{
			if(strstr(virt_row[5], "VIRTUAL") || strstr(virt_row[5], "PERSISTENT") || strstr(virt_row[5], "STORED"))
			{
				isvirtual[l++] = wyTrue;
				m_completeinsert = wyTrue;
			}
			else
			{
				isvirtual[l++]= wyFalse;
			}
			virt_row = sja_mysql_fetch_row(m_tunnel, virt_res);
		}
		
		sja_mysql_free_result(m_tunnel, virt_res);
		
		//-------------------------------isvirtual[] filled end -----------------------------------------//
	

    do{
		m_startrow = m_endrow;
		m_endrow += m_chunklimit;

	    query.Sprintf("select * from `%s`.`%s`", db, table);
				
		// Appending lilited row statement at the end of the query while using tunneleing
		//if http, then we will break data into chunks.
		//if Direct and user defined the chunk size then also we will break data into chunks.
		if(m_rowcount > (wyUInt32)m_chunklimit && (m_tunnel->IsTunnel() || (!m_tunnel->IsTunnel() && 
			(m_chunkinsert == wyTrue && m_chunklimit != 0))))
        {
            if(m_pkstring.GetLength())
			    query.AddSprintf(" order by %s ", m_pkstring.GetString());

			query.AddSprintf("limit %d,%d ", m_startrow, m_chunklimit);
        }
			
		CHECKSTOP()

        res =  SjaExecuteAndUseResult(m_tunnel, m_mysql, query);

        if(!res)
            return OnError();

		fcount = sja_mysql_num_fields(m_tunnel, res);	//counts number of fields

		if(m_startrow == wyFalse && TableHeader(buffer, db, table, fcount, rcount ,res,isvirtual) == wyFalse)
			return wyFalse;

        DumpTableDataAllRows(buffer, row, res, db, table, &fcount, rcount, intable, isvirtual);
								
		sja_mysql_free_result(m_tunnel, res);
	
        intable = wyTrue;

	}

	/* while loop to print total tables with limited row query, while tunneling , 
	and Direct and user defined the chunk size*/ 
	while( m_endrow < m_rowcount && (m_tunnel->IsTunnel() || (!m_tunnel->IsTunnel() && 
			(m_chunkinsert == wyTrue && m_chunklimit != 0))));

	if(isvirtual != NULL)
		{
		free(isvirtual);
		isvirtual = NULL;
		}
    return wyTrue;
}

wyBool
MySQLDump::DumpTableDataAllRows(wyString * buffer, MYSQL_ROW *row, MYSQL_RES *res, const wyChar *db, 
								const wyChar *table, wyInt32 *fcount, wyInt32 rcount, wyBool intable, wyBool *isvirtual)
{
    wyUInt32		startpos = 0;
	wyString        temp;

    if(intable == wyFalse)
        m_startpose = 1;

	while(*row = sja_mysql_fetch_row(m_tunnel, res))
	{ 	
        if(IsExportingStopped() == wyTrue)
            break;
		
		if(CheckQueryLength(buffer, db, table, res, *fcount, isvirtual) == wyFalse)
			return wyFalse;

		startpos++;
	
		/*Increment insert count with number of bytes being added to buffer.
		  A temp buffer is used to get the count and then data is added to buffer.*/
		if(m_bulkinsert == wyTrue)
		{ 
			if(m_startpose == 1)
            {	
				if(m_rowperline==wyTrue)
				m_insertcount = m_insertcount + temp.SetAs("\r\n(");

				else
				m_insertcount =  m_insertcount + temp.SetAs("(");

				m_startpose = 0;
			}
			else
			{
				if(m_rowperline==wyTrue)
				m_insertcount = m_insertcount + temp.SetAs(",\r\n(");

				else
				m_insertcount = m_insertcount + temp.SetAs(",(");

			}
			//Add to buffer from temp
			buffer->Add(temp.GetString());
		}
		else 
        {
			if(m_inquery == wyFalse)
			{
				if(m_completeinsert == wyFalse)
				{
					m_insertcount =  m_insertcount + temp.Sprintf("%sinsert %s into `%s` values(", 
                                     m_strnewline.GetString(), m_delayed.GetString(), table);
					
					//Add to buffer from temp
					buffer->Add(temp.GetString());
				}
				else
			 		CompleteInsert(buffer, db, res, table, *fcount,isvirtual);

				m_basicquery = wyTrue;
			}
		}
					
		if(PrintFieldValue(buffer, res, *row, *fcount,isvirtual) == wyFalse)
			return wyFalse;
		
		if(startpos == (wyUInt32)rcount)
			break;
		
		if(m_bulkinsert == wyTrue)
			m_insertcount = m_insertcount + temp.SetAs(")");
		else
		{
			m_insertcount = m_insertcount + temp.SetAs(");");
			m_inquery = wyFalse;
		}

		//Add to buffer from temp
		buffer->Add(temp.GetString());

		m_basicquery = wyFalse;		

        if((m_startrow + startpos) % 100 == 0)
            m_routine(m_lpparam, table, m_startrow + startpos, TABLEEXPORT);

		//If size of buffer is more than 8K, write to file 
		if(WriteBufferToFile(buffer) == wyFalse)
			return wyFalse;
	}

    return wyTrue;
}

/* This function inserts field names in to insert statement, in case complete insert option 
   is wyTrue.*/

wyBool
MySQLDump::CompleteInsert(wyString * buffer, const wyChar *db, MYSQL_RES *res ,const wyChar *table , wyInt32 fcount, wyBool*isvirtual)
{	
	MYSQL_FIELD	*field = NULL;
	wyInt32     startpos = 0, firstcol = 0;
	wyChar		*tablech = NULL;
	wyString    temp;

#ifdef _WIN32   
    ConvertString	conv;
	wyBool		ismysql41 = IsMySQL41(m_tunnel, &m_mysql);

	/*increment completeinsert count with number of bytes being added to buffer.
	  A temp buffer is used to get the count and then data is added to buffer.*/
	if(ismysql41 == wyFalse)
	{
		wyInt32	len = strlen(table);

		tablech = conv.Utf8toAnsi(table, len);
		
        m_insertcountcomplete = temp.Sprintf("%sinsert %s into `%s`(", m_strnewline.GetString(), m_delayed.GetString(), tablech);
	}
	else
		m_insertcountcomplete = temp.Sprintf("%sinsert %s into `%s`(", m_strnewline.GetString(), m_delayed.GetString(), table);
		
	//Add temp to buffer
	buffer->Add(temp.GetString());

#else
	m_insertcountcomplete = temp.Sprintf( "%sinsert %s into `%s`(", m_strnewline.GetString(), m_delayed.GetString(), table);
	buffer->Add(temp.GetString());
#endif

	while(field = sja_mysql_fetch_field(m_tunnel, res))
	{ 
		if(!field->name || isvirtual[startpos++]==1)
		{
		continue;
		}

		if(firstcol>0){
		buffer->Add(",");
		}

		//Back quotes with field names
		m_insertcountcomplete = m_insertcountcomplete + temp.Sprintf("`%s`", field->name);
	    
		buffer->Add(temp.GetString());

		firstcol++;
	}

	m_insertcountcomplete = m_insertcountcomplete + temp.SetAs( ") values ") +(fcount -1);
	
	//Add temp to buffer
	buffer->Add(temp.GetString());
	
	if(m_bulkinsert == wyFalse)
		buffer->Add("(");
		
	sja_mysql_field_seek(m_tunnel, res , 0);

	 m_querylength = 0;
	 m_insertcount  = 0;

    return wyTrue;
}

/* retrive filename from the full path like "c:\my docu..." */
wyChar *
MySQLDump::GetFileNameFromPath(wyChar * path)
{
	wyInt32		pathLen = strlen(path);
	wyInt32		index;
	wyChar	    delimiter;
/* The seperator can different on different OS */
#ifdef _WIN32 
	delimiter = '\\';
#else
	delimiter = '/';
#endif

	for(index = pathLen - 1; index >= 0; index--)
	{
		if(path[index] == delimiter)
			break;
	}

	index++;

	return(path + index);
}

/* Create proper file name if timestamp is selected*/
void 
MySQLDump::GetFormattedFilePath(wyString &path, wyBool iswithtimestamp)
{
	wyInt32 	index, pathlen;
	wyChar      delimiter;
	wyString    filename;
	wyString    filepath;
    wyString    timestring;
	
#ifdef _WIN32
	delimiter = '\\';
#else
	delimiter = '/';
#endif

    pathlen = path.GetLength();

	for(index = pathlen - 1; index >= 0; index--)
	{
		if(path.GetCharAt(index) == delimiter)
			break;
	}
    index++;

	if(pathlen <= index)
		return;

    filename.SetAs(path.Substr(index, pathlen - index));
    path.Strip(pathlen - index);
    
	if(iswithtimestamp == wyTrue)
	{
		GetTimeString(timestring, "-");
		path.AddSprintf("%s_%s", timestring.GetString(), filename.GetString());
	}

	else
		path.SetAs(filename);

    return;
}

#ifdef _WIN32
void            
MySQLDump::SetCriticalSection(CRITICAL_SECTION * cs)
{
    m_cs = cs;
}
#endif

void 
MySQLDump::StopExporting()
{
#ifndef _CONSOLE
    // Request ownership of the critical section.
    EnterCriticalSection(m_cs);
#endif
    // Access the shared resource.

    *m_stopexport = wyTrue;

#ifndef _CONSOLE
     // Release ownership of the critical section.
    LeaveCriticalSection(m_cs);
#endif
}

wyBool  
MySQLDump::IsExportingStopped()
{
    wyInt32  ret;
#ifndef _CONSOLE
    // Request ownership of the critical section.
    EnterCriticalSection(m_cs);
#endif
    // Access the shared resource.

    ret = (*m_stopexport);

#ifndef _CONSOLE
     // Release ownership of the critical section.
    LeaveCriticalSection(m_cs);
#endif

    return (ret)?wyTrue:wyFalse;
}


MYSQL_RES *
MySQLDump::ExecuteQuery(wyString& query)
{
    MYSQL_RES *result;

    result = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

    return result;
}


wyBool
MySQLDump::DumpDb(wyString * buffer, const wyChar *db)
{
	wyBool      ismysql41 = IsMySQL41(m_tunnel, &m_mysql);
    wyString    dbname;
	MYSQL_RES	*result = NULL;
	MYSQL_ROW	row = NULL;
	wyString 	charset, createdb, query;
	wyString    strtemp, charsetcollate;
		
	if(!db)
		return wyFalse;
	
	if(m_dumpalldbs == wyTrue)
        buffer->AddSprintf("%s/*Database structure for database `%s` */%s", m_strnewline.GetString(), db, m_strnewline.GetString());
		
	if(m_createdatabase == wyTrue)
    {
        if(ismysql41 == wyTrue)
		{				
			query.Sprintf("SHOW CREATE DATABASE `%s`;", db);
			
			CHECKSTOP()

			result = ExecuteQuery(query);
			if(!result)
			{
				OnError();
			
#if !defined _WIN32 || defined _CONSOLE 	  
				PRINTERROR(m_error.GetString(), logfilehandle); //printing error
				AppendErrMessage(m_error.GetString(), m_error.GetString()); //adding error to mail message
				m_iserr = wyTrue; //if an error occured iserr= wyfalse 		   
#endif
				return wyFalse;				
			}			

			row = sja_mysql_fetch_row(m_tunnel, result);
			//SHOW CREATE DATABASE statement
			if(row)
				charset.Sprintf("%s", row[1]);

			if(result)
				sja_mysql_free_result(m_tunnel, result);

			//Replace the CREATE DATABASE <db> with CREATE DATABASE <db> /*!32312 IF NOT EXISTS*/
			strtemp.Sprintf("CREATE DATABASE `%s`", db);
			
			if(strtemp.GetLength() >= charset.GetLength())
				return wyFalse;

			charsetcollate.SetAs(charset.Substr(strtemp.GetLength(), charset.GetLength() - strtemp.GetLength()));		
				
			createdb.Sprintf("CREATE DATABASE /*!32312 IF NOT EXISTS*/`%s`%s;", db, charsetcollate.GetString());
			
			buffer->AddSprintf("%s%s%s", m_strnewline.GetString(), createdb.GetString(), m_strnewline.GetString());  		    
		}
        else
        {
            dbname.SetAs(db);
            
			buffer->AddSprintf("%sCREATE DATABASE /*!32312 IF NOT EXISTS*/ `%s`;%s", m_strnewline.GetString(), dbname.GetAsAnsi(), m_strnewline.GetString());    
		}
    }

	if(m_usedb == wyTrue)
    {
        if(ismysql41 == wyTrue)
		   buffer->AddSprintf("%sUSE `%s`;%s", m_strnewline.GetString(), db, m_strnewline.GetString());
		else 
        {
           dbname.SetAs(db);
           buffer->AddSprintf("%sUSE `%s`;%s", m_strnewline.GetString(), dbname.GetAsAnsi(), m_strnewline.GetString());
		}
    }
	return wyTrue ;
}


/* This function Opens Individual Text File for each db */
wyBool
MySQLDump::OpenIndividualExpFileForEachDb(const wyChar *db)
{
	wyString    filenameBuff;
    wyString    timeString ;    

	if(!db)
	{
		return wyFalse;
	}


	GetTimeString(timeString, "-");

	if(m_compress == wyFalse)
	{
		if(m_filemodetimestamp == wyTrue)
			filenameBuff.Sprintf("%s%s_db_%s.sql", m_expfilepath.GetString(), timeString.GetString(), db);
		else
			filenameBuff.Sprintf("%sdb_%s.sql", m_expfilepath.GetString(), db);
	
#ifdef _WIN32
	// opens individual text file , and each opend file will be closed in function Footer()

		if(m_expfile)
		{
			fclose(m_expfile);
			m_expfile = NULL;
		}
		m_expfile = _wfopen(filenameBuff.GetAsWideChar(), m_filemodeappend ? TEXT(APPEND_MODE) : TEXT(WRITE_MODE)) ;
#else
		m_expfile = fopen(filenameBuff.GetString(), m_filemodeappend ? APPEND_MODE : WRITE_MODE) ;
#endif

		if(!m_expfile) 
			return wyFalse;
	}

#if !defined _WIN32 || defined _CONSOLE 
	else 
	{
		filenameBuff.Sprintf("db_%s.sql", db);

		if(m_zip.OpenFileInZip(filenameBuff.GetAsWideChar(), 0)!= ZIP_OK)
			return wyFalse;
	}
#endif		
	
	return wyTrue;
}

/*Functions write data from buffer to file
isforce == wyTrue when force writing to file.
*/
wyBool      
MySQLDump::WriteBufferToFile(wyString * buffer, wyBool  isforce)
{
	wyInt32 ret;
    /*
    buffer->FindAndReplace("\r\n","\n"); added to solve community issue 2128
    0xOD is additionally attached at every end of lines.
    Reason : fprintf handles \n as \r\n when working in windows and simply as \n when in UNIX
    hence \r\n will change to \r\r\n
    */

    buffer->FindAndReplace("\r\n","\n");
	//If buffer size is more than 8K or dumping end has reached , write data to file
	if((isforce == wyTrue && buffer->GetLength()) || buffer->GetLength()  >= SIZE_8K)
	{
		if(m_compress == wyFalse)
		{
			ret = fprintf(m_expfile, "%s", buffer->GetString());

			//If error occures in writing the file
			if(ret < 0)
			{
#ifdef WIN32  
				DisplayErrorText(GetLastError(), _("Error writing in file."));
#endif
				return wyFalse;
			}
		}

#if !defined _WIN32 || defined _CONSOLE 
		else 
		{
			if(m_zip.WriteToFile((void *)buffer->GetString(),buffer->GetLength())!= ZIP_OK)
				return wyFalse;
		}
#endif
		
		//Flushing buffer for next write
		buffer->Clear();
	}
	return wyTrue;
}

wyBool
MySQLDump::OnSingleTransaction()
{
    wyString    query;
    MYSQL_RES   *res;

    query.SetAs("SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

	 sja_mysql_free_result(m_tunnel, res);

	query.SetAs("START TRANSACTION /*!40100 WITH CONSISTENT SNAPSHOT */");

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

	sja_mysql_free_result(m_tunnel, res);

	query.SetAs("UNLOCK TABLES");

	res = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);

	if(!res && sja_mysql_affected_rows(m_tunnel, m_mysql) == -1)
		return OnError();

	 sja_mysql_free_result(m_tunnel, res);

    return wyTrue;
}