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


#ifndef _SQLMAKER_H_
#define _SQLMAKER_H_

#include "MDIWindow.h"
#include "Datatype.h"
#include "Tunnel.h"
#include "wyString.h"


/// Frames the show table status statement
/**
@param query		: OUT The resultant query
@param dbname		: IN Database name
@returns wyUInt32 The resultant query length
*/
wyUInt32 GetTableStatusStmt(wyString &query, const wyChar *dbname);

/// Frames the show full fields statement
/**
@param query		: OUT The resultant query
@param dbname		: IN Database name
@param tblname		: IN Table name
@returns wyUInt32 The resultant query length
*/
wyUInt32 GetFullFieldsStmt(wyString &query, const wyChar *dbname, const wyChar *tblname);

/// Frames the show keys statement
/**
@param query		: OUT The resultant query
@param dbname		: IN Database name
@param tblname		: IN Table name
@returns wyUInt32 The resultant query length
*/
wyUInt32 GetKeysStmt(wyString &query, const wyChar *dbname, const wyChar *tblname);

/// Gets the select table statement
/**
@param query		: IN Query
@param dbname		: IN Database name
*/
wyUInt32 GetSelectTableStmt(wyString &query, const wyChar *dbname);

/// Executes the query and returns the result set
/**
@param wnd          : Current MDIWindow pointer
@param tunnel		: IN Tunnel pointer
@param mysql		: IN PMYSQL value
@param query		: IN Query to be executed
@param isforse		: IN def. parameter tells whether query to be forcebly execute or not
@param stop         : IN Specific to http tunnel stopping
@returns MYSQL_RES*, it will be NULL if the query fails or query returns no resultset
*/
MYSQL_RES *ExecuteAndGetResult(MDIWindow *wnd, Tunnel *tunnel, PMYSQL mysql, wyString &query, 
							   wyBool isprofile = wyTrue, wyBool isbatch = wyFalse, wyBool currentwnd = wyTrue, 
							   bool isread = false, bool isforce = false, wyBool isimport = wyFalse, 
							   wyInt32 *stop = 0, wyBool isimporthttp = wyFalse);

/// Executes the query and returns the result set
/**
@param tunnel		: IN Tunnel pointer
@param mysql		: IN PMYSQL value
@param query		: IN Query to be executed
@returns MYSQL_RES*, it will be NULL if the query fails or query returns no resultset
*/
MYSQL_RES *ExecuteAndUseResult(Tunnel *tunnel, PMYSQL mysql, wyString &query);

#endif