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


#ifndef _QUERYEDIT_H_
#define _QUERYEDIT_H_

#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "CustTab.h"
#include "EditorBase.h"


class	EditorBase;

class EditorQuery : public EditorBase
{
public:
	EditorQuery(HWND hwnd);
	~EditorQuery();

      /// Creates the window
    /**
    @param wnd      : IN MDIWindow class pointer
    @returns wyTrue if success
    */
	wyBool				Create(MDIWindow * wnd) ;


    ///  Function to create the richedit window.
    /**
    @param wnd      : IN MDIWindow class pointer
    @param hwnd     : IN Window Handler
    @returns window HANDLE
    */
	HWND				CreateQueryEdit(MDIWindow * wnd, HWND hwnd);
    
    /// Function executes the current query.
    /**
    @param stop         : IN Stop condition
    @param isexecute    : IN Whether the table mode is changed from read-only to edit
	@returns wyTrue on success
    */
	wyBool				ExecuteCurrentQuery(wyInt32 *stop, wyBool isexecute = wyFalse);

    wyBool              ExecuteExplainQuery(wyInt32 *stop, wyBool isExtended);

    //Executes all queries.
    /**
    @param stop         : IN Stop condition
	@returns wyTrue on success
    */
	wyBool				ExecuteAllQuery(wyInt32 *stop);

    // Executes selected queries.
    /**
    @param stop         : IN Stop condition
	@returns wyTrue on success
    */
	wyBool				ExecuteSelQuery(wyInt32 *stop);

    /// Function to handle after query execution finish
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	wyBool		HandleQueryExecFinish(wyInt32 * stop, WPARAM wparam);
};

#endif
