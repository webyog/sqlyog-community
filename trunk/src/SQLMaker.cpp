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


/*! \file SqlMaker.Cpp
    \brief Implementation of all functions declared in SqlMaker.Cpp
    The functions in this file helps in framing the SQL query as well as get it executed.    
*/
#include <winsock.h>
#include <mysql.h>
#include "SQLMaker.h"
#include "FrameWindowHelper.h"
#include "ClientMySQLWrapper.h"


wyUInt32 
GetTableStatusStmt(wyString &query, const wyChar *dbname)
{
    wyInt32 retcode;

    retcode = query.Sprintf("show table status from `%s`", dbname);

    return retcode;
}

wyUInt32 
GetFullFieldsStmt(wyString &query, const wyChar *dbname, const wyChar *tblname)
{
    wyInt32 retcode;

    retcode = query.Sprintf("show full fields from `%s`.`%s`", dbname, tblname);

    return retcode; 
}

wyUInt32 
GetKeysStmt(wyString &query, const wyChar *dbname, const wyChar *tblname)
{
    wyInt32 retcode;

    retcode = query.Sprintf("show keys from `%s`.`%s`", dbname, tblname);

    return retcode; 
}

wyUInt32 
GetSelectTableStmt(wyString &query, const wyChar *dbname)
{
    wyInt32 retcode;
	
	//retcode = query.Sprintf("select `TABLE_NAME` from `INFORMATION_SCHEMA`.`TABLES` where `TABLE_SCHEMA` = '%s' and `TABLE_TYPE` = 'BASE TABLE'", dbname );
    retcode = query.Sprintf("show table status from `%s` where engine is not NULL", dbname );
    //retcode = query.Sprintf("show table status from `%s` where comment != 'view'", dbname );
	return retcode;
}

MYSQL_RES *
ExecuteAndGetResult(MDIWindow *wnd, Tunnel *tunnel, PMYSQL mysql, wyString &query, 
					wyBool isprofile, wyBool isbatch, wyBool currentwnd, bool isread, 
					bool isforce, wyBool isimport, wyInt32 *stop, wyBool isimporthttp)
{
    wyInt32     retcode;
    MYSQL_RES   *myres = NULL;

	if(wnd)
		wnd->SetThreadBusy(wyTrue);

    //if(isprofile == wyTrue)
    //{
        retcode = my_query(wnd, tunnel, mysql, query.GetString(), query.GetLength(), isbatch, 
			wyFalse, stop, 0, isprofile, currentwnd, isread, isimport, isimporthttp);
    //}
    /*else
    {
        wyBool isbadforxml = IsBadforXML(query.GetString());
		retcode = HandleMySQLRealQuery(tunnel, *mysql, query.GetString(), query.GetLength(), isbadforxml ? true : false, isbatch,
			                           false, 0, isread);
    }*/

    if(retcode)
	{
		if(wnd)
			wnd->SetThreadBusy(wyFalse);
        return myres;
	}
            
    myres = tunnel->mysql_store_result(*mysql, (isprofile == wyTrue)?false:true, isforce, wnd);
	if(wnd)
		wnd->SetThreadBusy(wyFalse);
    return myres;
}

MYSQL_RES *
ExecuteAndUseResult(Tunnel *tunnel, PMYSQL mysql, wyString &query)
{
    wyInt32     retcode;
    MYSQL_RES   *myres = NULL;
    
	retcode = HandleSjaMySQLRealQuery(tunnel, *mysql, query.GetString(), query.GetLength());
 
	if(retcode)
        return myres;
            
    myres = sja_mysql_use_result(tunnel, *mysql);
    
    return myres;
}