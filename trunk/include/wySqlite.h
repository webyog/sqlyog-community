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

#ifndef _WYSQLITE_H
#define _WYSQLITE_H

#include "Datatype.h"
#include "wyString.h"
#include "sqlite3.h"

class ySqlite {
 public:
	ySqlite();
	~ySqlite();

	sqlite3* uSqlite;
	wyBool	uIsAutoClose;
	wyString uDbName;
};


#define	DEF_SQLITE_TIMEOUT					180000 //[From 2.06 it is changed from 100000 to 180000]

class wySQLite {
 public:
	wySQLite();
	//wySQLite(wyLog* log, wyLog* sqlite_profile = NULL);
	wySQLite(wyChar* db_name, wyChar* sql);
	//wySQLite(wyChar* db_name, wyChar * sql, wyLog * log, wyLog* sqlite_profile = NULL);
	wySQLite(ySqlite* sqlite);
	~wySQLite();
	
	void Init();
	void SetLog();
	void SetQueryLog();
	void SetValues(wyChar* db_name, wyChar* sql);
	// Opening a new database connection as well as prepares the SQL stmt
	// Parameters
	//      pStmt: prepared stmt[OUT]
	// Returns 
	//      TRUE if success, FLASE otherwise
	wyBool OpenAndPrepare(sqlite3_stmt** stmt);

	// Opening a new database connection
	// Parameter
	//      pDbName: Database name
	// Returns
	//      TRUE if SUCCESS, FALSE otherwise
	wyBool       Open(const wyChar* db_name, wyBool isreadonlymode = wyFalse);

	// Set the connection of this class by passing a parameter to a sqlite connection.
	// This parameter should be a valid connection to Sqlite, usually initialzed :-
	// void		GetSqliteConnection(yChar *dbname);
	// Parameter
	//      ySqlite *: The valid Sqlite connection
	// Returns
	//      void

	void		SqliteConnectionSet(ySqlite* sqlite);

	// Get a valid sqlite connection. Helpful when you want to get a sqlite connection that should be
	// shared by many instances of this class. 
	// Usually used to get a connection, that should be passed to the following funtion:
	// void		SetSqliteConnection(ySqlite *pSqlite);
	// Parameter
	//      yCChar *pDbName: The path to the SQLite database
	//		ySqlite* pSqlite: The out pur sqlite structure
	//		wyLog* pLog:	Pointer to the log handler
	//		yBool pIsAutoClose = TRUE: If it is true the connection will be closed by the destructors of instances of wySQLite, who uses this connection
	// Returns
	//      ySqlite*: If successful a Connection to the SQLite sturcter. Else NULL.

	 static wyBool	SqliteConnectionGetNew(const wyChar* db_name, ySqlite* sqlite, wyBool is_auto_close = wyFalse, wyBool is_logging = wyFalse);

	// Prepares the SQL query, it converts the SQL query to byte-code 
	// Parameters
	//      pStmt: prepared stmt[OUT]
	//      pSqlStmt: SQL query
	// Returns
	//      TRUE if SUCCESS, FALSE otherwise
    wyBool Prepare(sqlite3_stmt** stmt, const wyChar* sql_stmt);

	// This function is used to evaluate the prepared stmt
	// Parameters
	//      pStmt: Stmt to evaluate
	// Returns
	//      TRUE if Success, FALSE otherwise
	wyBool Step(sqlite3_stmt** Stmt, wyBool is_logging = wyTrue);

	// Gets the last error code
	// Returns 
	//      Error code
	wyInt32 GetLastCode();

	// Gets the SQLite error message
	// Returns
	//      err msg
	const wyChar* GetErrMsg();

	// Gets a text value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pColName: column name
	// Returns
	//      column value
	const wyChar* GetText(sqlite3_stmt **pStmt, wyInt32 pCol);

	// Gets a text value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pColName: column name
	// Returns
	//      column value
	const wyChar* GetText(sqlite3_stmt **pStmt, const wyChar * pColName);

	// Gets a binary value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index
	// Returns
	//      column value
    const wyChar* GetBlob(sqlite3_stmt **pStmt, wyInt32 pCol);

	// Gets a binary value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index
	// Returns
	//      column value
	const wyChar* GetBlob(sqlite3_stmt **pStmt, const wyChar * pColName);
	
	// Gets an Int value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index
	// Returns
	//      column value
	wyInt32 GetInt(sqlite3_stmt **pStmt, wyInt32 pCol);

	// Gets an Int value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pColName: column name
	// Returns
	//      column value
	wyInt32      GetInt(sqlite3_stmt **pStmt, const wyChar * pColName);

	// This function is used to do a one-time evaluatation of a SQL query
	// Parameters
	//      pQuery: query to execute
	//      pErrMsg: Error message[OUT]
	// Returns
	//      TRUE if SUCCESS, FALSE otherwise
    wyBool Execute(wyString *pQuery, wyString *pErrMsg);

	// Gets a binary value from the current result row of a query
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index
	// Returns
	//      column value
    wyInt32 GetBlobLength(sqlite3_stmt **pStmt, wyInt32 pCol);

	// Bind a text value to prepared stmt
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index to bind
	//      pVal: value to bind to the parameter
	// Returns 
	//      void
	void SetText(sqlite3_stmt **pStmt, wyInt32 pCol, wyString * pVal);

	// Bind a text value to prepared stmt
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index to bind
	//      pVal: value to bind to the parameter
	// Returns 
	//      void
	void SetText(sqlite3_stmt **pStmt, wyInt32 pCol, const wyChar *pVal);

	// Bind a yInt32 value to prepared stmt
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index to bind
	//      pVal: value to bind to the parameter
	// Returns 
	//      void
	void SetInt(sqlite3_stmt **pStmt, wyInt32 pCol, wyInt32 pVal);

	// Bind a __int64 value to prepared stmt
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index to bind
	//      pVal: value to bind to the parameter
	// Returns 
	//      void
	void SetInt64(sqlite3_stmt **pStmt, wyInt32 pCol, wyInt64 pVal);

	// Bind a NULL value to prepared stmt
	// Parameters
	//      pStmt: prepared stmt
	//      pCol: column index to bind
	// Returns 
	//      void
	void SetNull(sqlite3_stmt **pStmt, wyInt32 pCol);
	

	// Reset a prepared stmt if passed as argument, otherwise reset current stmt 
	// Parameters
	//      pStmt: prepared stmt
	// Returns 
	//      void
	void Reset(sqlite3_stmt **pStmt);

	// It will delete the prepared stmt if passed as argument, otherwise current stmt is deleted
	// Parameters
	//      pStmt: prepared stmt
	// Returns 
	//      void
   void Finalize(sqlite3_stmt **pStmt);

	// Get the curretn SQLite DB name
	// Returns 
	//      DB name
    const wyChar* GetDbName();

	// this function returns the correct index by taking the compiled stmt and the column names 
	// Parameters
	//      pStmt: prepared stmt
	//      pFieldName: fieldname
	// Returns
	//      field index if found, -1 otherwise
    wyInt32 GetFieldIndex(sqlite3_stmt **pStmt, const wyChar *pFieldName);

	// This function will return number of clumns returned in the result set
	// Parameters
	//      pStmt: prepared stmt
	// Returns
	//      number of columns returned
    wyInt32 GetColumnCount(sqlite3_stmt **pStmt);

	// returns the column name from the resultset with the given index
	// Parameters
	//      pStmt: prepared stmt
	//      PIndex: column name
	// Returns
	//      column name
    const wyChar* GetColumnName(sqlite3_stmt **pStmt, wyInt32 pIndex);

	// Set the Auto Close Mode of the wySqlite object. If Auto Close Mode is TRUE then the destructor will
	// Automatically close the connection.
	// Parameters:
	//		yBool pMode: The mode to set
	// Returns:
	//		void
	void AutoCloseModeSet(wyBool pMode);

	// Returns the last inserted Row id in the current connection
	// Parameters
	//      none
	// Returns
	//      yInt64 Row Id
    wyInt64 GetLastInsertRowId();

	// This function returns the number of database rows that were changed or inserted or deleted by the most recently completed SQL statement
	// Parameters
	//      none
	// Returns
	//      yInt32 number of changed rows

    wyInt32 GetNumberOfRowsChanged();

	// closes the SQLite connection
	void	Close ();

	// Clear The Bindings on a prepare statement.
	// Parameters
	//      pStmt: prepared stmt
	// Returns:
	//		void
    void ClearBindings(sqlite3_stmt **pStmt);

	// Initialize the sqlite pointers and other class level params
	// Parameters
	//      pSqlite: ySqlite class pointer
	// Returns:
	//		void
	void InitSqlite(ySqlite * pSqlite);

	// Initialize the sqlite pointers and other class level params
  // Parameters
  //      pSqlite: ySqlite class pointer
  // Returns:
  //		void
	static void Profile(void* arg, const wyChar* query, sqlite3_uint64 time_taken);

	//it will tell whether table exist or not with given name in the sqlite db
	// Parameters
	//      table: table name to which we are finding the table existance.
	// Returns:
	//		TRUE or FALSE.
    wyBool	IsTableExists(const wyChar*  table);

	void SetLogging(wyBool is_logging);

 private:
	wyString err_msg_;
	wyString db_name_;
	wyString sql_;
	wyInt32 ret_;

	sqlite3* sqlite_;
	sqlite3_stmt*	sqlite_stmt_;
	wyBool	is_auto_close_; //If set to TRUE, the destructor will call sqlite3_close on sqlite_
    wyBool  is_logging;
};
#endif