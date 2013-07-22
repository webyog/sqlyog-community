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

#include "MDIWindow.h"
#include "Tunnel.h"
#include "wyString.h"

#ifndef __YOGEXPORT__
#define __YOGEXPORT__

/**
   * Wrapper for all the import return values.
*/
typedef	enum __import_errors
{
	SYSTEMERROR=0,/**< SYSTEMERROR, unexpected system failure */  
	GUIROUTINEERROR,/**< GUIROUTINEERROR, GUI routine error */  
	MYSQLERROR,/**< MYSQLERROR, MySQL error */  
	FILEERROR,/**< FILEERROR, File access error */  
	IMPORTSTOPPED,/**< IMPORTSTOPPED, Importing is stopped */  
	SUCCESS,/**< SUCCESS, Importing successful */  
	SUCCESSWITHERROR/**<SUCCESS,Importing successful when abort on error is not checked*/
} IMPORTERROR;

/**
*	A macro that returns maximum object name length.
*/
#define		MAX_OBJECT_NAME			128

/**
*	A macro that returns maximum filename.
*/
#define		MAX_FILE_NAME			1024

/**
*	A macro that returns maximum query length.
*/
#define		MAX_QUERY_LEN			1024


#define     ER_EMPTY_QUERY          1065

/* callback function to display any GUI related stuff.
   this is always called when we are reading a file */

typedef void (*gui_update_routin)(
    void * lpParam, long long int num_bytes, long long int num_queries
    );
typedef gui_update_routin LPGUI_UPDATE_ROUTINE;


/*! \class ImportFromSQL
    \brief Helps to import the sql statements from a file.

   This class imports batch files in to MySQL database, Exported bye CYogExport.
   In this process it connects to MySQL server opens batch file , 
   and it will read 1024 bites at a time in to a buffer , 
   from that it will take a single line of query  to issue mysql query.
   It  will find out single query with the help of delimiter " ; ", 
   it reads data in to query buffer until it reaches the delimiter , 
   and at the same time it calculates the length of the string, 
   and while parsing it will check for the comments and it will 
   skip those comments from execution. To optimize stuff always the 
   file is read into the same buffer to reduce the number of allocation/dellocations.
*/

class ImportFromSQL
{
public:



	/// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	 ImportFromSQL();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~ImportFromSQL();

    /// Initializes the tunnelmode
    /** Sets whether the tunnelmode is using or not.
    @param val			: IN wyTrue/wyFalse.
    @returns void.
    */
	void	SetTunnel(wyBool val);

    /// Sets the error filename
    /** 
    @param errorfile	: IN Error file name.
    @returns void.
    */
	void	SetErrorFile(wyString &errorfile);

    /// Sets the import filename
    /** 
    @param importfile	: IN Import file name.
    @returns void.
    */
	void	SetImportFile(wyString &importfile);

    /// Sets the tunnel host site
    /** 
    @param hostsite		: IN host site url.
    @returns void.
    */
	void	SetTunnelHostSite(wyString hostsite);

    /// Sets host information
    /** 
    @param host			: IN host address.
    @param user			: IN user name.
    @param pass			: IN password.
    @param dbname		: IN host database name.
    @param port			: IN host port.
    @returns void.
    */
	void	SetHostInfo(wyString host, wyString user, wyString pass, wyString dbname, int port);	

	/// Sets the Content-Type
    /** 
    @param contenttype		: IN contenttype.
    @returns void.
    */
	void	SetContentType(wyString contenttype);

	//Set the encoding style BASE64 or not
	/**
	@param isbase64encode : IN it's wyTrue for Base64 encode needed
	@return void
	*/
	void	SetImportEncodingScheme(bool isbase64encode);

	//Set the Abort on error flag
	/***
	@param abourt_on_error : If the checkbox for abort on nerror is set or not
	@return void
	*/
	void	SetAbortFlag(wyBool abort_on_error);

    /// This function will do the import according to the values set as well as the parameter given.
    /** 
    @param num_bytes	: OUT number of bytes read.
    @param num_queries	: OUT number of queries executed.
    @param gui_routine	: OUT callback function to display the info about importing.
    @param lpParam		: IN/OUT IMPORTBATCH structure containing all required values.
    @param stopimport	: IN/OUT Flag that is used to handle the thread.
	@param wnd			: IN con. window pointer
    @returns IMPORTERROR showing the status.
    @sa typedef	enum __import_errors
    @sa typedef struct __stimportbatch
    */
	IMPORTERROR	Import(wyUInt32 *num_bytes, wyUInt32 *num_queries, 
		LPGUI_UPDATE_ROUTINE gui_routine, void * lpParam, wyBool * stopimport, MDIWindow *wnd);

private:
    ///variable for Abort on error
	wyBool      m_abort_on_error;

	///variable to check non of the executed queries had error.This is useful when u have to ignore abort on error.
	wyBool		m_errorfree;
  
    /// Mysql port
	wyInt32			m_port;
    
	/// MySQl error number
	wyInt32			m_errno;
    
	/// pointer to keep the IMPORTBATCH
	void			*m_lp_param;
    
	/// DataBase name
	wyString		m_db;
    
	/// Import file name
	wyString		m_file;
    
	/// Host address
	wyString		m_host;
    
	/// user name
	wyString		m_user;
    
	/// Password
	wyString		m_pass;

    /// Error file name  
	wyString		m_errfile;
    
	/// pointer to Tunnel host site url
	wyString		m_tunnel_host_site;
    
	/// Status of tunnel
	wyBool			m_mode_of_tunnel;

    ///Content-type
	wyString		m_contenttype;

	//Whether the Base64 encoding needed or not
	wyBool			m_isbase64encode;

	/// Error description pointer	
	const wyChar	*m_error;
    
	/// Number of queries executed
	wyUInt32	    m_numquery;
    
	/// Total number of bytes to read
	wyUInt32	    m_numbytes;
    
	/// Number of bytes read
	wyUInt32	    m_bytesread;
    
	/// Line number
	wyUInt32	    m_line_number;
    
	/// File handle
	FILE			*m_importfile;
    
	/// mysql pointer
	MYSQL			*m_mysql;
    
	/// tunnel pointer
	Tunnel			*m_tunnel;

    /// GUI call back routine.
	LPGUI_UPDATE_ROUTINE	m_gui_routine;

    /// initialize the import with default values 
    /**
    @return void
    */
	void			Initialize();

    /// Releases the MySQl resources 
    /**
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			SafeExit();

    /// Writes the error message in SQLyog.err 
    /**
    @param errmsg : IN The query/errormsg that returned error, which is also logged in error file
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			OnError(const wyChar * errmssg);

    /// Connects to the mysql server 
    /**
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			MySQLConnect();

	/// Opens the import file for reading
    /** 
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			OpenImportFile();

	/// Initiates the import process
    /** 
    @param stopimport	: IN flag to control the import process
	@param	wnd			: IN con. window pointer
    @return wyBool, wyTrue if success, otherwise wyFalse.	
    */
	IMPORTERROR		StartImport(wyBool * stopimport, MDIWindow *wnd);

	/// Starts the import process
    /** 
    @param stopimport	: IN flag to control the import process
	@param isfkchkhttp  : IN flag tells whether set FK check = 0 if its wyTrue.
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			DoImport(wyBool * stopimport, wyBool isfkchkhttp);
	
	/// Executes a MySQL query on the connected server
    /** 
    @param query		: IN Query to be executed
    @param m_str_length	: IN Query length
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			ExecuteQuery(wyString& query, wyUInt32 m_str_length, bool isread, wyBool isfkchkhttp);

	/// Changes the server context database
    /** 
    @param query		: IN Query to be executed
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			ChangeContextDB(wyString& query);

	/// Gets the database name from the USE query
    /** 
    @param query		: IN Query to be executed
    @param db			: OUT database name
    @return void
    */
	void			GetDBFromQuery(wyString& query, wyString& db);

	void            GetSelectDB(wyString& importdb);

	/// Removes the spaces from the left of text
    /** 
    @param text			: IN text to be trimmed
    @return The trimmed text
    */
	const wyChar*	LeftPadText(const wyChar * text);

	/// Checks for bad char for XML
    /** 
    @param text			: IN text to be checked
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			IsBadforXML(const wyChar * text);

	/// Checks whether the buffer contains space only or not
    /** 
    @param text			: IN buff to be checked
    @return wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool			IsSpace(const wyChar * buff);
};

#endif
