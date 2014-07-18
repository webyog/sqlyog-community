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

#include "wyFile.h"
#include "wySqlite.h"
#include "CommonHelper.h"

// misc
#define SELECT_TBL_COUNT "SELECT COUNT(*) as cnt \
FROM [sqlite_master] WHERE [type] = 'table' and [name] = ?"

#define SQLITE_BUSY_TRIES	18000 // 3 minute (18000 * 10)
#define SQLITE_BUSY_SLEEP	10
#define SQLITE_BUSY_TRYCODE	2001 // our own code

ySqlite::ySqlite() 
{
    uSqlite = NULL;
    uIsAutoClose = wyTrue;
    uDbName.Clear();
}

ySqlite::~ySqlite() 
{
    // create one object of sqlite and set the autoclose mode ON. So the connection will
    // be closed by the destructor and logging will work.
    wySQLite temp(this);
    temp.AutoCloseModeSet(wyTrue);
}

wySQLite::wySQLite() 
{
    db_name_.Clear();
    sql_.Clear();
    Init();
}

wySQLite::wySQLite(wyChar * db_name, wyChar * sql): db_name_(db_name), sql_(sql) 
{
    Init();
}

wySQLite::wySQLite(ySqlite* sqlite) 
{	
    InitSqlite(sqlite);	
}

wySQLite::~wySQLite() 
{
    if(sqlite_stmt_) 
    {
        sqlite3_finalize(sqlite_stmt_);
    }

    if(sqlite_ && (is_auto_close_ == wyTrue))
    {
        sqlite3_close(sqlite_);
    }
}

void wySQLite::Init() 
{
    sqlite_     = NULL;
    sqlite_stmt_ = NULL;
    is_auto_close_ = wyTrue;
    is_logging = wyFalse;
}

// Initialize the sqlite pointers and other class level params
// Parameters
//      sqlite: ySqlite class pointer
// Returns:
//		void
void wySQLite::InitSqlite(ySqlite* sqlite) 
{
    sqlite_stmt_ = NULL;
    sqlite_     = sqlite->uSqlite;
    is_auto_close_ = sqlite->uIsAutoClose;
    db_name_.SetAs(sqlite->uDbName);
    is_logging = wyFalse;
}

//// Sets the Log File object pointer
//// Parameters
////      log: log pointer
//void wySQLite::SetLog(wyLog* log, wyLog* sqlite_profile) {
//	log_ = log;
//	if(sqlite_profile)
//		log_sqlite_profile_ = sqlite_profile;
//	else
//		log_sqlite_profile_ = NULL;
//}
//
//// Sets the Query Log File object pointer
//// Parameters
////      log: log pointer
//void	wySQLite::SetQueryLog(wyQueryLog* query_log) {
//	query_log_ = query_log;
//}

// Sets the Database, query and log file
// Parameters
//      yChar * db_name
//      yChar * sql
//      log: log pointer        
void wySQLite::SetValues(wyChar* db_name, wyChar* sql) 
{
    db_name_.SetAs(db_name);
    sql_.SetAs(sql);
    Init();
}
// closes the SQLite connection
void wySQLite::Close () 
{
    if(sqlite_stmt_) 
    {
        sqlite3_finalize(sqlite_stmt_);
    }

    if(sqlite_) 
    {
        sqlite3_close(sqlite_);
    }

    sqlite_ = NULL;
    sqlite_stmt_ = NULL;

    db_name_.Clear();
    sql_.Clear();	
}

void wySQLite::Profile(void * arg, const wyChar* query, sqlite3_uint64 time_taken) 
{
    /*UNREFERENCED_PARAMETER(arg);
    UNREFERENCED_PARAMETER(query);
    UNREFERENCED_PARAMETER(time_taken);
    wySQLite * sqlite = (wySQLite *)arg;
    double	   timex = time_taken / 1000000000.0;	 // Nano second
    //float timex = pTimeTaken / 1000000000.0;	 // Nano second

    if(sqlite->log_sqlite_profile_ && timex > 1.0)
    {
        LOGP(sqlite->log_sqlite_profile_, 
             sqlite->db_name_.GetString(),
             query, 
             time_taken
        );			
    }*/
}

// Opening a new database connection as well as prepares the SQl stmt
// Parameters
//      stmt: prepared stmt[OUT]
// Returns 
//      TRUE if success, FLASE otherwise
wyBool wySQLite::OpenAndPrepare(sqlite3_stmt **stmt) 
{
    wyChar* unused;
    wyBool isdbexists;
    sqlite3_stmt* sstmt;	
    wyFile sqlitefile;
        
#ifdef _WIN32
    UNREFERENCED_PARAMETER(isdbexists);
#else
    sqlitefile.SetFilename(db_name_.GetString());
    isdbexists = sqlitefile.CheckIfFileExists(db_name_.GetString());
#endif

    ret_ = sqlite3_open(db_name_.GetString(), &sqlite_); 
    //LOG3(log_, 1000, "sqlite3_open called!"); //SABYA

    if (ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }

        return wyFalse;
    }

    ////Profiling information, wySQLite::Profile
    //if(log_sqlite_profile_)
    //	sqlite3_profile(sqlite_, wySQLite::Profile, this);
#ifdef _WIN32
  UNREFERENCED_PARAMETER(isdbexists);
#else
    // If DB is not already present then its a new DB so set the permission accordingly
    if (isdbexists == wyFalse)
    {
        sqlitefile.SetFilePermission(db_name_.GetString());
    }
#endif //_WIN32

    // set the default timeout
    sqlite3_busy_timeout(sqlite_, DEF_SQLITE_TIMEOUT);
    //sqlite3_busy_handler(sqlite_, wySQLite::DBBusyHandler, this);

    ret_ = sqlite3_prepare(sqlite_, sql_.GetString(),(wyInt32)sql_.GetLength()*sizeof(wyChar), 
                          &sstmt,(const wyChar **)&unused);

    if(ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }
    }

    if(stmt)
    {
        *stmt = sstmt;
    }
    else
    {
        sqlite_stmt_ = sstmt;
    }

    return (ret_ == SQLITE_OK) ? wyTrue : wyFalse;
}

// This function is used to do a one-time evaluatation of a SQL query
// Parameters
//      query: query to execute
//      err_msg: Error message[OUT]
// Returns
//      TRUE if SUCCESS, FALSE otherwise
wyBool wySQLite::Execute(wyString *query, wyString *err_msg) 
{
    wyChar *errmsg;

    ret_ = sqlite3_exec(sqlite_, (wyChar*) query->GetString(), NULL, 0, &errmsg);
    if(ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }

        err_msg->SetAs(errmsg);
    }

    return (ret_ == SQLITE_OK) ? wyTrue : wyFalse;
}

// Opening a new database connection
// Parameter
//      db_name: Database name
// Returns
//      TRUE if SUCCESS, FALSE otherwise
wyBool wySQLite::Open(const wyChar * db_name, wyBool isreadonlymode) 
{
    wyBool isdbexists = wyTrue;
    wyFile sqlitefile; 

    db_name_.SetAs(db_name);

#ifdef _WIN32
    UNREFERENCED_PARAMETER(isdbexists);
#else
    isdbexists = sqlitefile.CheckIfFileExists(db_name);
#endif

    if (isreadonlymode == wyFalse) 
    {	
        ret_ = sqlite3_open(db_name_.GetString(), &sqlite_);     
    } 
    else 
    {
        ret_ = sqlite3_open_v2(db_name_.GetString(), &sqlite_, SQLITE_OPEN_READONLY, NULL);     
    }

    //LOG3(log_, 1000, "sqlite3_open called!"); //SABYA
    
    if (ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }

        return wyFalse;
    }


#ifdef _WIN32
    UNREFERENCED_PARAMETER(isdbexists);
#else
    // If DB is not already present then its a new DB so set the permission accordingly
    if (isdbexists == wyFalse) 
    {
        sqlitefile.SetFilePermission(db_name);
    }
#endif //_WIN32
    // set the default connection timeout
    sqlite3_busy_timeout(sqlite_, DEF_SQLITE_TIMEOUT);
    //sqlite3_busy_handler(sqlite_, wySQLite::DBBusyHandler, this);

    return wyTrue;
}

// Prepares the SQL query, it converts the SQL query to byte-code 
// Parameters
//      stmt: prepared stmt[OUT]
//      sql_stmt: SQL query
// Returns
//      TRUE if SUCCESS, FALSE otherwise
wyBool wySQLite::Prepare(sqlite3_stmt **stmt, const wyChar *sql_stmt) 
{	
    wyChar* unused;
    sqlite3_stmt *sstmt;

    sql_.SetAs(sql_stmt);
    

    ret_ = sqlite3_prepare(sqlite_, sql_.GetString(),(wyInt32)sql_.GetLength() * sizeof(wyChar), 
                         &sstmt, (const char **)&unused);

    if(ret_ == SQLITE_SCHEMA)	
    {
        ret_ = sqlite3_prepare(sqlite_, sql_.GetString(),(wyInt32)sql_.GetLength() * sizeof(wyChar), 
                           &sstmt, (const char **)&unused);
    }

    if(ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }
    }

    if (stmt)
    {
        *stmt = sstmt;
    }
    else
    {
        sqlite_stmt_ = sstmt;
    }

    return (ret_ == SQLITE_OK) ? wyTrue : wyFalse;
}

// This function is used to evaluate the prepared stmt
// Parameters
//      stmt: Stmt to evaluate
// Returns
//      TRUE if Success, FALSE otherwise
wyBool wySQLite::Step(sqlite3_stmt **stmt, wyBool is_logging) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    ret_ = sqlite3_step(sstmt);

    if(ret_ != SQLITE_DONE && ret_ != SQLITE_ROW) 
    {
        if(ret_ == SQLITE_ERROR) 
        {
            Reset(&sstmt);
        }

        if(is_logging == wyTrue)
        {
            if(is_logging == wyTrue)
            {
                YOGLOG(GetLastCode(), GetErrMsg());
            }
        }
    }

    return (ret_ == SQLITE_DONE || ret_ == SQLITE_ROW) ? wyTrue : wyFalse;
}

// Gets the last error code
// Returns 
//      Error code
wyInt32 wySQLite::GetLastCode() 
{
    return ret_;
}

// Gets the SQLite error message
// Returns
//      err msg
const wyChar* wySQLite::GetErrMsg() 
{	
    err_msg_.Sprintf("[DB: %s] %s", db_name_.GetString(), (wyChar *) sqlite3_errmsg(sqlite_));
    return err_msg_.GetString();
}

// Gets a text value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col_name: column name
// Returns
//      column value
const wyChar* wySQLite::GetText(sqlite3_stmt** stmt, wyInt32 col) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    // during utf-conversion - sqlite3_column_text its returning 
    return(const wyChar*)sqlite3_column_text(sstmt, col);
}

// Gets a text value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col_name: column name
// Returns
//      column value
const wyChar * wySQLite::GetText(sqlite3_stmt** stmt, const wyChar* col_name)
{    
    return GetText(stmt, GetFieldIndex(stmt, col_name) );
}

// This function will return number of clumns returned in the result set
// Parameters
//      stmt: prepared stmt
// Returns
//      number of columns returned
wyInt32 wySQLite::GetColumnCount(sqlite3_stmt** stmt) 
{
    sqlite3_stmt *sstmt;

    if (stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    return sqlite3_column_count(sstmt);
}

// returns the column name from the resultset with the given index
// Parameters
//      stmt: prepared stmt
//      PIndex: column name
// Returns
//      column name
const wyChar * wySQLite::GetColumnName(sqlite3_stmt **stmt, wyInt32 index) 
{
    sqlite3_stmt *sstmt;
    
    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }
    
    return sqlite3_column_name(sstmt, index);
}

// this function returns the correct index by taking the compiled stmt and the column names 
// Parameters
//      stmt: prepared stmt
//      field_name: fieldname
// Returns
//      field index if found, -1 otherwise
wyInt32  wySQLite::GetFieldIndex(sqlite3_stmt **stmt, const wyChar *field_name) 
{
    sqlite3_stmt *sstmt;
    wyInt32  colcount;
    wyInt32  cnt      = 0;
    const wyChar   *colname = NULL;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    colcount =  sqlite3_column_count(sstmt);
    
    for(; cnt < colcount; cnt++) 
    {
        colname = NULL;
        colname = sqlite3_column_name(sstmt, cnt);
        
        if(colname) 
        {
            if(strcmp(colname, field_name) == 0)
            {
                return cnt;
            }
        }
    }

    //assert(0); // should not come here

    return -1;
}

// Gets a binary value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col: column index
// Returns
//      column value
const wyChar* wySQLite::GetBlob(sqlite3_stmt **stmt, wyInt32 col) 
{
    sqlite3_stmt *sstmt;

    if (stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    // during utf-conversion - sqlite3_column_text its returning 
    return(const wyChar*)sqlite3_column_blob(sstmt, col);
}

// Gets a binary value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col: column index
// Returns
//      column value
const wyChar* wySQLite::GetBlob(sqlite3_stmt **stmt, const wyChar *col_name) 
{
    return GetBlob(stmt, GetFieldIndex(stmt, col_name));
}

// Gets a binary value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col: column index
// Returns
//      column value
wyInt32 wySQLite::GetBlobLength(sqlite3_stmt **stmt, wyInt32 col) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    // during utf-conversion - sqlite3_column_text its returning 
    return(wyInt32)sqlite3_column_bytes(sstmt, col);
}

// Gets an Int value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col: column index
// Returns
//      column value
wyInt32 wySQLite::GetInt(sqlite3_stmt **stmt, wyInt32 col) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    return sqlite3_column_int(sstmt, col);
}

// Gets an Int value from the current result row of a query
// Parameters
//      stmt: prepared stmt
//      col_name: column name
// Returns
//      column value
wyInt32 wySQLite::GetInt(sqlite3_stmt **stmt, const wyChar *col_name) 
{
    return GetInt(stmt, GetFieldIndex(stmt, col_name));
}

// Bind a text value to prepared stmt
// Parameters
//      stmt: prepared stmt
//      col: column index to bind
//      val: value to bind to the parameter
// Returns 
//      void
void wySQLite::SetText(sqlite3_stmt **stmt, wyInt32 col, wyString * val) 
{		
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    ret_ = sqlite3_bind_text(sstmt, col, val->GetString(),
                                 (wyInt32)val->GetLength() * sizeof(wyChar), 
                                 SQLITE_TRANSIENT);
  
    if(ret_ != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(GetLastCode(), GetErrMsg());
        }
    }
}

// Bind a text value to prepared stmt
// Parameters
//      stmt: prepared stmt
//      col: column index to bind
//      val: value to bind to the parameter
// Returns 
//      void
void wySQLite::SetText(sqlite3_stmt **stmt, wyInt32 col, const wyChar* val) 
{
    wyString value(val);

    SetText(stmt, col, &value);
}

// Bind a yInt32 value to prepared stmt
// Parameters
//      stmt: prepared stmt
//      col: column index to bind
//      val: value to bind to the parameter
// Returns 
//      void
void wySQLite::SetInt(sqlite3_stmt **stmt, wyInt32 col, wyInt32 val) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    sqlite3_bind_int(sstmt, col, val);
}

// Bind a __int64 value to prepared stmt
// Parameters
//      stmt: prepared stmt
//      col: column index to bind
//      val: value to bind to the parameter
// Returns 
//      void
void wySQLite::SetInt64(sqlite3_stmt **stmt, wyInt32 col, wyInt64 val) 
{
    sqlite3_stmt *sstmt;

    if (stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    sqlite3_bind_int64(sstmt, col, val);
}

// Bind a Null value to prepared stmt
// Parameters
//      stmt: prepared stmt
//      col: column index to bind
// Returns 
//      void
void wySQLite::SetNull(sqlite3_stmt **stmt, wyInt32 col) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    sqlite3_bind_null(sstmt, col);
}

// Reset a prepared stmt if passed as argument, otherwise reset current stmt 
// Parameters
//      stmt: prepared stmt
// Returns 
//      void
void wySQLite::Reset(sqlite3_stmt **stmt)
{
    sqlite3_stmt *sstmt;

    if (stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    sqlite3_reset(sstmt);
}


// It will delete the prepared stmt if passed as argument, otherwise current stmt is deleted
// Parameters
//      stmt: prepared stmt
// Returns 
//      void
void wySQLite::Finalize(sqlite3_stmt **stmt) 
{
    sqlite3_stmt *sstmt;

    if(stmt)
    {
        sstmt = *stmt;
    }
    else
    {
        sstmt = sqlite_stmt_;
    }

    if(sstmt)
    {
        sqlite3_finalize(sstmt);
    }

    if(stmt)
    {
        *stmt = NULL;
    }
    else
    {
        sqlite_stmt_ = NULL;
    }
}

// Get the curretn SQLite DB name
// Returns 
//      DB name
const wyChar* wySQLite::GetDbName() 
{
    return db_name_.GetString();
}


/*
//SABYA: comment here
void		wySQLite::SqliteConnectionSet(ySqlite *sqlite)
{
    sqlite_ = sqlite->uSqlite;
    //SABYA the db_name_ is not set!
}
*/

wyBool wySQLite::SqliteConnectionGetNew(const wyChar* db_name, 
                                        ySqlite* sqlite,
                                        wyBool is_auto_close,
                                        wyBool is_logging)
{
    wyInt32 ret = 0;
    wyBool isdbexists = wyTrue;
    wyFile sqlitefile; 

#ifdef _WIN32
    UNREFERENCED_PARAMETER(isdbexists);
#else
    isdbexists = sqlitefile.CheckIfFileExists((wyChar*) db_name);
#endif

    ret = sqlite3_open(db_name, &(sqlite->uSqlite));
    
    if(ret != SQLITE_OK) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(ret, sqlite3_errmsg(sqlite->uSqlite));
        }

        sqlite3_close(sqlite->uSqlite);
        sqlite = NULL;
        return wyFalse;
    }
    
#ifdef _WIN32
#else
    // If DB is not already present then its a new DB so set the permission accordingly
    if(isdbexists == wyFalse) 
    {
        sqlitefile.SetFilePermission((wyChar*) db_name);
    }
#endif //_WIN32

    // set the default timeout
    sqlite3_busy_timeout(sqlite->uSqlite, DEF_SQLITE_TIMEOUT);

    sqlite->uDbName.SetAs(db_name);
    sqlite->uIsAutoClose = is_auto_close;
    return wyTrue;
}

// Set the Auto Close Mode of the wySqlite object. If Auto Close Mode is TRUE then the destructor will
// Automatically close the connection.
// Parameters:
//		yBool pMode: The mode to set
// Returns:
//		void
void wySQLite::AutoCloseModeSet(wyBool mode) 
{
    is_auto_close_ = mode;
}

void wySQLite::ClearBindings(sqlite3_stmt **stmt) {
  sqlite3_stmt *sstmt;

    if (stmt)
        sstmt = *stmt;
    else
        sstmt = sqlite_stmt_;

  sqlite3_clear_bindings(sstmt);
}

/* Commented for future use
yInt32 wySQLite::DBBusyHandler(void * pWySQLite, yInt32 pTries)
{
    wySQLite	* wysqlite = (wySQLite *)pWySQLite; 
    wyLog		* log = wysqlite->GetLog();
    wyString	busylog;
    
    printf("\n%s\n%s\n", wysqlite->sql_.GetString(), wysqlite->db_name_.GetString());
    
    wyUtil::MilliSleep(SQLITE_BUSY_SLEEP);	

    busylog.Sprintf("Tries: %d, Waited Time: %d", pTries, pTries * SQLITE_BUSY_SLEEP);

    LOG3(log, SQLITE_BUSY_TRYCODE, busylog.GetString());
    LOG3(log, SQLITE_BUSY_TRYCODE, wysqlite->db_name_.GetString());
    LOG3(log, SQLITE_BUSY_TRYCODE, wysqlite->sql_.GetString());

    if (pTries >= SQLITE_BUSY_TRIES) // now return BUSY
        return 0;

    // check whether lock is removed!
    return 1;
}

wyLog *  wySQLite::GetLog()
{
    return log_;
}
*/

// returns the last insert Row id in the current connection
wyInt64 wySQLite::GetLastInsertRowId() 
{
    return sqlite3_last_insert_rowid(sqlite_);
}

// Returns number of chnaged rows.
wyInt32 wySQLite::GetNumberOfRowsChanged() 
{
    return sqlite3_changes(sqlite_);
}
                        
// checks for a table exists or not
wyBool wySQLite::IsTableExists(const wyChar* table) 
{
    wyString query;
    wyBool ret = wyFalse;

    if(Prepare(NULL, SELECT_TBL_COUNT) == FALSE) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(-1, table);
        }

        return wyFalse;
    }

    SetText(NULL, 1, table);

    if(Step(NULL) == FALSE) 
    {
        if(is_logging == wyTrue)
        {
            YOGLOG(-1, table);
        }

        return wyFalse;
    }

    if(GetLastCode() == SQLITE_ROW) 
    {
        ret = (GetInt(NULL, "cnt") == 1)? wyTrue: wyFalse;
    }

    Finalize(NULL);
    return ret;
}

void
wySQLite::SetLogging(wyBool is_logging)
{
    this->is_logging = is_logging;
}

