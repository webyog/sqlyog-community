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

#ifndef _COMMON_MISC_
#define _COMMON_MISC_

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "Datatype.h"
#include "Tunnel.h"
#include "wyString.h"
#include "List.h"

#ifdef _CONSOLE
#define _(STRING)   STRING
#else
#include "L10nText.h"
#endif

#if ! defined COMMUNITY && defined _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "wyFile.h"
#endif

#ifdef _WIN32
#include "Global.h"
#else
#include "CommonJobStructures.h"
#endif

#ifndef     MAX_ROW_LIMIT
#define		MAX_ROW_LIMIT		1000
#endif

#define     NCP_UTF8    1
#define     NCP_UTF16BE 2
#define     NCP_UTF16   3
#define     NCP_ASCII   4

#define		FMT_SPACE_4 "    " 

#define		REGKEY			"Provide any UUID here"


#define		SSHTUNNELER 	  "plink.exe"
#define     SQLYOG_MUTEX_NAME "SQLyogSSHMutex"
#define     SQLYOG_SAVETIMER "SQLyogSaveTimer"
#define		SQLYOG_NAMEDMUTEX "SQLyogNamedMutex"

#define	 BULK_SIZE           16*1024*1024

#define	 PLINK_LOCK_WAIT  500
#define	 PLINK_LOCK_WAIT_TRY_COUNT  120

#define  WAIT_TIMEOUT_SERVER  "28800"
#define  PROCEDURE_FUNC_ERRMSG _("Unable to retrieve information. Please check your privileges. For routines (stored procedures and functions) you need SELECT privilege to mysql.proc if you are not the owner of the routines.")

#define  CPI_BIG5   950
#define  CPI_CP850  850
#define  CPI_KOI8R 20866
#define  CPI_LATIN1 1252
#define  CPI_LATIN2 28592
#define  CPI_SWE7  20107
#define  CPI_ASCII 20127
#define  CPI_UJIS  51932 
#define  CPI_SJIS  932 
#define  CPI_HEBREW 28598
#define  CPI_TIS620  874
#define  CPI_EUCKR  51949
#define  CPI_KOI8U 21866
#define  CPI_GB2312 936
#define  CPI_GREEK 28597
#define  CPI_CP1250 1250
#define  CPI_LATIN5 28599
#define  CPI_UTF8 65001
#define  CPI_CP866 866
#define  CPI_KEYBCS2 895
#define  CPI_MACCE  10029
#define  CPI_MACROMAN 10000
#define  CPI_CP852    852
#define  CPI_LATIN7  28603
//#define  CPI_UTF8MB4 65001
#define  CPI_CP1251   1251
#define  CPI_UTF8B	  0
#define  CPI_UTF16LE  91
#define  CPI_UTF16BE  92
#define  CPI_CP1256 1256
#define  CPI_CP1257 1257
//#define  CPI_UTF32  12000

#define  CPI_CP932 932

enum EntType
{
	ENT_ULTIMATE	= 1,
	ENT_NORMAL	= 2,
	ENT_PRO		= 3,
	ENT_TRIAL   = 4,
	ENT_INVALID	= 5
};


typedef struct ForeignKey
{
	wyString    m_fkey;
	wyString    m_fkeyname;
    wyString    m_fkeyinfo;
	wyString    m_constraint;
	wyBool      m_isleft;

}FKEYINFO, *LPFKEYINFO;
//class holds the field names that involved in the relationship and insert into linked list
class RelTableFldInfo : public wyElem
{
public:
	//Constructor & Destructor
	RelTableFldInfo(const wyChar *fieldname);

	~RelTableFldInfo();

	//field name of the f-key table , involves into relationship
	wyString	m_tablefld;
};
class SelectedObjects: public wyElem
{
public:
	SelectedObjects(const wyChar *tablename, wyBool ismysql41 = wyTrue);
	~SelectedObjects();
	wyString  m_selobject;
};
//For F-key options ON DELETE / ON UPDATE
enum fkeyOption
{
	NOOPTION	= 0,  //if no option selected for ON DELETE or ON UPDATE
	CASCADE		= 1,  
	SETNULL		= 2,
	NOACTION	= 3,
	RESTRICT	= 4
};

//Structure holds the all parsed infos of F-key
typedef struct FKeyInfos
{
	wyString	m_constraint;		// Constraint name
    wyString    m_parenttabledb;    // DB name of parent
	wyString	m_parenttable;		// Referenced table name
	List		*m_fkeyfldlist;		// List holds all fields in child table , involved in relationship
	List		*m_pkeyfldlist;		// List holds all fields in parent table , involved in relationship
	fkeyOption	m_ondelete;		 // gets if option ON DELETE is defined
	fkeyOption	m_onupdate;
}FKEYINFOPARAM, *LFKEYINFOPARAM;

struct BomLookup
{
	DWORD   m_bom;
	wyInt32 m_len;
	wyInt32 m_type;
};


wyBool		UseDatabase(wyString &dbname, MYSQL *mysql, Tunnel *tunnel);

///DatabaseName is Valid or not
/**
@param dbname           : IN DatabaseName
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns wyTrue if Valid else wyFalse
*/
wyBool		IsDatabaseValid(wyString &dbname, MYSQL *mysql, Tunnel *tunnel);
/// supporting transaction by setting autocommit variable
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns if success else wyFalse.
*/
wyBool		StartTransaction(Tunnel *tunnel, MYSQL *mysql);

/// ending transaction by commit
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@returns if success else wyFalse.
*/
wyBool      EndTransaction(Tunnel *tunnel, MYSQL *mysql);

#ifdef _WIN32

///Handle the ssh reconnection
/**
@param coninfo : IN ConnectionInfo pointer
@return true on success else return false
*/
bool	   ReConnectSSH(ConnectionInfo *coninfo);

#ifdef WIN32 
///Create SSH session
/**
@param coninfo  : IN ConnectionInfo pointer
@param pi       : IN PROCESS_INFORMATION pointer
@param hmapfile : IN Handle to file map
@return 0 success else return grater than 0
*/
wyInt32    CreateSSHSession(ConnectionInfo *conninfo, PROCESS_INFORMATION * pi, HANDLE &hmapfile);

///Exit commands for closing ssh connection
/**
@param coninfo	: IN ConnectionInfo struct pointer
@return VOID
*/
void		OnExitSSHConnection(PIPEHANDLES *sshpipehanles);


#endif

/// Handles the reconnect
/**
@param tunnel : IN tunnel pointer
@param mysql  : IN handle to PMYSQL
@param dbname : IN database name to be used to issue 'USE <dbname>'
@return wyTrue on success else return wyFalse
*/
bool	   HandleReconnect(Tunnel *tunnel, PMYSQL mysql, ConnectionInfo *coninfo, const wyChar *dbname);

// Set the default SQL mode
/**
@param tunnel : IN tunnel pointer
@param mysql  : IN mySQL pointer
@returns void
*/
void		SetDefSqlMode(Tunnel *tunnel, MYSQL *mysql);

/// Sets the character set for the particular connection
/**
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@return void
*/
void	   SetCharacterSet(Tunnel *tunnel, MYSQL *mysql, wyString &cpname);

wyInt32	   GetSjaExecPath(wyChar *buffer);

#endif

/// Create proper timestamp to create the file name
/**
@param buff         : OUT Buffer to the filename
@param timesep      : IN  Time stamp
*/
void    GetTimeString(wyString& buff, wyChar *timesep);

/// Retrieves the file path
/**
@param filename     : IN File name
@param extension    : IN File extension
@param bufferlength : IN Buffer length
@param buffer       : OUT Pointer to the buffer that receives the path and file name of the file found.
@param lpfileport   : OUT Pointer to the variable that receives the address (within buffer) of the last component of the valid path and file name, which is the address of the character immediately following the final backslash (\) in the path. 
@returns wyTrue if successful else wyFalse
*/
wyBool  SearchFilePath(wyWChar *filename, wyWChar *extension, wyInt32 bufferlength, 
                      wyWChar *buffer, wyWChar **lpfileport);

/// Copies table from new to old
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@param newtargettunnel  : IN Target tunnel
@param newtargetmysql   : IN Target mysql
@param db               : IN Database name
@param table            : IN Table name
@returns wyFalse on success else return wyTrue
*/
wyInt32 CopyTableFromNewToOld(Tunnel * tunnel, PMYSQL mysql, Tunnel * newtargettunnel,
							  PMYSQL newtargetmysql, const wyChar *db, const wyChar *table, wyString & query);

/// Gets the field index
/**
@param tunnel       : IN Tunnel pointer
@param result       : IN Mysql result set
@param colname      : IN Column name
*/
wyInt32 GetFieldIndex(Tunnel *tunnel, MYSQL_RES * result, wyChar * colname);
//function was written as a patch for bug--http://bugs.mysql.com/bug.php?id=75685.
wyInt32 GetBodyOfTrigger(wyString *body );
#ifdef _WIN32
/// Gets the CREATE TRIGGER string
/**
@param hwnd			: IN Window handle
@param tunnel       : IN Tunnel pointer
@patam mysql        : IN Mysql pointer
@param db           : IN Database name
@param table        : IN Table name
@param strprocedure : IN Start procedure
@param strmsg       : OUT The SELECT stmt
@param query		: OUT the query got
@param definer		: OUT definer
@returns wyTrue on success else wyFalse
*/
wyBool  GetCreateTriggerString(HWND hwnd , Tunnel * tunnel, PMYSQL mysql, const wyChar *db, 
                               const wyChar *trigger, wyString &strtrigger, wyString &strmsg, wyBool isdefiner = wyFalse);
#endif

/// Get Error
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Pointer to mysql pointer
@param strmsg   : OUT The SELECT stmt
@returns void   
*/
void    GetError(Tunnel * tunnel, PMYSQL mysql, wyString &strmsg);

/// Gets the CREATE FUNCTION string
/**
@param tunnel       : IN Tunnel pointer
@patam mysql        : IN Mysql pointer
@param db           : IN Database name
@param function     : IN Function name
@param strprocedure : OUT The SELECT stmt
@param strmsg       : OUT The ERROR stmt if any
@param queryex		: OUT query
@returns wyTrue on success else wyFalse
*/
wyBool  GetCreateFunctionString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar * function, 
                                wyString &strfunction, wyString &strmsg, wyString *queryex = NULL);

/// Gets the CREATE PROCEDURE string
/**
@param tunnel       : IN Tunnel pointer
@patam mysql        : IN Mysql pointer
@param db           : IN Database name
@param procedure    : IN Procedure name
@param strprocedure : OUT The SELECT stmt
@param strmsg       : OUT The ERROR stmt if any
@param queryex		: OUT query
@returns wyTrue on success else wyFalse
*/
wyBool  GetCreateProcedureString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar *procedure, 
								 wyString &strprocedure, wyString &strmsg, wyString *queryex = NULL);
/// Gets the CREATE EVENT string
/**
@param tunnel       : IN Tunnel pointer
@patam mysql        : IN Mysql pointer
@param db           : IN Database name
@param event        : IN Procedure name
@param strevent		: OUT The SELECT stmt
@param strmsg       : OUT The ERROR stmt if any
@param queryex		: OUT query
@returns wyTrue on success else wyFalse
*/
wyBool  GetCreateEventString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar *event, 
							 wyString &strevent, wyString &strmsg, wyString *queryex);


/// This function gets only the REAL comment, strips of Foreign Key definitions, etc 
/**
@param comment      : OUT Comment statement
@returns comment string
*/
wyBool  GetCommentString(wyString &commentstring);

/// This function gets only table charset from collation
/**
@param comment      : OUT Comment statement
@returns comment string
*/
wyBool  GetCharsetString(wyString &charsetstring);

/// Get the number of rows
/**
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param db           : IN Database name
@param table        : IN Table name
@param whereclause  : IN WHERE clause to consider while counting rows
@returns the row count
*/
wyUInt32 GetRowCount(Tunnel *tunnel, MYSQL *mysql, const wyChar *db , 
                     const wyChar *table, const wyChar * whereclause);


/// Gets the create table string
/**
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param db           : IN Database name
@param tbl          : IN Table name
@param strcreate    : OUT The CREATE statement
@returns wyTrue on success else wyFalse
*/
wyBool  GetCreateTableString(Tunnel * tunnel, PMYSQL pmysql, const wyChar *db, 
							 const wyChar* tbl, wyString &strcreate, wyString &query);

/// Reverses the given string 
/**
@param text         : IN/OUT Text to change
@returns void
*/
void    ReverseString(wyChar *text);


/// Checks for the keyword variables
/*
@param val      : IN Value
@returns wyTrue on success else wyFalse
*/
wyBool  CheckForVariable(const wyChar *val);

/// Create the SELECT stmt to get all VIEWs for the given database
/*
@param db           : IN Database name
@param selectstmt   : OUT The SELECT stmt
@returns length
*/
wyInt32 GetSelectViewStmt(const wyChar *db, wyString &selectstmt);

/// Create the SELECT stmt to get all PROCEDUREs for the given database
/*
@param db           : IN Database name
@param selectstmt   : OUT The SELECT stmt
@returns length
*/
wyInt32 GetSelectProcedureStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate = wyFalse);

/// Create the SELECT stmt to get all FUNCTIONs for the given database
/*
@param db           : IN Database name
@param selectstmt   : OUT The SELECT stmt
@returns length
*/
wyInt32 GetSelectFunctionStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate = wyFalse);
/// Create the SELECT stmt to get all events for the given database
/*
@param db           : IN Database name
@param selectstmt   : OUT The SELECT stmt
@returns length
*/
wyInt32 GetSelectEventStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate = wyFalse);

/// Create the SELECT stmt to get all Triggeres for the given database
/*
@param db           : IN Database name
@param selectstmt   : OUT The SELECT stmt
@returns length
*/
wyInt32 GetSelectTriggerStmt(const wyChar *db, wyString &selectstmt);

/// Get Application folder
/**
@param path     : OUT Path of the application
@returns wyTrue on success else wyFalse
*/
wyBool  GetApplicationFolder(wyWChar *path);

/// Allocate memory For Wide Char
/**
@param size     : IN The size of memory required
@returns the allocated buffer
*/
wyWChar *AllocateBuffWChar(wyInt32 size);

/// Allocate memory
/**
@param size     : IN The size of memory required
@returns the allocated buffer
*/
wyChar  *AllocateBuff(wyInt32 size);

/// Re-allocates the buffer
/**
@param buff         : IN Buffer to reallocate
@param size         : IN newsize
@returns the reallocated buffer
*/
wyChar  *RewAllocateBuff(wyChar *buff, wyInt32 size);

/// Creates a tunnel 
/**
@param istunnel     : IN tunneling or not.
@returns a instance of the Tunnel(Child) class
*/
Tunnel  *CreateTunnel(wyBool istunnel);

/// Checks for the SQLYOG files
/**
@param filename     : IN Filename to check
@returns wyTrue on success else wyFalse
*/
wyBool  CheckSQLyogFiles(const wyWChar *filename);

/// Checks for the existence of a file
/**
@param buffer       : IN Buffer to get the whole file name
@param path         : IN Parent directory path of the file
@param filename     : IN File name
@param extension    : IN File extension
@returns wyTrue on success else wyFalse
*/
wyBool  CheckFileExists(wyWChar *buffer, const wyWChar *path, const wyWChar *filename, 
                        const wyWChar *extension);

/// Gets the field inforamtion
/**
@param tunnel       : IN Tunnel pointer
@param myres        : IN Mysql result pointer
@param strcreate    : OUT CREATE statement
@param tgttunnel    : IN Pointer to target tunnel
@param tgtmysql     : IN Pointer to target mysql pointer
@returns void
*/
void    GetFieldInfoString(Tunnel *tunnel, MYSQL_RES *myres, 
                        wyString &strcreate, Tunnel *tgttunnel, PMYSQL tgtmysql);

/// Check for presence of ON UPDATE statement 
/**
@param strcreate    : IN CREATE statement
@param fieldpos     : IN Field position
@returns wyTrue on success else wyFalse
*/
wyBool CheckForOnUpdate(wyString &strcreate, wyInt32 fieldpos);

//get the expreeion value by parsing the current row definition
/**
@param currentrow    : FIELD definiton in create table statement
@param expression     : Value of expression used
@returns wyTrue on success else wyFalse
*/
wyBool GetExpressionValue(wyChar * currentrow, wyString * expression);

//get the check constraint expression value by parsing the current row definition
/**
@param currentrow    : FIELD definiton in create table statement
@param expression     : Value of expression used
@returns wyTrue on success else wyFalse
*/
wyBool GetCheckConstraintValue(wyChar * currentrow, wyString * expression);
wyBool GettablelevelCheckConstraintValue(wyChar * currentrow, wyString * expression);
void  CheckForQuotesAndReplace(wyString *name);

wyBool GetCheckConstraintName(wyChar * currentrow, wyString * checkconstraintname);

wyInt32 GetBitFieldColumnWidth(wyString &strcreate, wyInt32 fieldpos);

/// Gets the module filename length.
/**
@return the filename length
*/
wyInt32  GetModuleNameLength();

wyBool	GetModuleDir(wyString &path);

//Gets the mysql specific escaped value.
/**
@param tunnel	: IN Tunnel pointer
@pararm mysql	: IN mysql pointer
@param value	: IN buffer to be escaped
@returns an escaped buffer
*/
wyChar * GetEscapedValue(Tunnel *tunnel, PMYSQL mysql, const wyChar *value);

/// Prepares to show the tables
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Mysql pointer
@param db       : IN Database name
@param query    : IN Query
@returns the query length
*/
wyUInt32 PrepareShowTableInfo(Tunnel * tunnel, PMYSQL mysql, wyString& db, wyString &query);

/// Prepares to show all the tablenames
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Mysql pointer
@param db       : IN Database name
@param query    : IN Query
@returns the query length
*/
wyUInt32 PrepareShowTable(Tunnel * tunnel, PMYSQL mysql, wyString& db, wyString &query);

/// Decode a base64 buffer
/**
@param src      : IN Source
@param dest     : OUT Destination
@returns wyTrue on success else wyFalse
*/
size_t  DecodeBase64(const wyChar *src, wyChar *dest);

/// Encodes a base64 buffer
/**
@param inp      : IN Input string
@param insize   : IN Size of the string
@param outptr   : OUT Pointer to an allocated area holding the base64 data
@returns wyTrue on success else wyFalse
*/
wyInt32 EncodeBase64(const wyChar *inp, size_t insize, wyChar **outptr);

///  Decodes a string
/**
@param dest     : OUT Destination
@param src      : IN Source 
@returns void
*/
void    DecodeQuantum(wyUChar *dest, const wyChar *src);

///  Extract the engine name from the details like 'have_enginename_engine
/**
@param engine : IN engine name like 'have_en...'
@param  enginename: OUT extracted engine name
@returns void
*/
void    ExtractEngineName(wyChar *engine, wyString &enginename);


///  Checks whether the engine is listed in the list 
/**
@param engine : IN engine name 
@returns wyBool, wyTrue if exists, otherwise wyFalse
*/
wyBool  IsSupportedEngine(wyString &enginename);

/// Collects all the available tables engines and stores in a string seperated by ;
/**
@param tunnel   : IN Tunnel pointer
@param mysql    : IN Mysql pointer
@param strengine: OUT engines seperated be semicolon(;) 
*/
void    GetTableEngineString(Tunnel *tunnel, PMYSQL mysql, wyString &strengine);

/// Gets the chunk limit for the insert stmts
/**
@param chunklimit: OUT chunk limit
@returns chunklimit
*/
wyInt32 GetChunkLimit(wyInt32 *chunklimit);

///Break into chunks or not
/**
@returns wyTrue if Don't break into chunks is Unchecked
*/
wyBool	IsChunkInsert();

/// Helps in comparing two strings
/**
@param arg1: IN string1
@param arg2: IN string2
*/
wyInt32     Comparestring(wyWChar **arg1, wyWChar **arg2);

///Converts a normal string into Wide Character version
/**
@param arg1: IN string
@param arg2: IN *len
@param widecharbuffer: OUT wide character buffer
@returns widecharbuffer
*/
//wyWChar*	GetAsWideChar(const wyChar *normalstring);

/// Delete the selected profile string in the ini file
/**
@param strkey       : IN String to be deleted
@param section      : IN Section to which it belongs
@param filename     : IN Ini file name
*/
wyInt32 DeletePrivateProfileString(wyChar *strkey, wyChar *section, wyChar *filename);


/// Connects to mysql
/**
@param info         : IN Connection information
@param pmysql       : IN Pointer to mysql pointer
@param auth         : IN Tunnel auth pointer
@param errmsg       : IN Error message , if any
*/
//Tunnel	*ConnectToMySQL(ConnectionInfo *info, PMYSQL pmysql, TUNNELAUTH * auth, wyString &errmsg);

/// Retrives the comment part
/**
*/
wyBool      GetCommentString(wyString &commentstring);

#ifdef _WIN32

wyBool      GetForeignKeyInfo(wyString *showcreate,
                              wyString &key, wyInt32 relno, FKEYINFOPARAM *fkeyparam = NULL, const wyChar* dbname = NULL);

void		ExamineData(wyString &codepage, wyString &buffer);
#endif

/// 
wyBool      CheckForUtf8(wyString &pBuffer);
wyInt32     DetectFileFormat(wyChar *pBuffer, wyInt32 pBytesRead, wyInt32 *pHeaderSize);

void		GetTableEngineString(Tunnel *tunnel, PMYSQL mysql, wyString &strengine);

wyBool		CheckForUtf8Bom(wyString &filename);

wyBool      GetServerDefaultCharset(Tunnel *tunnel, MYSQL *mysql, wyString &charset, wyString &query);
wyBool      GetDbCollation(Tunnel *tunnel, MYSQL *mysql, wyString &collation);
/*! \struct Datatype
	\brief Struct to hold the mysql data type and the respective excel data type

	wyString mysqltype		Mysql data type
	wyString exceltype		Excel data type
	wyUInt32  length		length of the field
*/
struct __datatype
{
	wyString m_mysqltype;
	wyString m_exceltype;
    wyULong *length;
};
typedef struct __datatype MysqlDataType;


#define YOGLOG(x, y)	{YogDebugLog(x,y,__FILE__, __LINE__);}
void YogDebugLog(int errcode, const char* msg, char* file, int line);

/// Gets the mysql datatype and returns the respective excel datatype
/**
@param rettype		: OUT Returning datatype
@param fieldno		: IN Field number
@return wyTrue on successful write in the file else wyFalse
*/
wyBool	GetMySqlDataType(MysqlDataType *rettypedata, MYSQL_FIELD *fields, wyInt32 fieldno);

/// Delete the selected profile string in the ini file
/**
@param strkey       : IN String to be deleted
@param section      : IN Section to which it belongs
@param filename     : IN Ini file name
*/
wyInt32 DeletePrivateProfileString(wyChar *strkey, wyChar *section, wyChar *filename);

wyInt32	HandleMySQLRealQuery(Tunnel *tunnel, MYSQL * mysql, const char * query, 
							 unsigned long length, bool isbadforxml, bool batch = false, 
							 bool isend = false, bool * stop = 0, bool isread = false, bool fksethttpimport = false);

/// Executes an SQL query specified as a counted string.
/**
@param tunnel           : IN Tunnel pointer
@param mysql            : IN Mysql pointer
@param query            : IN Query
@param len              : IN length of the query
@returns zero if the query was successful. non-zero if an error occurred.
*/
int HandleSjaMySQLRealQuery(Tunnel * tunnel, MYSQL * mysql, const char * query, unsigned long len, bool isbatch = false, bool* stop = NULL);

/// gets the row value length
/**
@param row  : IN mysql row pinter
@param numcols : IN number of columns
@param col : IN col
@param len : OUT collength
#returns void
*/
void GetColLength(MYSQL_ROW row, wyInt32 numcols, wyInt32 col, wyUInt32 *len);

///Gets all parsed infos of a foreign key and insert into a struct
/**
@param fkeyinfo : IN sting contains a f-key details 
@param fkeyparam : OUT struct loads with F-key details parsed from 'key'
@return wyTrue for success, else wyFalse
*/
wyBool			GetForeignKeyDetails(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam, const wyChar* dbname = NULL);

///Gets the Referenced table name
/**
@param fkeyinfo : IN Foreign key infos
@param parenttable : OUT gets the referenced table name
@return wyTrue while success , else wyFalse
*/
//wyBool			GetParentTable(wyString *fkeyinfo, wyString *parenttable, wyString* parenddb = NULL);
wyBool			GetParentTable(wyString *fkeyinfo, wyString *parenttable, wyString* parenddb = NULL);

///Gets the Constraint name
/**
@param fkeyinfo : IN Foreign key infos
@param parenttable : OUT gets the referenced table name
@return wyTrue while success , else wyFalse
*/
wyBool			GetConstraintName(wyString *fkeyinfo, wyString *constraintname);

///Gets the parent table fields involved in relationship 
/**
@param fkeyinfo  : IN Foreign key infos
@param fkeyparam : OUT struct used to store all parsed infos in F-key details
@return wyTrue while success , else wyFalse
*/
wyBool			GetParentTableFlds(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam);

///Gets the child table fields involved in relationship 
/**
@param fkeyinfo  : IN Foreign key infos
@param fkeyparam : OUT struct used to store all parsed infos in F-key details
@return wyTrue while success , else wyFalse
*/
wyBool			GetChildTableFlds(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam);

///Gets the option selected for ON DELETE foreign key if any
/**
@param fkeyinfo  : IN Foreign key infos
@param fkeyparam : OUT struct used to store all parsed infos in F-key details
@param isupdate  : IN default parameter, wyTrue for ON UPDATE , wyFalse for ON DELETE  
@return NoOption ON DELETE is not selectd, or Cascade/SetNull/NoAction/Restrict , according to the one selected
*/
fkeyOption		GetOnFkeyOption(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam, wyBool isupdate = wyTrue);
//free up the memory .
/**
@param list  : IN linked list reference.
@return 
*/
void            ReleaseMemory(List *list);

void			commonlog(const char * buff);

//For writing Error to Log file
/**
@param message  : IN error message
@return wyBool
*/

wyBool			WriteToLogFile(wyChar *message);

//Sets the mysql_options during connecting
/**
@param conn		: IN pointer to ConnectionInfo struct
@param tunnel	: IN tunnel pointer
@param pmysql	: IN MySQL handle
@return VOID
*/
VOID			SetMySQLOptions(ConnectionInfo *conn, Tunnel *tunnel, PMYSQL pmysql, wyBool issetcharset = wyTrue);

// Executes mysql_options() with INIT_COMMAND
/**
@param tunnel	    : IN tunnel pointer
@param pmysql	    : IN MySQL handle
@param initcommands : IN string of commands to be passed to the mysql_options
@return VOID
*/
void    ExecuteInitCommands(MYSQL* mysql, Tunnel* tunnel, wyString& initcommands);

/// Gets the field inforamtion of the view
/**
@param buffer       : IN / OUT Buffer 
@param tunnel       : IN Tunnel pointer
@param view         : IN View name
@param res          : IN Mysql result pointer
@return wyBool
*/
wyBool         DumpViewStruct(wyString * buffer, Tunnel * tunnel, const wyChar *view, MYSQL_RES *res);

/// Gets the engiine for a particular table
/**
@param tunnel				: IN Tunnel pointer
@param mysql				: IN Mysql pointer
@param tablename			: IN table name  
@param dbname				: IN database name
&param strengine			: OUT function stores the Engine name to this wyString variable
@return wyTrue if it success else return wyFalse
*/
wyBool		  GetTableEngine(Tunnel * tunnel, MYSQL *mysql, const wyChar *tablename, const wyChar *dbname, wyString * strengine);

//Gets the open port in local host (for ssh port forwarding)
/**
@con : IN ConnectionInfo structure
@filetype : IN wyFile class pointer
@return 0 for succcess else error number
*/
#if ! defined COMMUNITY && defined _WIN32
wyInt32		  GetLocalEmptyPort(ConnectionInfo *con);
#endif

//Initialise the socket
/**
@return wyTrue
*/
wyBool		  InitWinSock();

///Handle the temp file for protecting the local port found
/**
@filetype : IN wyFile class pointer
@return wyTrue if temp file could be oponed, else return wyFalse(means some other thread/prcess already oponed it but not closed still)
*/
//#if ! defined COMMUNITY && defined _WIN32
#if defined _WIN32
wyBool			LockPlinkLockFile(wyFile *plinklock); 
#endif

#ifdef _WIN32
//Kills the chid process of a parent proc
/**
@param parentproc : IN Handle to parent process
@return VOID
*/
void			KillProcessTree(HANDLE parentproc);
#endif

void			InitConnectionDetails(ConnectionInfo *conn);

///The function is the equivalant of GetForeignKeyInfo. But it uses information_schema to get the details, one limitation of this is, it cannot see the options for constraints
/**
@param myres        : IN myres returned by GetConstraintRes
@param ptunnel      : IN tunnel pointer
@param relno        : IN 1 based index of the constraint
@param pfkinfoparam : OUT info about the constraint
@returns wyTrue on success, else wyFalse
*/
wyBool          GetForeignKeyInfo50(MYSQL_RES* myres, Tunnel* ptunnel, wyInt32 relno, FKEYINFOPARAM* pfkinfoparam = NULL);

///The function gets the constraints by executing a query against information_shcema
/**
@param ptunnel      : IN tunnel pointer
@param pmysql       : IN mysql pointer
@param dbname       : IN database name
@param tablename    : IN table name
@returns MySQL_RES pointer on success, else NULL
*/
MYSQL_RES*      GetConstraintRes(Tunnel* ptunnel, PMYSQL pmysql, const wyChar* dbname, const wyChar* tablename);

/// Checks whether the datatype is numeric of not
/*
@param datatype             :   IN  the datatype
*/
wyBool          IsDatatypeNumeric(wyString  &datatype);


/// Decoding of password
/**
@param  text         : IN String to decode
@returns wyTrue on success
*/
wyBool	DecodePassword_Absolute(wyString &text);

/// Encoding of password
/**
@param  text         : IN String to decode
@returns wyTrue on success
*/
wyBool	EncodePassword_Absolute(wyString &text);

/// Rotate string left , bitwise
/**
@param  str             : IN/OUT String to rotate
@returns void
*/
void	RotateBitRight(unsigned char *str);

/// Rotate string left , bitwise
/**
@param  str          : IN/OUT String to rotate
@returns void
*/
void    RotateBitLeft (unsigned char *str);

void    RemoveDefiner(wyString &text, const wyChar* pattern, wyInt32 extra);

void    RemoveBrackets(wyString &text, const wyChar* pattern);

/// Encryption of password
/**
@param  text         : IN String to decode
@returns wyTrue on success
*/
wyBool	EncodePassword(wyString &text);

/// Decoding of password
/**
@param  text         : IN String to decode
@returns wyTrue on success
*/
wyBool	DecodePassword(wyString &text);

//Encrypt the password
wyBool	MigrateAllPassword(wyString conn, wyString dirstr);

wyBool	MigratePassword(wyString conn, wyString dirstr, wyString &pwdstr);

wyBool	MigratePassword(wyString &pwdstr);

//void DebugLog(const char *buffer);
#ifdef _WIN32
void			WriteLog(const wyChar* str);
#endif

#ifdef _WIN32

class ConvertString
{
    public:
        
		ConvertString();
        ~ConvertString();

        /// Converts the input(wide) string to Utf8 
        /**
        @param widestr : IN widestring to convert
        @returns utf8 string
        */
        wyChar* ConvertWideCharToUtf8(wyWChar *widestr);

        /// converts input (utf8) string to widechar
        /**
        @param utf8str : IN utf8 string to convert
        @retuns wide string 
        */
        wyWChar*  ConvertUtf8ToWideChar(wyChar *utf8str);

		wyWChar * ConvertAnsiToWideChar(wyChar* ansistr);
		wyChar*	  Utf8toAnsi(const wyChar *utf8, wyInt32 len);		

    private:
        /// utf8 buffer
        wyChar  *m_utf8str;

		/// utf8 buffer
        wyChar  *m_ansistr;

        /// wide char buffer
        wyWChar   *m_widestr;
};

#endif // _win32

#endif

    
