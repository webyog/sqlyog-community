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


#include "DBListBuilder.h"
#include "Global.h"
#include "Verify.h"
#include "FrameWindowHelper.h"
#include "GUIHelper.h"


extern	PGLOBALS		pGlobals;

/*	Function implementation of CGetDB class.
	This class just gets unique database and adds it to the combo whose handle is passed
	as parameter	*/

DBListBuilder::DBListBuilder()
{

}

DBListBuilder::~DBListBuilder()
{

}

void
DBListBuilder::GetDBs(HWND hwndcombo, wyBool isrefreshtrue)
{
	 // now we call the enum function and fill the combo with the databases.
	//Fetching the databases from MySQL(by executing Show databases)

	if(isrefreshtrue == wyTrue)
		EnumChildWindows(pGlobals->m_hwndclient, DBListBuilder::GetDBFromServers, (LPARAM)hwndcombo);
	else
		//Fetching the databases from object browser
		EnumChildWindows(pGlobals->m_hwndclient, DBListBuilder::GetDBFromActiveWins, (LPARAM)hwndcombo);
}

void
DBListBuilder::GetSVs(HWND hwndcombo)
{
	 // now we call the enum function and fill the combo with the connections
	//11.52: No enumeration for databases since database combo will only list dbs for the selected connection
	//11.52: Enumerate Only to Obtain connection details
	EnumChildWindows(pGlobals->m_hwndclient, DBListBuilder::GetServers, (LPARAM)hwndcombo);
}

wyInt32  
DBListBuilder::GetServers(HWND hwnd, LPARAM lparam)
{
	HWND            hwndcombo;
	wyInt32         ret;
	MDIWindow		*pcquerywnd=NULL, *wnd=NULL;
	wyWChar         classname[SIZE_512] = {0};
	wyString        dbname, findvalue;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;

	
	
	wyString            query, database;

    hwndcombo =(HWND)lparam;
	
	VERIFY(GetClassName(hwnd, classname, SIZE_512 - 1));

	VERIFY(wnd = GetActiveWin());

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
    {
		VERIFY(pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
			
		/* starting from 4.1 BETA 4 we use connection name for unique values */
		
		/*11.52:Connections need to be filtered.*/
		findvalue.Sprintf("%s", pcquerywnd->m_title.GetString());
		// find it. if not found add it.
		ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)findvalue.GetAsWideChar());

		if(ret == CB_ERR)
        {
			ret = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)findvalue.GetAsWideChar());

			//11.52: diffcombo item for connections will not contain database name
			pdiffcombo = new DIFFCOMBOITEM;
				
			//wcscpy(pdiffcombo->szDB, database.GetAsWideChar());
			wcsncpy(pdiffcombo->szDB, L"", SIZE_128 - 1);
			pdiffcombo->szDB[SIZE_128 - 1] = '\0';
                
			pdiffcombo->wnd = pcquerywnd;
				
			pdiffcombo->mysql = &pcquerywnd->m_mysql;
			pdiffcombo->tunnel = pcquerywnd->m_tunnel;
			pdiffcombo->info = &pcquerywnd->m_conninfo;

			SendMessage(hwndcombo, CB_SETITEMDATA, ret, (LPARAM)pdiffcombo);
		}
		


	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;

cleanup:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;	
}
wyInt32  
DBListBuilder::GetDBFromActiveWins(HWND hwnd, LPARAM lparam)
{
	HWND            hwndcombo,hwndtree;
	wyInt32         ret;
	MDIWindow		*pcquerywnd=NULL, *wnd=NULL;
	wyWChar         classname[SIZE_512] = {0};
	wyWChar			database[SIZE_512]  = {0};
    wyString        findvalue, dbname;
	TVITEM			tvi;
	HTREEITEM		hitem;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;

	hwndcombo =(HWND)lparam;

	VERIFY(GetClassName(hwnd, classname, SIZE_512 - 1));

	VERIFY(wnd = GetActiveWin());

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
    {
		VERIFY(pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));

		hwndtree = pcquerywnd->m_pcqueryobject->m_hwnd;

		hitem = TreeView_GetChild(hwndtree, TreeView_GetRoot(hwndtree));

		while(hitem)	
        {
			memset(&tvi, 0, sizeof(tvi));

			tvi.mask		= TVIF_TEXT;
			tvi.pszText		= database;
			tvi.cchTextMax	= SIZE_512 - 1;
			tvi.hItem		= hitem;

			VERIFY(TreeView_GetItem(hwndtree, &tvi));
			
			dbname.SetAs(database);
			/* starting from 4.1 BETA 4 we use connection name for unique values */
			findvalue.Sprintf("%s - %s", pcquerywnd->m_title.GetString(), dbname.GetString());

			// find it. if not found add it.
			ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)findvalue.GetAsWideChar());

			if(ret == CB_ERR)
            {
				ret = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)findvalue.GetAsWideChar());

			pdiffcombo = new DIFFCOMBOITEM;
				
			//wcscpy(pdiffcombo->szDB, database);
			wcsncpy(pdiffcombo->szDB, database, SIZE_128 - 1);
			pdiffcombo->szDB[SIZE_128 - 1] = '\0';
                
			pdiffcombo->wnd = pcquerywnd;
				
			pdiffcombo->mysql = &pcquerywnd->m_mysql;
			pdiffcombo->tunnel = pcquerywnd->m_tunnel;
			pdiffcombo->info = &pcquerywnd->m_conninfo;

				SendMessage(hwndcombo, CB_SETITEMDATA, ret, (LPARAM)pdiffcombo);
			}

			hitem = TreeView_GetNextSibling(hwndtree, hitem);
		}
	}

	return wyTrue;
}
//Getting databases from MySQL by executing show databases query
wyInt32  
DBListBuilder::GetDBFromServers(HWND hwnd, LPARAM lparam)
{
	HWND            hwndcombo;
	wyInt32         ret;
	MDIWindow		*pcquerywnd=NULL, *wnd=NULL;
	wyWChar         classname[SIZE_512] = {0};
	wyString        dbname, findvalue;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;

	MYSQL_RES			*myres=NULL;
	MYSQL_ROW			myrow;
	wyString            query, database;

    hwndcombo =(HWND)lparam;
	
	VERIFY(GetClassName(hwnd, classname, SIZE_512 - 1));

	VERIFY(wnd = GetActiveWin());

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
    {
		VERIFY(pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
			
		// execute query to get all the database names.
		query.Sprintf("show databases");
        myres = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);

		if(!myres)
		{
			ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
			goto cleanup;
		}
		
		while(myrow = pcquerywnd->m_tunnel->mysql_fetch_row(myres))	
		{
			database.SetAs(myrow[0]);
			dbname.SetAs(database);
			/* starting from 4.1 BETA 4 we use connection name for unique values */
			findvalue.Sprintf("%s - %s", pcquerywnd->m_title.GetString(), dbname.GetString());

			// find it. if not found add it.
			ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)findvalue.GetAsWideChar());

			if(ret == CB_ERR)
            {
				ret = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)findvalue.GetAsWideChar());
				
			pdiffcombo = new DIFFCOMBOITEM;
				
			//wcscpy(pdiffcombo->szDB, database.GetAsWideChar());
			wcsncpy(pdiffcombo->szDB, database.GetAsWideChar(), SIZE_128 - 1);
			pdiffcombo->szDB[SIZE_128 - 1] = '\0';
                
			pdiffcombo->wnd = pcquerywnd;
				
			pdiffcombo->mysql = &pcquerywnd->m_mysql;
			pdiffcombo->tunnel = pcquerywnd->m_tunnel;
			pdiffcombo->info = &pcquerywnd->m_conninfo;

				SendMessage(hwndcombo, CB_SETITEMDATA, ret, (LPARAM)pdiffcombo);
			}
		}

		pcquerywnd->m_tunnel->mysql_free_result(myres);
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;

cleanup:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;	
}

wyInt32  
DBListBuilder::GetDBFromActiveWinscopydb(HWND hwnd, LPARAM lparam)
{
	HWND            hwndcombo,hwndtree;
	wyInt32         ret;
	MDIWindow		*pcquerywnd=NULL, *wnd=NULL;
	wyWChar         classname[SIZE_512] = {0};
	wyWChar			database[SIZE_512]  = {0};
    wyString        findvalue, dbname;
	TVITEM			tvi;
	HTREEITEM		hitem;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;

	hwndcombo =(HWND)lparam;

	VERIFY(GetClassName(hwnd, classname, SIZE_512 - 1));

	VERIFY(wnd = GetActiveWin());

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
    {
		VERIFY(pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));

		hwndtree = pcquerywnd->m_pcqueryobject->m_hwnd;

		hitem = TreeView_GetChild(hwndtree, TreeView_GetRoot(hwndtree));

		while(hitem)	
        {
			memset(&tvi, 0, sizeof(tvi));

			tvi.mask		= TVIF_TEXT;
			tvi.pszText		= database;
			tvi.cchTextMax	= SIZE_512 - 1;
			tvi.hItem		= hitem;

			VERIFY(TreeView_GetItem(hwndtree, &tvi));
			
			dbname.SetAs(database);
			/* starting from 4.1 BETA 4 we use connection name for unique values */
			findvalue.Sprintf("%s", dbname.GetString());

			ret = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)findvalue.GetAsWideChar());

			//pdiffcombo = new DIFFCOMBOITEM;
			//	
			////wcscpy(pdiffcombo->szDB, database);
			//wcsncpy(pdiffcombo->szDB, database, SIZE_128 - 1);
			//pdiffcombo->szDB[SIZE_128 - 1] = '\0';
			//pdiffcombo->wnd = pcquerywnd;
			//pdiffcombo->mysql = &pcquerywnd->m_mysql;
			//pdiffcombo->tunnel = pcquerywnd->m_tunnel;
			//pdiffcombo->info = &pcquerywnd->m_conninfo;

			//SendMessage(hwndcombo, CB_SETITEMDATA, ret, (LPARAM)pdiffcombo);


			hitem = TreeView_GetNextSibling(hwndtree, hitem);
		}
	}

	return wyTrue;
}
//Getting databases from MySQL by executing show databases query
wyInt32  
DBListBuilder::GetDBFromServerscopydb(HWND hwnd, LPARAM lparam)
{
	HWND            hwndcombo;
	wyInt32         ret;
	MDIWindow		*pcquerywnd=NULL, *wnd=NULL;
	wyWChar         classname[SIZE_512] = {0};
	wyString        dbname, findvalue;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;

	MYSQL_RES			*myres=NULL;
	MYSQL_ROW			myrow;
	wyString            query, database;

    hwndcombo =(HWND)lparam;
	
	VERIFY(GetClassName(hwnd, classname, SIZE_512 - 1));

	VERIFY(wnd = GetActiveWin());

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
    {
		VERIFY(pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
			
		// execute query to get all the database names.
		query.Sprintf("show databases");
        myres = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);

		if(!myres)
		{
			ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
			goto cleanup;
		}
		
		while(myrow = pcquerywnd->m_tunnel->mysql_fetch_row(myres))	
		{
			database.SetAs(myrow[0]);
			dbname.SetAs(database);
			/* starting from 4.1 BETA 4 we use connection name for unique values */
			findvalue.Sprintf("%s", dbname.GetString());


			ret = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)findvalue.GetAsWideChar());
				
			//pdiffcombo = new DIFFCOMBOITEM;
				
			//wcscpy(pdiffcombo->szDB, database.GetAsWideChar());
			//wcsncpy(pdiffcombo->szDB, database.GetAsWideChar(), SIZE_128 - 1);
			//pdiffcombo->szDB[SIZE_128 - 1] = '\0';
                
			//pdiffcombo->wnd = pcquerywnd;
				
			//pdiffcombo->mysql = &pcquerywnd->m_mysql;
			//pdiffcombo->tunnel = pcquerywnd->m_tunnel;
			//pdiffcombo->info = &pcquerywnd->m_conninfo;

			//SendMessage(hwndcombo, CB_SETITEMDATA, ret, (LPARAM)pdiffcombo);
			
		}

		pcquerywnd->m_tunnel->mysql_free_result(myres);
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;

cleanup:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;	
}