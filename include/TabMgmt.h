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


#ifndef _QUERYTAB_H_
#define _QUERYTAB_H_

#include "Global.h"
#include "TabMessage.h"
#include "FrameWindowHelper.h"
#include "CustTab.h"
#include "ResultView.h"
#include "TabTable.h"
#include "TabQueryHistory.h"
#include "TabQueryObject.h"

class CQueryInfo;
class QueryAnalyzerBase;

class TabMgmt
{
public:

    /// Constructor with parent window as argument.
    /**
    @param hwndparent		: IN parent window handle
    */
	TabMgmt(HWND hwndparent, MDIWindow* pmdi);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~TabMgmt();

    /// Creates the tab window control
    /**
    @param wnd				: IN the parent query window pointer
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool      Create();

	/// Gets the tab window handle
    /**
    @returns HWND, handle to tha tab window control
    */
	HWND		GetHwnd();

    /// Resizes the tab control
    /**
    @returns void
    */
	void		Resize();
	
    /// Handles on tab selection change
    /**
    @returns void
    */
	void		OnTabSelChange(LPNMCTC lpnmctc = NULL);

	MySQLDataEx*       GetResultData(wyInt32 index = -1);

    DataView*   GetDataView(wyInt32 index = -1);

    /// Changes the tab caption to include tab order number
    /**
    @returns void
    */
	void		ChangeTitles();

    /// Hides the result window
    /**
    @returns void
    */
	void		HideResultWindow();

    /// Returns the current selected image index
    /**
    @returns wyInt32, index of the image
    */
	wyInt32     GetSelectedItem();

    
    /// Selects a particular tab
    /**
    @param index		: IN tab index to select
    @returns void
    */
	void		SelectTab(wyInt32 index);

    /// Deletes all tabs
    /**
    @returns void
    */
	void		DeleteAllItem(wyBool isdeletefixedtabs);

    /// Inserts a result tab
    /**
    @param tabindex		: IN tab index
    @param pdata		: IN result data
    @returns void
    */
    void		AddRecordTab(wyInt32 tabindex, MySQLResultDataEx* pdata);

    /// Inserts a message tab
    /**
    @param tabindex		: IN tab index
    @returns void
    */
	void		AddMessageTab();


    void        AddTableDataTab(MySQLTableDataEx* data = NULL);

    void        AddHistoryTab();

    void        AddInfoTab();
    
	//Add Query Analyzer tab 
	/**
	@param wnd : IN MDIWindow handle
	@return void
	*/
	void		AddAnalyzerTab(wyInt32 index, MDIWindow *wnd = NULL);

	///Creates the Query Analyzer tab
	/**
	@param query			: IN Query executed
	@param isselectquery	: IN sets wyTrue if its SELECT query 
	@param istunnel			: IN sets wyTru if its HTTP Tunnel
    @param index			: IN Index of the Analyzer tab
	@param wnd				: In con. window pointer
	@return 1 if Profiler tab is added elese return 0
	*/
    wyInt32     AddQueryAnalyzer(MySQLResultDataEx* pdata, wyBool isselectquery, wyBool istunnel, wyInt32 index);

    wyInt32     AddExplainQueryAnalyzer(MySQLResultDataEx* pdata, wyBool isselectquery, wyBool istunnel, wyInt32 index, wyBool isExtended);

	/// Event Handler when objects info tab is selected
    /**
    @param wnd			: IN parent connection window pointer
    @returns void
    */
	void		OnSelectObjectInfo(MDIWindow * wnd);

	/// Sets the result data as LPARAM on the tab control
    /**
    @param ptr			: IN TabQuryTypes* pointer to set
    @returns wyBool , wyTrue if SUCCESS, otherwise wyFalse
    */
    wyBool      SetResultData(TabQueryTypes * ptr);

    void        SetTabEditorPtr(TabTypes* ptr);

    TabQueryTypes*   GetActiveTabType();

    wyInt32         GetActiveTabIcon();

    wyInt32         SelectFixedTab(wyInt32 image, wyBool isonlyprob = wyFalse);

    void            DeleteTab(wyInt32 index,wyBool ispostion=wyFalse);

    void            AddFixedTab(wyInt32 image);


    /// Tab window handle
    HWND		m_hwnd;

	/// Parent window HANDLE
	HWND		m_hwndparent;

    /// pointer to result viewer
    ResultView* m_presultview;

    TabTable*   m_ptabletab;

    TabQueryHistory* m_phistory;

    TabQueryObject* m_pqueryobj;

	/// pointer to message viewer
	TabMessage   *m_pcquerymessageedit;

	// Pointer to parent 'TabEditor' pointer
	TabTypes*		m_tabeditorptr;
    	
	//Analyzer object
	QueryAnalyzerBase	*m_pqa;
		

private:

    /// window procedure for the tab control
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool *pishandled);

    /// Handle CTCM_COMMAND
    /**
    @param hwnd			: IN tab window handle
    @param wparam		: IN windowprocedure WPARAM
    @param lparam		: IN windowprocedure LPARAM
    @returns wyInt32 , always 0.
    */
    wyInt32     OnCtcmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);


    /// Sets the font for Tab management
    /**
    @returns void
    */
	void        SetFont();

    /// Creates the query tab 
    /**
    @param hwndparent	: IN parent window HANDLE
    @returns HWND, HANDLE to the new tab control if successful, otherwise NULL
    */
	HWND        CreateQueryTab(HWND hwndparent);

    /// Creates tab image list
    /**
    @returns void
    */
	void        CreateTabImageList();

    /// Creates the result viewer
    /**
    @returns void
    */
	void        CreateResultView();

    /// Creates the message viewer
    /**
    @returns void
    */
	void        CreateQueryMessageEdit();

    ///// Sets the result data as LPARAM on the tab control
    ///**
    //@param ptr			: IN TabQuryTypes* pointer to set
    //@returns wyBool , wyTrue if SUCCESS, otherwise wyFalse
    //*/
    //wyBool      SetResultData(TabQueryTypes * ptr);


	/// Initializes the data required for the activation and working of context menu
	/**
	@param ptrp		: IN Structure containing table information 
	*/
	void		InitDataForContextMenu(TabQueryTypes *	ptrp);


	/// Initializes the data required for the activation and working of context menu from table tab
	/**
	@returns TabQuryTypes* structure
	*/
	TabQueryTypes *			ExportFromTableTab();

	/// HANDLE to the font
	HFONT		m_hfont;

    /// Used when list view changes to/from text view
	HCURSOR		m_hcursor;

    /// Image list for the tab control
	HIMAGELIST	m_himl;

    /// Processes Windows messages.
	WNDPROC     m_wporigproc;

	//'TabQueryTypes' pointer
	TabQueryTypes *		m_pctabquerytypes;

	//MDIWindow Object pointer
	MDIWindow			*m_pcimdiwindow;
};

#endif
