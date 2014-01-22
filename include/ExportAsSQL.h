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


#include "Datatype.h"
#include "wyString.h"
#include "Tunnel.h"
#include "CommonHelper.h"

#if !defined _WIN32 || defined _CONSOLE 
//compress backup
#include "wyZip.h"
#endif

#if defined WIN32 && ! defined _CONSOLE
#include "Global.h"
#else
#include "CommonJobStructures.h"
#include "TunnelAuth.h"
#endif

#ifndef __YOG_EXPORTBATCH__
#define __YOG_EXPORTBATCH__

#define		MAX_OBJECT_NAME			128

//#ifndef _WIN32
  //  #define	 MAX_PATH				260
//#endif

#define		MAX_STRING_LEN			1024		
#define		MAX_QUERY_LEN			1024
#define		REFRESH_LOG				2
#define		MAX_ALLOWED_LENGTH		16*1024*1024
#define		SEPS					";,"
#define		APPEND_MODE				"a"
#define		WRITE_MODE				"w"		


/* query constants */
#define		UNLOCKTABLES				"UNLOCK TABLES"		

/*******************************************************************************
   ** Description**

   This class dumps tables and databases from MySQl server
   in to text a file specified by user, user can revert it back when needed.
   Compatible on both Windows and Linux platforms.
   It dumps data with bulk insert statements and also complete insert 
   statements;user can turn on of or turn of these options.
   This class Opens connection with MySQlL server,issues mysql queries to 
   fetch data from tables using result sets,from result sets it will print
   This class handles 16 MB queries properly, by inserting more insert
   statements if the query length is grater than max allowed length. 
   data to a file,specified bye the user.
   it can dump either single table,or multiple tables 
   and either single database or multiple databases.

********************************************************************************/

/**
   * An enum.
   * It is used to give different export errors
	
    SYSTEMERROR         = 0,
	GUIROUTINEERROR,    = 1,
	MYSQLERROR,         = 2,
	FILEERROR,          = 3,
    EXPORTSTOPPED,      = 4,
	SUCCESS             = 5

    */
typedef	enum __export_errors
{
	SYSTEMERROR=0,
	GUIROUTINEERROR,
	MYSQLERROR,
	FILEERROR,
	EXPORTSTOPPED,
	SUCCESS
} EXPORTERROR;

/* options to store extra info about jobs */


typedef struct __schdextra_options
{
	wyBool	  m_bulkinsert;
	int		  m_bulksize;
	wyBool	  m_chunkinsert;
	int		  m_chunklimit;
	wyBool	  m_utf8charset;
	wyBool	  m_droptable;
	wyBool	  m_createview;
	wyBool	  m_createprocedure;
	wyBool	  m_createfunction;
	wyBool	  m_createtrigger;
	wyBool	  m_dropobjects;
	wyBool	  m_dropview;
	wyBool	  m_dropfunction;
	wyBool	  m_dropprocedure;
	wyBool	  m_droptrigger;
	wyBool    m_dropevent;
	wyBool	  m_createdb;
	wyBool	  m_insertdelayed;
	wyBool	  m_disablekeys;
	wyBool	  m_flushlogs;
	wyBool	  m_flushmaster;
	wyBool	  m_locktableread;
	wyBool	  m_flushslave;
	wyBool	  m_autocommit;
	wyBool	  m_locktablewrite;
	wyBool	  m_singletransaction;  //single transaction
	wyBool	  m_struconly;
	wyBool	  m_completeinsert;
	wyBool	  m_usedb;
	wyBool    m_fkchecks;
	wyBool	  m_appendtofile;
	wyString  m_charset;
} SCHDEXTRAOPTIONS;

/**
   * An enum.
   * It is used to give different export states
	
    TABLESTART          = 1,
	VIEWSTART           = 2,
	PROCEDURESTART      = 3,
	FUNCTIONSTART       = 4,
	TRIGGERSTART        = 5,
	ENDSTART            = 6
    TABLEEXPORT         = 7,
    TABLEROWS           = 8,
	SETOBJECTCOUNT		= 9,
	FETCHDATA			= 10
    */
typedef enum __export_state
{
	DATABASESTART,
	TABLESTART,
	VIEWSTART,
	PROCEDURESTART,
	FUNCTIONSTART,	
	TRIGGERSTART,
	EVENTSTART,
	ENDSTART,
    TABLEEXPORT,
    TABLEROWS,
	SETOBJECTCOUNT,
	FETCHDATA
} EXPORTTABLESTATE;

/* Callback function to display any GUI related stuff.
   this is always called when we are reading a file */

typedef void(*gui_export_update_routin)(
    void *lpParam, const wyChar *table, wyUInt32 rows, EXPORTTABLESTATE state
   );
typedef gui_export_update_routin LPGUI_EXPORT_UPDATE_ROUTINE;

class MySQLDump
{
private:

    /// Port number.
	wyInt32				m_port;

    /// Max size of chunk.
	wyInt32				m_chunklimit;

    /// MAx size of bulk
	wyUInt32				m_bulksize;

    /// Stop export
	wyInt32             *m_stopexport;

    /// DELAYED string 
	wyString           	m_delayed;

    /// Number of insert to be dome 
	wyUInt32	    	m_insertcount;

    /// Start row
	wyUInt32	    	m_startrow;

    /// End row
	wyUInt32	    	m_endrow;

    /// Complete insert format ?
	wyUInt32	    	m_insertcountcomplete;

    /// Length of the query
	wyUInt32            m_querylength;

    /// No of rows
	wyUInt32            m_rowcount;

    /// Length of the escape char
	wyUInt32            m_escapeddatalen;

    /// Is SSL checked ????
    wyBool              m_issslchecked;

	///Is SSH checked
	wyBool				m_issshchecked;

	
    /// Is SSL Authentication checked ????
    wyBool              m_issslauthchecked;

    /// Client key file path
    wyString            m_clikey;

    /// Client certificate file path;
    wyString            m_clicert;

    /// Client CA certificate file path
    wyString            m_cacert;

    /// Cipher to use
    wyString            m_cipher;

    /// Connection to the database when the WIN32 is defined
    /**
	@returns wyTrue on success else wyFalse
    */
    wyBool              ConnectToDataBaseWin32Def();

    /// Connection to the database next to win32 definition
    /**
	@returns wyTrue
    */
    wyBool              ConnectToDataBaseNext();

    ///Starts the databases dumping process
    /**
	@param buffer	 : IN/OUT String
    */
    wyBool              DumpDatabaseStart(wyString * buffer);

    /// Dumps database with read lock
	/**
	@returns wyTrue
	*/
    wyBool              DumpDatabaseOnMasterSlave();

	/// Dumps database with setting the utf8 option 
	/**
	@returns wyTrue
	*/
    wyBool              DumpDatabaseOnUtf8();

	/// Changes the current database
	/**
	@param db			: IN Database name to change to 
	@returns wyTrue
	*/
    wyBool              ChangeContextDatabase(const wyChar *db);

	/// Create a dummy table for the view. i.e.. a table  which has the
    /// same columns as the view should have. This table is dropped
    /// just before the view is created. The table is used to handle the
    /// case where a view references another view, which hasn't yet been
    /// created(during the load of the dump). 
    /**
	@param buffer	 : IN/OUT String
	@returns wyTrue
	*/
	wyBool              DumpTableStructureOnView(wyString * buffer, const wyChar *view, wyBool isview = wyFalse);

	/// Handles when the flush log is selected
	/**
	@returns wyTrue
	*/
    wyBool              OnFlushLog();

	/// When this function is called , it unlocks the table
	/**
	@param query		: IN/OUT Query string
	@param table		: IN table name
	@returns wyTrue
	*/
    wyBool              DumpTableOnLockTables(wyString &query, const wyChar *table);

	/// Things to do when the table structure is not selected during dump table
	/**
	@param buffer		: IN/OUT String
	@param table		: IN Table name
	@returns wyTrue on success else returns error
	*/
    wyBool              DumpTableNotOnTableStructure (wyString * buffer, const wyChar *table);

	/// Dumps the given table preserving its structure
	/**
	@param buffer		: IN/OUT String
	@param table		: IN Table name
	@returns wyTrue on success else returns error
	*/
    wyBool              DumpTableOnTableStructure (wyString * buffer, const wyChar *table);

	/// Unlocks the tables when they are locked
	/**
	@returns wyTrue on success else returns error
	*/
    wyBool              DumpTableOnLockTables ();

	/// Executes the FULSH MASTER statement
	/**
	@returns wyTrue on success else returns error
	*/
    wyBool              OnFlushMaster();

	/// This function initiates dumping of table 
	/**
	@param buffer		: IN/OUT String
	@param table		: IN Table name
	@param fileerr		: OUT File error
	@returns wyTrue on success else wyFalse
	*/
    wyBool              OnIndividualFiles(wyString * buffer, const wyChar *table, wyInt32 *fileerr);	
    
    
	/// Dump tables with selected the row 
	/**
	@param buffer		: IN/OUT String
	@param row			: IN Mysql row pointer
	@param db			: IN Database name
	@param table		: IN Table name
	@param rcount		: IN Number of rows
	@returns wyTrue on successful deletion else wyFalse
	*/
    wyBool              DumpTableDataRows(wyString * buffer, MYSQL_ROW *row, const wyChar *db, const wyChar *table, wyInt32 rcount);

	/// Dump tables with all the row 
	/**
	@param buffer		: IN/OUT String
	@param row			: IN Mysql row pointer
	@param res			: IN Mysql result set 
	@param db			: IN Database name
	@param table		: IN Table name
	@param fcount		: IN Fields count
	@param rcount		: IN Number of rows
	@param intable		: IN Staring of a table or inside a table
	@returns wyTrue on successful deletion else wyFalse
	*/
    wyBool              DumpTableDataAllRows(wyString * buffer, MYSQL_ROW *row, MYSQL_RES *res, const wyChar *db, const wyChar *table, 
                                                wyInt32 *fcount, wyInt32 rcount, wyBool intable);

	/// Executes the Single transaction statement
	/**
	@returns wyTrue on success else returns error
	*/
    wyBool              OnSingleTransaction();    
    
    
	/// Database name
	wyString			m_db;

	/// File name
	wyString        	m_file;

	/// Username
	wyString        	m_uname;

	/// Application name
	wyString        	m_appname;

	/// Password to use
	wyString        	m_password;

	/// Host name
	wyString        	m_hostname;

	//Wait_timeout
	wyString			m_mysqlwaittimeout;

	//Compressed protocol
	wyBool				m_compressedprotocol;

	/// Tunnel host site
	wyString        	m_tunnelhostsite;

	/// Character set 
	wyString        	m_charset;

	/// Foreign key string
	wyString        	m_pkstring;

	/// Complete path for export file 
	wyString			m_expfilepath;

    /// Error Msg
    wyString            m_fileerr;

	///Error or not
	wyBool				m_iserr;

	/// Escape char to use in export dialog
	wyChar              *m_escapeddata;

	/// Table name
	//wyString			m_table;

	/// View name
	//wyString			m_view;

	/// Procedure name
	//wyString			m_procedure;

	/// Function name
	//wyString			m_function;

	/// Trigger name
	//wyString			m_trigger;

	// Tunnel pointer
	Tunnel				*m_tunnel;

	/* Linked list maintaing for storing selected triggerss when  dumping*/
	List                    m_triggerlist;	

	/* Linked list maintaing for storing selected tables when  dumping*/
	List                    m_seltables;	
	/* Linked list maintaing for storing selected views when  dumping*/
	List                    m_selviews;
	/* Linked list maintaing for storing selected procedures when  dumping*/
	List                    m_selprocedures;
	/* Linked list maintaing for storing selected functions when  dumping*/
	List                    m_selfunctions;
	/* Linked list maintaing for storing selected triggers when  dumping*/
	List                    m_seltriggers;
	/* Linked list maintaing for storing selected events when  dumping*/
	List                    m_selevents;


	/// HANDLE to export file
	FILE    			*m_expfile;

	/// Pointer to mysql structure
	MYSQL				*m_mysql;

	/// Tunnel authentication
	TUNNELAUTH			m_auth;

	/// LPGUI_EXPORT_UPDATE_ROUTINE object
	LPGUI_EXPORT_UPDATE_ROUTINE		m_routine;

	/// Message parameter
	void							*m_lpparam;
	
	/// Error number
	wyInt32				m_errno;

	/// Error string
	wyString			m_error;

	/// Function to check if quote is required or not
	/**
	@returns escape char if new else NULL
	*/
	const char*			IsNewMySQL();
	
	/* flags for mysql dump */

	/// Flag to write use db stmt. in the file
	wyBool			m_usedb;		

	/// Flag to set fk_checks = 0 and 1
	wyBool			m_setfkchecks;	

	///Flag to set all databases
	wyBool			m_alldb;

	//Flag to set with mysql or without mysql
	wyBool			m_expmysql;

	///Flag to dump all objects i.e all table(s), view(s), trigger(s), function(s) and procedure(s)
	wyBool			m_allobjects;

	/// Flag to dump all tables 
	wyBool			m_alltables;	

	/// Flag to dump all views 
	wyBool			m_allviews;

	/// Flag to dump all functions
	wyBool			m_allfunctions;

	/// Flag to dump all procedures
	wyBool			m_allprocedures;

	/// Flag to dump all triggers
	wyBool			m_alltriggers;
	/// Flag to dump all triggers
	wyBool			m_allEvents;
	wyBool          m_iscommit;

	/// Enables or disables create views option
	wyBool			m_createviews;

	/// Enables or disables create procedure option
	wyBool			m_createprocedures;

	/// Enables or disables create function option
	wyBool			m_createfunctions;

	/// Enables or disables create trigger option
	wyBool			m_createtriggers;

	/// Enables or disables drop object option
	wyBool			m_dropobjects;

	/// Enables or disables flush log option
	wyBool			m_flushlog;

	/// Enables or disables bulk insert
	wyBool			m_bulkinsert;

	//Break into chunks or not
	wyBool			m_chunkinsert;

	/// Flag to dump all databases 
	wyBool			m_dumpalldbs;	

	/// Disabled keys or not ?
    wyBool			m_disablekeys;

	/// Table to be locked before exporting ?
	wyBool			m_locktables;

	/// Lock all tables across all databases
	wyBool			m_firstslave;

	/// Write INSERT DELAYED statements rather than INSERT statements. 
	wyBool			m_optdelayed;

	/// Flush master or not
	wyBool			m_flushmaster;

	/// Auto commit enabled or not
	wyBool			m_autocommit;

	/// Using Utf8 encoding or not
	wyBool			m_utf8charset;

	/// Tunneling or not ?
	wyBool			m_modeoftunnel;

	/// Dump table structure only on export
	wyBool			m_tablestructure;

	/// Use complete INSERT statements that include column names
	wyBool			m_completeinsert;

	/// Set data to readonly mode ?
	wyBool			m_dataonly;

	/// Create database in Export Data dialog ?
	wyBool			m_createdatabase;

	/// File mode to be append ?
	wyBool			m_filemodeappend;

	/// Create a lock table write ?
	wyBool			m_locktablewrite;

	/// Export table to individual file ?
	wyBool			m_individualtablefiles;

	/// Export table to individual file ?
	wyBool			m_individualdbfiles;

	/// View flag
	wyBool			m_views;

	/// To use timestamp file mode or not
	wyBool			m_filemodetimestamp;

	/// Position inside a query or not 
	wyBool			m_inquery;

	/// Checks for query without arguments 
	wyBool			m_basicquery;

	/// The complete length of the query
	wyBool			m_completecount;

	/// abort on error
	wyBool			m_abortonerr;

	/// Enterprise module HANDLE
#ifdef _WIN32
    HINSTANCE       m_entinst;
#endif

	/// Starting position of query
	wyUInt32        m_startpose;

	/// Max allowed size packet size
	wyUInt32        m_maxallowedsize;

	//single transaction
	wyBool			m_singletransaction;

	//compress flag
	wyBool			m_compress;

    wyString        m_strnewline;

#if !defined _WIN32 || defined _CONSOLE 
	CWyZip			m_zip;
#endif


#ifdef _WIN32
    CRITICAL_SECTION        *m_cs;
#endif
	
public:

	  MySQLDump();
     ~MySQLDump();

    /// Sets the connection info for SSL
    /**
    @param conn         : IN Connection information
    */
    void                SetSSLInfo(ConnectionInfo *conn);

	//Sets the connection info(compressed protocol, wait_timeout)
	/**
	@param conn : IN ConnectionInfo atruct pointer
	@retrun VOID
	*/
	void				SetOtherConnectionInfo(ConnectionInfo *conn);

	/// Sets flag for bulk insert in the export data dialog
	/**
	@param flag			: IN Set Bulk insert ?
	@returns void
	*/
	void			SetBulkInsert(wyBool flag);		

	/// Sets flag for break into chunks or not in the export data dialog
	/**
	@param flag			: IN Set Chunk insert ?
	@returns void
	*/
	void			SetChunkInsert(wyBool val);

	/// Sets flag for dropping a procedure, function , views and trigger all at once.
	/**
	@param flag			: IN Drop all ?
	@returns void
	*/
	void			SetDropObjects			(wyBool flag);

	/// Sets flag for create database
	/**
	@param flag			: IN Create database ?
	@returns void
	*/
	void			SetCreateDatabase		(wyBool flag);

	/// Sets flag for selection of all objects
	/**
	@param flag			: IN Select all ?
	@returns void
	*/
	void			SetAllObjects			(wyBool val = wyFalse);

	/// Sets flag for selection of all tables
	/**
	@param flag			: IN Select all ?
	@returns void
	*/
	void			SetAllTables			(wyBool flag);		

	/// Sets flag for creation of views
	/**
	@param flag			: IN Select all?
	@returns void
	*/

	void			SetAllViews			(wyBool flag);		

	/// Sets flag for selection of all of procedures
	/**
	@param flag			: IN Select all?
	@returns void
	*/

	void			SetAllProcedures			(wyBool flag);	

	/// Sets flag for selection of all of Functions	
	/**
	@param flag			: IN Select all?
	@returns void
	*/

	void			SetAllFunctions			(wyBool flag);	

	/// Sets flag for selection of all of events	
	/**
	@param flag			: IN Select all?
	@returns void
	*/

	void			SetAllEvents			(wyBool flag);	


	/// Sets flag for creation of views
	/**
	@param flag			: IN Create views?
	@returns void
	*/
	void			SetCreateViews			(wyBool flag);					

	/// Sets flag for creation of procedure
	/**
	@param flag			: IN Create procedure ?
	@returns void
	*/
	void			SetCreateProcedures		(wyBool flag);					

	/// Sets flag for creation of function
	/**
	@param flag			: IN Create function ?
	@returns void
	*/
	void			SetCreateFunctions		(wyBool flag);	

	/// Sets flag for creation of triggers
	/**
	@param flag			: IN Create trigger?
	@returns void
	*/
	void			SetCreateTriggers		(wyBool flag);	
	///save  selected tabless in linked list
	/**
	@param names        : IN TableNamesElem calss object
	@returns void
	*/
	void			SelectedTables(SelectedObjects *names);

	///save  selected views in linked list
	/**
	@param names        : IN ViewNamesElem calss object
	@returns void
	*/
	void            SelectViews(SelectedObjects *names);
	///save  selected procedures in linked list
	/**
	@param names        : IN ProcedNamesElem calss object
	@returns void
	*/
	void            SelectedProcedures(SelectedObjects *names);
	///save  selected functions in linked list
	/**
	@param names        : IN FunNamesElem  calss object
	@returns void
	*/
	void            SelectedFunctions(SelectedObjects *names);
	///save  selected triggers in linked list
	/**
	@param names        : IN TriggernamesElem calss object
	@returns void
	*/
	void            SelectedTriggers(SelectedObjects *names);
	///save  selected events in linked list
	/**
	@param names        : IN EventnamesElem calss object
	@returns void
	*/
	void            SelectedEvents(SelectedObjects *names);

	/// Sets to dump all databases
	/**
	@param val			: IN Dump all database ?
	@returns void
	*/
	void			SetDumpAllDbs           (wyBool val);	

	///Flag sets for Compress SQL
	/**
	@param val : IN wyTru or wyFalse
	@return VOID
	*/
	void			SetToCompress(wyBool val);

	/// Sets to dump all databases with mysql or without mysql
	/**
	@param val			: IN Dump with or without mysql ?
	@returns void
	*/
	void			SetDumpMysql           (wyBool val);	

	/// Sets to dump all triggers
	/**
	@param val			: IN Dump all triggers ?
	@returns void
	*/
	void            SetAllTriggers          (wyBool val);

	// Sets require delayed keyword or not 
	/**
	@param flag			: IN Enable/Disable delayed ?
	@returns void
	*/
    void			SetInsertDelayed		(wyBool flag);

	/// Enables or disables the different Keys for a table
	/**
	@param flag			: IN Enable / Disable ?
	@returns void
	*/
	void			SetDisableKeys			(wyBool flag);	

	/// Sets the flush logs before dump
	/**
	@param flag			: IN Flush ?
	@returns void
	*/
	void			SetFlushLogs			(wyBool flag);		

	/// Sets the flush logs master dump
	/**
	@param flag			: IN Flush ?
	@returns void
	*/
	void			SetFlushMaster			(wyBool flag);

	/// Sets the Lock table option 
	/**
	@param flag			: Lock table ?
	@returns void
	*/
	void			SetLockTable			(wyBool flag);		

	/// Sets the flush slave before dump
	/**
	@param flag			: IN Flush ?
	@returns void
	*/
	void			SetFlushSlave			(wyBool flag);

	/// Sets the option for the autocommit
	/**
	@param flag			: IN AutoCommit on ?
	@returns void
	*/
	void			SetAutoCommit			(wyBool flag);

	/// Sets the utf8 charset for exporting
	/**
	@param flag			: Utf8 charset ?
	@returns void
	*/
	void			SetDumpCharSet			(wyString &charset);

         /// Sets the utf8 charset for exporting
	/**
	@param flag			: Utf8 charset ?
	@returns void
	*/
	void			SetUtf8CharSet			(wyBool flag);
	/// Sets the tunnel mode for exporting
	/**
	@param flag			: IN Tunnel mode ?
	@returns void
	*/
	void			SetTunnelMode			(wyBool flag);
	
	
	/// Sets the write lock only in export data dialog
	/**
	@param flag			: IN Insert write lock only  ?
	@returns void
	*/
	void			SetInsertWriteLocks		(wyBool flag); 

	/// Sets the Dump structure only in export data dialog
	/**
	@param flag			: IN Dump structure only  ?
	@returns void
	*/
	void			SetDumpStructureOnly	(wyBool flag);	


	/// Sets the Dump database only in export data dialog
	/**
	@param flag			: IN Dump data only  ?
	@returns void
	*/
	void			SetDumpDataOnly			(wyBool val);

	/// Checks for the Complete insert option during exporting
	/**
	@param flag			: IN Complete insert ?
	@returns void
	*/
	void			SetCompleteInsert		(wyBool flag);	

	/// Checks for the USE DB option during exporting
	/**
	@param flag			: IN Use DB ?
	@returns void
	*/
	void			SetUseDb				(wyBool flag);

	/// Sets the foreign key checks in export dialog
	/**
	@param flag			: IN FK checks ?
	@returns void
	*/
	void			SetFKChecks				(wyBool flag);

	/// Enables or disables the file append mode
	/**
	@param val			: IN Enabled or disabled ?
	@returns void
	*/
	void			SetAppendFileMode		(wyBool flag);

	/// Sets the Max chunk size
	/**
	@param val			: IN Size to set
	@returns void
	*/
	void			SetChunkLimit			(wyInt32 val);

	/// Sets the bulk size 
	/**
	@param val			: IN Size of the bulk
	@returns void
	*/
	void			SetBulkSize				(wyInt32  val);

	/// Sets the Single transaction
	/**
	@param val			: IN single transaction
	@returns void
	*/
	void			SetSingleTransaction(wyBool val);
	
	/// Sets the time stamp mode
	/**
	@param
	*/
	void			SetTimeStampMode		(wyBool val);

	/// 
	/**
	@param 
	@returns void
	*/
	void			SetAppName				(const wyChar *appname);

	/// Sets the charset name
	/**
	@param charset			: Charset name
	@returns void
	*/
	void			SetCharSet				(const wyChar *charset);

	/// Sets the database name
	/**
	@param db				: Database name
	@returns void
	*/
	void			SetDatabase				(const wyChar *db);

	/// Sets the table 
	/**
	@param table			: Table name	
	@returns void
	*/
//	void			SetTables				(const wyChar * table);
	
	
	/// Sets the view 
	/**
	@param view 			: view name	
	@returns void
	*/
	//void			SetViews				(const wyChar * view);

	/// Sets the procedure
	/**
	@param proc				: procedure name	
	@returns void
	*/
	//void			SetProcedure			(const wyChar * proc);

	/// Sets the Function
	/**
	@param func		: Function name	
	@returns void
	//*/
	//void			SetFunction			(const wyChar * func);

	/// Sets the export file path
	/**
	@param filepath			: File path
	@param timestampdir		: Use timestamp or not
	@returns void
	*/
	wyBool			SetExpFilePath			(wyChar *filepath, wyBool timestampdir, wyBool timestampzip);

	/// Sets the tunnel host site
	/**
	@param hostname			: Host name
	@returns void
	*/
	void			SetTunnelHostSite		(const wyChar *hostname);

	/// Sets the export file
	/**
	@param outputfile
	@returns void
	*/
	void			SetExpFile				(const char * outputfile);

	/// Sets the host information
	/**
	@param hostname			: IN Host name
	@param uname			: IN User name
	@param password			: IN Password
	@param port				: IN Port number
	@param auth				: IN Tunnel authentication
	@returns void
	*/
	void			SetHostInfo				(const wyChar * hostname, 
											  const wyChar * uname, const wyChar * password, wyInt32 port, TUNNELAUTH * auth);

    /// Sets the host name along with other connection details
    /**
    @param conn             : IN CONNECTIONDETAILS structure 
    @param auth             : IN Tunnel authentication
    */
	void			SetHostInfo(ConnectionInfo *conn, TUNNELAUTH *auth);

	/// Sets flag abort on error	
	/**
	@param flag			: IN abort on err
	@returns void
	*/

	void			SetAbortOnErr			(wyBool flag);	


	void			SetAdvExpValues		(SCHDEXTRAOPTIONS *opt);

	// Set the default SQL mode
	/**
	@returns void
	*/
	void			SetDefaultSqlMode();

    /// Sets the critical section pointer
    /**
    @param cs           : IN CS pointer
    @returns            void 
    */
#ifdef _WIN32
    void            SetCriticalSection(CRITICAL_SECTION     *cs);
#endif

#ifdef _WIN32
	/// Sets the ent instance
	/**
	@param hinst		: Instance HANDLE
	@returns void
	*/
   void            SetEntInstance(HINSTANCE hinst);
#endif
	/// Gets the error 
	/**
	@returns error
	*/
	const wyChar*		GetError  ();

	/// Gets the error number
	/**
	@returns error number
	*/
	wyInt32				GetErrorNum();

	/// This function initializes dumping
	/**
	@param routine		: Function to a call back function
	@param lpparam		: Message pointer
	@param stopexport	: Stop execution condition
	@returns export error
	*/
	EXPORTERROR	Dump(LPGUI_EXPORT_UPDATE_ROUTINE routine, void * lpparam, wyInt32 * stopexport);

    void                StopExporting();
    wyBool              IsExportingStopped();
	wyInt32				GetmySQLCaseVar();
	wyBool				IsLowercaseFS();

private:	
	
	/// This function gets called if in case there is any error related to mysql database.
	/// it will display errors and finally closes the export file
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    OnError		();
	
	
	///	This function Initializes all flag values and sets up default values
	/**
	@returns void
	*/
	void	    Initialize	();

	/// This function Opens output file and assigns name , that was set by user using SetExpFile() function.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    OpenDumpFile();

	///  This function connects to the host 
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    ConnectToDataBase();
	
	
    /// Initiates dumping
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    StartDump();

	/// This functions fetches  all data bases in to result set object
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpAllDatabases(wyString * buffer);

	/// This function Dumps the data base with option selected tables or 
	/// it will dump all tables in the database or selected tables depending upon the options 
	/**
	@param buffer	: IN/OUT String
	@param db		: IN/OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpDatabase(wyString * buffer, const wyChar *db ,wyInt32 * fileerr = NULL);

	/// This function dumps only selected databases 
	/**
	@param buffer	: IN/OUT String
	@param db		: IN/OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpSelectedDatabases(wyString * buffer, wyString &db);

	/// dump all dbs to a single file
	/**
	@param buffer	 : IN/OUT String
	@param MYSQL_RES : IN dbres result of a data bases of a server.
	@return wyTrue.
	*/
	wyBool      DumpAlldbsToSingleFile(wyString * buffer, MYSQL_RES *dbres);

	/// dump table and temporary table strucures for views
	/**
	@param buffer		: IN/OUT String
	@param const wyChar : IN db data base name
	@return wyTrue if dump is success.
	*/
	wyBool      DumpTableAndViewTableStructure(wyString * buffer, const wyChar *db);

	///dump data base objects
	/**
	@param buffer		: IN/OUT String
	@param const wyChar : IN db data base name
	@return wyTrue if dump is success.
	*/
	wyBool      DumpDBRoutines(wyString * buffer, const wyChar *db);
	
	/// Prints header information at beginning of the exp file 
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue 
	*/
	wyBool	    Header(wyString * buffer);

	/// Prints Footer information at the end of the Exp file
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue 
	*/
	wyBool	    Footer(wyString * buffer);

	/// This function Opens Individual Text File for each table 
	/**
	@param table		: IN Table name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    OpenIndividualExpFileForEachTable(const wyChar * table);
	
	/// This function initiates dumping of table
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param Table		: IN Table name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpTable(wyString * buffer, const wyChar * db, const wyChar * table, wyInt32 * fileerr = NULL);

	/// release linked list memory used for maintaing selected tables, views, funs, triggers, procs, events(db objects)
	/**
	return void.
	*/
	//void        ReleaseMemory(List *elem);
	void        FreeMemory();
	/// This function initiates dumping of views
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param view			: IN View name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpView(wyString * buffer, const wyChar * db, const wyChar *view, wyInt32 * fileerr = NULL);

	/// This function initiates dumping of trigger
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param procedure	: IN Procedure name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpTrigger(wyString * buffer, const wyChar * db, const wyChar * trigger, wyInt32 * fileerr = NULL);

	/// This function initiates dumping of procedure
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param procedure	: IN Procedure name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpProcedure(wyString * buffer, const wyChar * db, const wyChar * procedure, wyInt32 * fileerr = NULL);

	/// This function initiates dumping of function 
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param function		: IN Function name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpFunction(wyString * buffer, const wyChar * db, const wyChar * fucntion, wyInt32 * fileerr = NULL);

	/// This function initiates dumping of event 
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param function		: IN event name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpEvent(wyString * buffer, const wyChar * db, const wyChar * event,wyBool &seteventschdule , wyInt32 * fileerr = NULL);

	/// This function initiates dumping of View 
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@param trigger		: IN Trigger name
	@param fileerr		: OUT File error number
	@returns wyTrue on success else wyFalse
	*/
	//wyBool	    DumpTrigger(wyString * buffer, const wyChar * db, const wyChar * table, const wyChar * trigger, wyInt32 * fileerr = NULL);

	/// Dump all tables
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpAllTables(wyString * buffer, const char * db);

	/// Dump views
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpViews		(wyString * buffer, const wyChar * db, wyBool isviewstructure = wyTrue);

	/// Dump procedure
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpProcedures	(wyString * buffer, const wyChar * db);

	/// Dump functions
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpFunctions	(wyString * buffer, const wyChar * db);
	/// Dump events
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpEvents   	(wyString * buffer, const wyChar * db);

	/// Dump triggers
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpTriggers	(wyString * buffer, const wyChar *db);

	/// This function gets all primary key columns from table and prepares pkstring, which can be
	/// used while executing select tatement with order by pk cloumns
	/**
	@param db			: IN Database name
	@param table		: IN Table name
	@returns wyTrue on success else wyFalse
	*/
	wyBool  	GetPrimaryKeyCols(const wyChar * db , const wyChar * table);

	/// This function  prints each and every field value to the expoert file
 	/// in order ti dump table data
	/**
	@param buffer		: IN/OUT String
	@param res			: IN Mysql result set pointer
	@param row			: IN Mysql row
	@param fcount		: IN Field count
	@returns wyTrue
	*/
	wyBool  	PrintFieldValue(wyString * buffer, MYSQL_RES *res, MYSQL_ROW row, wyInt32 fcount);

	/// This function prints Database related header information 
    /// to the export file 
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    PrintDBHeaderInfo(wyString * buffer);

	/// This function adds footer information after dumping table data 
	///	something like unlocking,and enable keys and all.
	/**
	@param buffer		: IN/OUT String
	@param row			: IN Mysql row
	@param db			: IN Database name
	@param table		: IN Table name
	@returns wyTrue
	*/
	wyBool  	TableFooter(wyString * buffer, MYSQL_ROW row , const wyChar *db, const wyChar *table);

	/// This function prints header information before dumping table data
 	///for each table, information something like comments,disable keys locking and all
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@param fcount		: IN Field count 
	@param rcount		: IN Row count
	@param res			: IN Mysql result set
	*/
	wyBool  	TableHeader(wyString * buffer, const wyChar * db, const wyChar * table,wyInt32 fcount, wyUInt32 rcount, MYSQL_RES * res);

	/// Dumps the selected tables
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@returns wyTrue on success else wyFalse
	*/
	wyBool      DumpSelectedTables(wyString * buffer, wyString &db);
	
	/// Dumps the selected triggers
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue is success else wyFalse
	*/
	wyBool      DumpSelectedTriggers(wyString * buffer);

	/// Dumps the selected views
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue is success else wyFalse
	*/
	wyBool      DumpSelectedViews(wyString * buffer);
	
	/// Dumps the selected procedure
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue is success else wyFalse
	*/
	wyBool		DumpSelectedProcedure(wyString * buffer);
	
	/// Dumps the selected Function
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue is success else wyFalse
	*/
	wyBool		DumpSelectedFunction(wyString * buffer);

	
	/// Dumps the selected Event
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue is success else wyFalse
	*/
	wyBool		DumpSelectedEvent(wyString * buffer);

	/// Dumps the table structure
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@returns wyTrue 
	*/
	wyBool  	DumpTableStructure(wyString * buffer, const wyChar * db, const wyChar * table);

	/// This function fetches the data from from table and outputs to the dump file(m_exprt File);
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	    DumpTableData(wyString * buffer, const wyChar * db, const wyChar * table);

	/// This function inserts field names in to insert statement, in case complete insert option 
	/// is wyTrue.
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param res			: IN Mysql result set
	@param table		: IN Table name
	@param fieldcount	: INNumber of fields
	@returns wyTrue on success else wyFalse
	*/
	wyBool  	CompleteInsert(wyString * buffer, const wyChar * db, MYSQL_RES * res ,const wyChar * table ,wyInt32 fcount);

	/// Checks and determines the query length
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@param table		: IN Table name
	@param q_res		: IN Mysql result set
	@param fieldcount	: IN Number of fields
	@returns wyTrue on length within limit else wyFalse
	*/
	wyBool	    CheckQueryLength(wyString * buffer, const wyChar * db, const wyChar * table, MYSQL_RES * q_res, wyInt32 fieldcount);

	/// This function gets row count
	/**
	@param db		: IN Database name
	@param table	: IN Table name
	@returns the row count 
	*/
	wyUInt32    GetRowCount(const wyChar * db, const wyChar * table);

	/// Sets the appropriate titles
	/**
	@param buffer	 : IN/OUT String
	@returns wyTrue
	*/
	wyBool  	Title(wyString * buffer);

	/// This function opens the output file, Table structure and the data will be dumped in to the file opened by this function.
	/**
	@returns wyTrue
	*/
    wyBool	    SafeExit  (); 

	/// Extracts the file name from the path given
	/**
	@param path			: IN Full path
	@returns filename
	*/
	wyChar*	    GetFileNameFromPath(wyChar * path);

	/// Create proper file name if timestamp is selected
	/**
	@param path			: OUT Path
	@returns void
	*/
	void    	GetFormattedFilePath(wyString &path, wyBool iswithtimestamp = wyTrue);

	//Helper to execute query
    MYSQL_RES   *ExecuteQuery(wyString& query);


	/// Dumps the databse structure
	/**
	@param buffer		: IN/OUT String
	@param db			: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool		DumpDb(wyString * buffer, const wyChar *db);

	wyBool		OpenIndividualExpFileForEachDb(const wyChar *db);

	/// Writes the given string to the file
	/**
	@param buffer		: IN/OUT String
	@param buffer		: IN String to print
	@param isforce		: IN Flag to decide force writing to file.
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool      WriteBufferToFile(wyString *buffer, wyBool  isforce = wyFalse);

}; 

#endif
