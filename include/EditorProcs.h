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


#ifndef _ADVEDIT_H_
#define _ADVEDIT_H_

#include "FrameWindowHelper.h"
#include "Global.h"
#include "scintilla.h"
#include "MDIWindow.h"
#include "CustTab.h"
#include "EditorBase.h"


//class MDIWindow;
class	EditorBase;

#define IDC_DELIMITOR			WM_USER+1
#define IDC_DELIMITORSTATIC		WM_USER+1

//HWND	StartExecute ( MDIWindow * wnd, EXECUTEOPTION opt );
//void	EndExecute ( MDIWindow * wnd, HWND hwnd, EXECUTEOPTION opt );

class EditorProcs : public EditorBase
{
public:
	EditorProcs(HWND hwnd);
	~EditorProcs();
	
    ///Comman virtual function to create Query edit and Advanced edit
	/**
	@param wnd : IN pointer to MDIWindow Object.
	@param htreeitem : IN handle to tree item	
	@returns wyTrue on success else returns wyFalse.
	*/
    wyBool				Create(MDIWindow *wnd, HTREEITEM htreeitem, wyString *strhitemname = NULL);

	///function to create the rich edit window.
	/**
	@param wnd : IN pointer to MDIWindow Object.
	@param htreeitem : IN handle to tree item	
	@returns (HWND)handle to AdvEdit.
	*/
	HWND				CreateAdvEdit(MDIWindow *wnd, HWND hwnd, HTREEITEM hitem, wyString *strhitemname = NULL);

	
	///Function executes the current query.
	///i.e the query in which the cursor is at the moment. 
    /**	
	@param stop : IN Stop condition 
	@param isedit : IN 
	@returns wyTrue on success else returns wyFalse
	*/ 
	wyBool				ExecuteCurrentQuery(wyInt32 * stop, wyBool isedit = wyFalse);
	
    ///This function executes all the query in the query window.
    /**
    It strtoks through the text with taking ;
	as a separator and executes each sql statement as it gets one.
	@param stop : IN Stop condition
    @returns wyTrue on success else returns wyFalse
	*/
	wyBool				ExecuteAllQuery(wyInt32 * stop);
	
	///Function executes query for updation.
	/**
	Gets the current query and calls the insertupdate.
    @param stop : IN Stop condition
    @returns wyTrue on success else returns wyFalse
	*/
	wyBool				ExecuteSelQuery(wyInt32 * stop);

    /// Function to handle after query execution finish
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	wyBool		HandleQueryExecFinish(wyInt32 * stop, WPARAM wparam);

};

#endif
