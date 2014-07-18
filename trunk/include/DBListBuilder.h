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

#ifndef __CGETDBFROMWINDOWS__
#define __CGETDBFROMWINDOWS__

#include <windows.h>
#include <windowsx.h>

#include "Datatype.h"

class DBListBuilder
{
public:

	static wyInt32 CALLBACK	GetDBFromActiveWins(HWND hwnd, LPARAM lParam);

	//getting databases from Mysql
	static wyInt32 CALLBACK GetDBFromServers(HWND hwnd, LPARAM lparam);

	static wyInt32 CALLBACK GetServers(HWND hwnd, LPARAM lparam);
	static wyInt32 CALLBACK	GetDBFromActiveWinscopydb(HWND hwnd, LPARAM lParam);

	//getting databases from Mysql
	static wyInt32 CALLBACK GetDBFromServerscopydb(HWND hwnd, LPARAM lparam);
	DBListBuilder();
	~DBListBuilder();

	///Filling the combo box
	/**
	@param hwndcombo	: IN Combo box handle
	@param isrefresh    : IN wyTrue if we want fetch the database from the Server,else wyFalse (it will fetch from Object browser)
	@return void
	*/
	void GetDBs(HWND hwndcombo, wyBool isrefresh = wyFalse);
	void GetSVs(HWND hwndcombo);
};


#endif