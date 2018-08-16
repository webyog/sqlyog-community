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

#ifndef __TABMODULE_H__
#define __TABMODULE_H__

#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "TabTypes.h"
#include "TabHistory.h"
#include "TabObject.h"
#include "TabTableData.h"
#include "FindAndReplace.h"

class EditorProcs;
class TabEditor;
class EditorBase;
class TabTypes;
class TabQueryBuilder;
class TabSchemaDesigner;
class TableTabInterface;
class TabObject;
class TabHistory;

#ifdef COMMUNITY	
class CommunityRibbon;
#endif

class TabModule
{

public:
	
	/// Constructor with parent window handle.
    /**
    @param hwnd			: IN parent window handle
    */	
	TabModule(HWND hwnd);


	///Destructor 
	/**
    Free up all the allocated resources
    */
	~TabModule();


	/// Wrapper to create "TabControl" and initializes with a default normal "TabEditor"
    /**
	@param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool				Create(MDIWindow * wnd);


	/// Tab header for the community
	/**
	@return wyTrue 
	*/
	wyBool				CreateCommunityTabHeader();
    
	/// Function to create the TabControl
	/*
	@ returns VOID
	*/
	VOID				CreateTabControl();

	/// TabControl window procedure
	/**
	@param hwnd     : Window HANDLE
	@param message  : Window message
	@param wparam   : Unsigned int message parameter
	@param wparam   : Long message parameter
    @param pishandled : whether the message is handled.
	@returns long pointer
	*/
	static	LRESULT		CALLBACK TabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, wyBool* pishandled);


	/// Wrapper to create Editor tab and initializes with a default query editor & query tab
	/*  
	@return wyBool wytrue if it is SUCCESS, otherwise failure.
	@param wnd			: IN parent connection window pointer.
	*/
	wyBool				CreateQueryEditorTab(MDIWindow* wnd, wyInt32 pos = -1, wyBool setfocus = wyTrue);


	/// Creates the normal query editor
    /**
    @param wnd			: IN parent connection window pointer.
    @param hwnd			: IN handle to the parent editor tab.
    @returns EditorQuery pointer, NULL if it fails
    */
	EditorQuery *		CreateQueryEdit(MDIWindow *wnd);

	
	/// Wrapper to creates an Advanced Editor
    /**
    Creates an AdvEdit control on tab, it has got some difference with normal editor,
    because it executes all the queries in a single shot.
    @param wnd			: IN parent connection window pointer.
    @param title		: IN title to the new tab.
    @param image		: IN Image Id of the image to be shown with the tab caption.
    @param hitem		: IN The HTREEITEM pointer, 
    We are allowing to create an Advanced editor on the context of a treeview node only. 
    It is useful so that we can refresh that node once the content in the editor is executed successfully.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool				CreateAdvEditorTab(MDIWindow *wnd, wyChar *title, wyInt32 image, HTREEITEM hitem, wyString *strhitemname=NULL);

	/// Creates the normal Tabeditor
    /**
    @param wnd			: IN parent connection window pointer.
    @returns TabEditor pointer, NULL if it fails
    */
    TabEditor *			CreateTabEditor(MDIWindow * wnd);


	///Wrapper to create Tab for QueryBuilder
	/**  
	@param wnd			: IN parent connection window pointer.
	@return wyBool wytrue if it is SUCCESS, otherwise failure.
	@param wnd			: IN parent connection window pointer.
	*/
	wyBool				CreateQueryBuilderTab(MDIWindow * wnd);

	///Function to instantiate the TabQueryBuilder
	/*
	@param wnd			: IN parent connection window pointer.
    @returns TabQueryBuilder pointer
	*/
#ifndef COMMUNITY
	TabQueryBuilder		*CreateTabQB(MDIWindow * wnd);
#endif

    ///Function to create TabInterface for table operations.
    /*
    @param wnd          : IN parent connection window pointer
    @returns wyTrue if it is SUCCESS, else wyFalse.
    */
    wyBool              CreateTableTabInterface(MDIWindow *wnd, wyBool isaltertable=wyFalse, wyInt32 setfocustotab=-1);

    wyBool              IsValidFocusInOB(wyInt32    subtabindex);


    wyBool              IsAlterTableTabOpen(wyString& tblname, wyInt32& tabindex);

	///Wrapper to create Tab for SchemaDesigner
	/**  
	@param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
	*/
	wyBool				CreateSchemaDesigner(MDIWindow * wnd);	

	/// Crteating the Search tab
	/**
	@param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
	*/
	wyBool				CreateDatabaseSearchTab(MDIWindow * wnd, wyBool isdefault = wyFalse);

	wyBool				CreateHistoryTab(MDIWindow * wnd, wyBool showtab, wyBool setfocus = wyTrue);

	wyBool				CreateInfoTab(MDIWindow * wnd, wyBool setfocus = wyFalse);

	wyBool				CreateTabDataTab(MDIWindow * wnd, wyBool isnewtab = wyFalse, wyBool setfocus = wyFalse);

	///Function to instantiate the TabSchemaDesigner
	/**
	@param wnd			: IN parent connection window pointer.
    @returns TabSchemaDesigner pointer
	*/
	//TabSchemaDesigner	*CreateTabSchemaDesigner(MDIWindow * wnd);

	/// Handles resize of editor tab
    /**
    @returns void
    */
	void				Resize(wyBool issetredraw = wyFalse);

	/// Functiion to get the current 'TabEditor'
	/*
	*
	@ returns pointer to Active tab
	*/
	TabTypes*			GetActiveTabType();
	
    /// Gets Current Active Tabeditor
    /*
    @ returns TabTypes pointer
    */
    TabEditor*		    GetActiveTabEditor();

    /// Gets Current Active Tabeditor
    /*
    @ returns TabTypes pointer
    */
    TabEditor*		    GetTabEditorAt(wyInt32 index);

    TabTableData*		    GetActiveTabTableData();


	 /// Gets Current Active HistoryTab
    /*
    @ returns TabHistory pointer
    */
    TabHistory*			GetActiveHistoryTab();  

	TabObject*			GetActiveInfoTab();  

	/// Gets the currently selected editor pointer.
    /**
    @returns Image ID of active tab
    */
	wyInt32				GetActiveTabImage();

	LPSTR			GetActiveTabText();

    //Sets the parent connection window pointer
	/**
    @param wnd: IN the connection window pointer to set.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool				SetParentPtr(MDIWindow *wnd);

	/// Get the parent connection window pointer
    /**
    @returns parent connection window pointer.
    */
	MDIWindow *			GetParentPtr();


	/// Gets the present window handle
    /**
    @returns current window handle
    */
	HWND				GetHwnd();

	///To get the Parent Window Handle
	/*
	@returns parent connection window handle
	*
	*/
	HWND				GetparentHwnd();

	/// Closes the Tab
    /**
    @param index: IN tabindex to close
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool				CloseTab(wyInt32 index);

	/// Sets the tab font.
    /**
    It will take the font details from the .ini file and sets the font.
    @returns void
    */
	void				SetTabFont();

	/// Sets the tab font color.
    /**
    It will take the font color details from the .ini file and sets the font color.
    @returns void
	*/
	void				SetTabFontColor();

	/// Sets the TabHistory font color
    /**
    It takes the font color from the .ini
    @returns void
    */
	void				SetHistoryColor();

	/// Sets the font 
    /**
    It will not take from the .ini, if we are taking from .ini 
        he positioning also need to be calculated
    @returns void
    */
	void				SetHistoryFont();

    ///Sets the BackQuotes option 
    /*
    @returns void
    */
    void                SetBackQuotesOption();

	// Will regenerate and refresh all names, etc. which can be affected by preferences and sql previews if needed to 
	// apply any change in preferences etc
	void				Refresh();

	/// Handles the checking or unchecking the Menu item 'text or grid'
    /**
    @param pcquerywnd       : IN Query window pointer
    @param ischecked        : IN Checked or not
    @param menuid           : IN Menu id
    @returns wyTrue on success
    */
    wyBool				HandleCheckMenu(MDIWindow *pcquerywnd, wyBool ischecked, wyUInt32 menuid);

	 /// Sets the tab name if tab loaded from file
	/**
	@param filename : IN file name.
	@param isshowext : IN wyTrue for show the file-extension(FOr SD, QB files extension is not showing)
	@param isedited : IN def. parameter tells wheter the file is edited or not
	@return void
	*/
	 VOID		SetTabName(wyWChar *filename, wyBool isshowext, wyBool isedited = wyFalse);
	 VOID		SetTabRename(wyWChar *name, wyBool isedited = wyFalse);
	/// Shows/Hides all the content of the editor
    /**
    @param tabindex: IN currently selected tab index.
    @param status: IN wyTrue/wyfalse
    @returns void
    */
	VOID				ShowTabContent(wyInt32 tabindex, wyBool status);

	/// Current tab (TabControl)  Window Handle
	HWND				m_hwnd;

	/// Parent window handle
    HWND				m_hwndparent;

	/// Parent connection window(mdi) pointer
	MDIWindow *			m_parentptr;
	
	///  Editor class pointer
	TabEditor *			m_pctabeditor;

	/// TabQueryBuilder class pointer
	TabQueryBuilder		*m_pctabqb;

	/// TabSchemaDesigner class pointer
	TabSchemaDesigner		*m_pctabsd;

    /// TableTabInterface class pointer
    TableTabInterface   *m_tabinterface;

    /// TabHistory class pointer
	TabHistory			*m_pctabhistory;

	TabObject			*m_pcinfotab;

	/// EditorQuery object pointer
	EditorQuery *		m_pceditorquery;
	
	/// EditorProc object pointer
	EditorProcs *		m_pcadvedit;

	///Window Proc
	WNDPROC				m_wporigproc;
	
	/// buffer to store history data
	wyChar				*m_historydata;

    ///Flag specifying that the tabs are creating by default or not
    wyBool              m_isdefault;

#ifdef COMMUNITY		
	CommunityRibbon		*m_cribbon;
#endif

	///Community title 
	HWND				m_hwndcommtytitle;

	//Flag tells whether tab creating or not, used to avoid flickering while create tab, this used in Resize() function
	wyBool				m_istabcreate;

	//Result sets for 'SHOW STATUS' for Info tab on select 'server' on object browser
	MYSQL_RES			*m_infostatusres;

	//Result sets for 'SHOW VARIABLES' for Info tab on select 'server' on object browser
	MYSQL_RES			*m_infovariablesres;

	TableView			*m_tableview;
};
#endif
