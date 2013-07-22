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

#ifndef __CSWRAPPER__
#define __CSWRAPPER__

#ifdef _WIN32
	#include <winsock.h>
	#include <mysql.h>
#else
	#include <mysql/mysql.h>
#endif

#ifdef _WIN32
    #include "Tunnel.h"
#else
    #include "Tunnel.h"
#endif    

#ifdef COMMUNITY 
#include "Global.h"
#else
#include "CommonJobStructures.h"
#endif

#include "Datatype.h"
#include "wyString.h"



/// Function checks whether there a unprintable character in the XML fragment 
/**
@param text             : IN Text to check
@returns wyTru on success else wyFalse
*/
wyBool IsBadforXML(const char * text);

/// Executes an SQL query specified as a counted string.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@param query            : IN Query
@param len              : IN length of the query
@returns zero if the query was successful. non-zero if an error occurred.
*/
int sja_mysql_real_query(Tunnel * tunnel, MYSQL * mysql, const char * query, unsigned long len);

/// Gets or initializes a MYSQL structure.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns an initialized mysql* handle, null if there was insufficient memory to allocate a new object.
*/
MYSQL* sja_mysql_init(Tunnel * tunnel, MYSQL * mysql);

/// Sets a Specified characterset type before connection.
/**
@param mysql            : IN Mysql pointer
@param option           : IN enum mysql_option type specifying a number
@param arg				: IN character pointer speciying character set name like utf8, latin1
@returns an initialized mysql* handle, null if there was insufficient memory to allocate a new object.
*/
int sja_mysql_options(MYSQL * mysql, enum mysql_option option, const char *arg);

/// Establishes connection to a MySQL server.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@param host             : IN Host name
@param user             : IN User name
@param passwd           : IN Password
@param db               : IN Database name
@param port             : IN Port number
@param unix_socket      : IN Socket name or named pipe
@param clientflag       : IN Client flag for multiple connection
@param url              : IN Url
@returns a mysql* connection handle if the connection was successful, null if the connection was unsuccessful.
for a successful connection, the return value is the same as the value of the first parameter.
*/
MYSQL*	sja_mysql_real_connect(Tunnel * tunnel, MYSQL *mysql, const char *host,
                                 const char *user,
                                 const char *passwd,
                                 const char *db,
                                 unsigned int port,
                                 const char *unix_socket,
                                 unsigned long clientflag, char * url );

/// Gets the mysql server information
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns a character string that represents the server version number.
*/
const char* sja_mysql_get_server_info(Tunnel * tunnel, MYSQL * mysql);

/// Gets the mysql error number
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns the error number
*/
unsigned int sja_mysql_errno(Tunnel * tunnel, MYSQL * mysql);

/// Gets the mysql error
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns the error
*/
const char* sja_mysql_error(Tunnel * tunnel, MYSQL * mysql);

/// Closes the connection to mysql
/**
@param tunnel           : IN Tunnel pointer
@param sock             : IN Mysql pointer
@returns void
*/
void sja_mysql_close(Tunnel * tunnel, MYSQL * sock);

/// Retrieves a complete result set to the client.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns a mysql_res result structure with the results. null if an error occurred.
*/
MYSQL_RES * sja_mysql_store_result(Tunnel * tunnel, MYSQL *mysql, bool profile = true, bool force = false);

/// Initiates a row-by-row result set retrieval.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns a mysql_res result structure. null if an error occurred.
*/
MYSQL_RES * sja_mysql_use_result(Tunnel * tunnel, MYSQL *mysql);

/// Retrieves the next row of a result set. When used after mysql_store_result().
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@returns a mysql_row structure for the next row. null if there are no more rows to retrieve or if an error
occurred.
*/
MYSQL_ROW	sja_mysql_fetch_row(Tunnel * tunnel, MYSQL_RES *result);

/// Frees the memory allocated for a result set by mysql_store_result()
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@returns the number of rows in the result set.
*/
void	sja_mysql_free_result(Tunnel * tunnel, MYSQL_RES *result);

/// Returns the number of rows in the result set.
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@returns the number of rows in the result set.
*/
my_ulonglong sja_mysql_num_rows(Tunnel * tunnel, MYSQL_RES *res);

/// Seeks to an arbitrary row in a query result set.
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@param offset           : IN Row number
@returns void
*/
void sja_mysql_data_seek(Tunnel * tunnel, MYSQL_RES *result, my_ulonglong offset);

/// Returns the lengths of the columns of the current row within a result set.
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@returns an array of unsigned long integers representing the size of each column(not including any terminating
null characters). NULL if an error occurred.
*/
unsigned long * sja_mysql_fetch_lengths(Tunnel * tunnel, MYSQL_RES *result);

/// Returns the number of columns in a result set.
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@returns an unsigned integer representing the number of columns in a result set.
*/
unsigned int sja_mysql_num_fields(Tunnel * tunnel, MYSQL_RES *res);


/// Fetches the field information
/**
@param tunnel           : IN Tunnel Pointer
@param res              : IN Mysql result set
@returns an array of mysql_field structures for all columns of a result set.
*/
MYSQL_FIELD * sja_mysql_fetch_fields(Tunnel * tunnel, MYSQL_RES *res);

/// Fetches the field information from a perticular field in a row
/**
@param tunnel           : IN Tunnel pointer
@param res              : IN Mysql result set
@param fieldnr          : IN Field number
@returns the mysql_field structure for the specified column.
*/
MYSQL_FIELD * sja_mysql_fetch_field_direct(Tunnel * tunnel, MYSQL_RES *res, unsigned int fieldnr);

/// Adds the escape characters to the given string
/**
@param tunnel           : IN Tunnel pointer
@param result           : IN Mysql result set
@param to               : IN The ending string
@param from             : IN The starting string
@param length           : Length of the string
@returns the length of the value placed into to, not including the terminating null character
*/
unsigned long sja_mysql_real_escape_string(Tunnel * tunnel, MYSQL *mysql, char *to, const char *from, unsigned long length);

/// 
/**
@param tunnel           : IN Tunnel pointer
@param result           : IN Mysql result set
@param offset           : IN filed index
@returns
*/
unsigned int sja_mysql_field_seek(Tunnel * tunnel, MYSQL_RES *result, unsigned int offset);

/// Fetches the field information
/**
@param tunnnel          : IN Tunnel pointer
@param result           : IN Mysql result set
@returns the fields
*/
MYSQL_FIELD * sja_mysql_fetch_field(Tunnel * tunnel, MYSQL_RES *result);

/// Gets the number of rows affected
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns the number of rows affected
*/
my_ulonglong sja_mysql_affected_rows(Tunnel * tunnel, MYSQL * mysql);

/// Counts the number of fields
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Pointer to mysql
@returns the number of fields
*/
unsigned int sja_mysql_field_count(Tunnel * tunnel, MYSQL *mysql);

/// Checks for tunnel existance
/**
@param tunnel : IN tunnel pointer
@returns true if tunnel else false
*/
bool sja_mysql_istunnel(Tunnel *tunnel);

/// Sets the charset for mysql
/**
@param tunnel   : IN Tunnel pointer
@param charset  : IN Character set
@returns void
*/
void sja_mysql_setcharset(Tunnel *tunnel, char*charset);

/// Checks for the correct tunnel version
/**
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@returns wyTrue on success else wyFalse
*/
bool sja_CheckCorrectTunnelVersion(Tunnel * tunnel, MYSQL * mysql);

/// Executes the given query and get back the result
/**
@param tunnel		: IN Tunnel pointer
@param mysql		: IN Mysql pointer
@param query		: IN Query
@param isreconnect  : IN def. flag tells whether need to be reconnect or not 
@param usedbname    : IN database name to be used in reconnect function, by default its NULL
@returns mysql result set
*/
MYSQL_RES* SjaExecuteAndGetResult(Tunnel * tunnel, PMYSQL mysql, wyString &query, wyBool isreconnect = wyFalse, bool isbatch = false, 
								  bool isforce = false, ConnectionInfo *coninfo = NULL, const wyChar *usedbname = NULL, wyInt32* stop = NULL, wyInt32* status = NULL);

/// Executes the given query and get back the result
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Mysql pointer
@param query    : IN Query
@returns mysql result set
*/
MYSQL_RES* SjaExecuteAndUseResult(Tunnel * tunnel, MYSQL * mysql, wyString &query);

/// Executes the mysql_next_result
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Mysql pointer
*/
INT  sja_mysql_next_result(Tunnel *tunnel, MYSQL * mysql);

/// Gets the max allowed packet size
/**
@param tunnel		: IN Tunnel pointer
@param mysql		: IN Pointer to mysql 
@returns the max allowed packet size
*/
wyUInt32    Get_Max_Allowed_Packet_Size(Tunnel *tunnel, PMYSQL mysql);

// linux patch....since stricmp is not defined in Linux...we write a wrapper
// of the same name and use strcmp inside

#ifndef _WIN32
   extern int stricmp(const char * arg1, const char * arg2);
   extern my_ulonglong _atoi64(const char * nptr);
   extern int strnicmp(const char * arg1, const char * arg2, int count);
#endif

#endif 

