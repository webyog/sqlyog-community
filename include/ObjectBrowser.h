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


#ifndef _QUERYOBJECT_H_
#define _QUERYOBJECT_H_

#include "FrameWindowHelper.h"
#include "DialogPersistence.h"
#include "CustGrid.h"  
#include <string>

#define OBFILTERTIMER   555

/*! \struct drag_state
    \brief Used to store information when there is a mouse drag
    \param wyBool       m_dragging        Dragged or not
	\param HIMAGELIST   m_dragimage       Image list
    \param HTREEITEM    m_item            Tree items
*/
typedef struct
{
	wyBool      m_dragging;
	HIMAGELIST	m_dragimage;
	HTREEITEM	m_item;

}	drag_state;


class CQueryObject
{

public:

	CQueryObject(HWND hwnd, wyChar  *filterdb);

    ~CQueryObject();

    /// Function to create the object browser.
    /**
    @param hwnd     : Window Handler.
    */
	HWND		CreateObjectBrowser(HWND hwnd);

    /// Callback function for the object browser.
    /**
    @param hwnd     : Window Handler.
    @param message  : Messages.
    @param wparam   : Unsigned message parameter.
    @param lparam   : LPARAM parameter.
    */
	static		LRESULT CALLBACK WndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

    
    /// Gets the window HANDLE.
    /**
    */
	HWND		GetHwnd();

    /// Resize the object browser. 
    /**
    */
	void		Resize(wyBool isannouncements = wyFalse, wyBool isstart = wyFalse);

    /// Function to create the object browser.
    /**
    */
	wyBool      Create();

    /// Refreshes the object browser.
    /**
    @param tunnel      : Tunnel pointer.
    @param mysql       : Pointer to mysql pointer.
    */
	void		RefreshObjectBrowser(Tunnel * tunnel, PMYSQL mysql, MDIWindow* wnd);

	void		CollapseObjectBrowser(HWND hTree);

	void		CollapseNode(HWND hTree, HTREEITEM hti);
   
    /// Gets the old database name.
    /**
    @param olddb          : databases name.
    */
	LRESULT		GetOldDatabase(wyString &olddb);

    /// Sets the context to old database.
    /**
    @param tunnel      : Tunnel pointer.
    @param mysql       : Pointer to mysql pointer.
    @param db          : databases name.
    */
	LRESULT		SetOldDatabase(Tunnel *tunnel, PMYSQL mysql, wyString &db);

    /// Function creates imagelist for the various items in the treeview.
    /**
    @param pnt      : Point.
    */
	void		CreateImageList();

    /// Get in which item the mouse is selected.
    /**
    @param pnt      : Point.
    */
	HTREEITEM	SelectOnContext(LPPOINT pnt);

    /// Function to implement context menu of the edit box.
    /**
    @param lparam   : LPARAM parameter.
    */
	wyBool		OnContextMenu(LPARAM lparam);

    /// Implements on item expanding message.
    /**
    @param pnmtv     : Tree items containing notification message
    */
	wyBool		OnItemExpandingHelper(LPNMTREEVIEW pnmtv, MDIWindow *wnd);

    /// Change toolbars depending upon selection.
    /**
    @param hItem     : Tree items
    */
	wyBool		OnSelChanged(HTREEITEM hitem, LPNMTREEVIEW pnmtv = NULL);

	/// Function to add all the tables in the database.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    @param htable    : Tree items
    */  
	wyBool		InsertDatabases(Tunnel *tunnel, PMYSQL mysql, HTREEITEM hServer);
		
 	/// Handles the object browser when altered a table or created new table(s)
	/**
	@return VOID
	*/
	VOID		RefreshObjectBrowserOnCreateAlterTable();

    /// Drop database when the user selects the option from the context menu.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		DropDatabase(Tunnel *tunnel, PMYSQL mysql);

    /// Drop table when the user selects the option from the context menu.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		DropTable(Tunnel *tunnel, PMYSQL mysql);

    /// Drop a field when the user selects the option from the context menu.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		DropField(Tunnel *tunnel, PMYSQL mysql);

    /// Drop an index the user selects the option from the context menu.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		DropIndex(Tunnel *tunnel, PMYSQL mysql);

    /// Clear a table when the user selects the option from the context menu.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		EmptyTable(Tunnel *tunnel, PMYSQL mysql);

    /// Empty the database i.e. drop all the tables in the database.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		EmptyDatabase(HWND hwnd, Tunnel *tunnel, PMYSQL mysql);

    /// Truncate the database i.e. empty all the tables in the database.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		TruncateDatabase(HWND hwnd, Tunnel *tunnel, PMYSQL mysql);

    /// Calls the advance properties dialog box to show the properties of a column.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    */
	wyBool		AdvProperties(Tunnel *tunnel, PMYSQL mysql);
    
    /// Function initializes the Export Data dialog box with the parameter depending upon
    /// the selection.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Pointer to mysql pointer.
    @param auth      : Authentication
    @param nid       : Item id    
    */
	wyBool		ExportData(Tunnel *tunnel, PMYSQL mysql, TUNNELAUTH * auth, wyInt32 nid);

    /// Change the table type of a selected table to a different table type.
    /**
    @param tunnel       : Tunnel pointer.
    @param mysql        : Pointer to mysql pointer.
    @param newtabletype : New table type
    */
	wyBool		ChangeTableType(Tunnel *tunnel, PMYSQL mysql, const wyWChar *newtabletype);

    /// Identify whether the selected htreeitem of NFOLDER is index or column.
    /**
    @param hitem    : Tree item
    */
	wyBool		IsSelectedIndex(HTREEITEM hitem);

    /// Function to create the schema window.
    /**
    */
	wyBool		CreateSchema();

    /// This function creates the insert statement of the selected table.
    /**
    */
	wyBool		CreateInsertStmt();

	/// Creates the UPDATE statement of the selected table.
    /**
    */
	wyBool		CreateUpdateStmt();

	/// Creates the DELETE statement of the selected table.
    /**
    */
	wyBool		CreateDeleteStmt();


	/// Creates the insert statement of the selected table.
    /**
    */
	wyBool		CreateSelectStmt();

	
    /// Creates a new table.
    /**
    */
	wyBool		CreateTableMaker();

    /// Checks for various validation and puts the node into edit label mode.
    /**
    */
	wyBool		RenameObject();

    /// Initializes the as csv dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		ExportAsCSV(Tunnel *tunnel, PMYSQL mysql);

    /// Initializes the export as xml dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		ExportAsXML(Tunnel *tunnel, PMYSQL mysql);

    /// Initializes the export as html dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		ExportAsHTML(Tunnel *tunnel, PMYSQL mysql);

    /// Initializes the Backup Database dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		BackUpDB(Tunnel *tunnel, PMYSQL mysql);

    /// Initializes the Restore Database dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		RestoreDB(Tunnel *tunnel, PMYSQL mysql);

    /// Function initializes the copy database dialog.
    /**
    */
	wyBool		CopyDB();

    /// Function initializes the copy database for different host.
    /**
    */
	wyBool		CopyTableToDiffertHostDB();	
	
    /// Initializes the import from CSV dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		ImportFromCSV(Tunnel *tunnel, PMYSQL mysql);
	 /// Initializes the import from XML dialog box.
    /**
    @param pcquerywnd    : Query window pointer.
    @param mysql         : Pointer to mysql pointer.
    */
	wyBool		ImportFromXML(Tunnel *tunnel, PMYSQL mysql);

    /// This function copies given table into a new table using MySQL inbuilt select command.
    /**
    @param pcquerywnd    : Query window pointer.
    */
	wyBool		CopyTable(MDIWindow *pcquerywnd);

    /// This function is called when the user has finished changing the name of the table.
    /**
    @param ptvdi    : Tree view display inforamtion.
    */
	wyBool		EndRename(LPNMTVDISPINFO ptvdi);

    /// Selects the table and database name and stores it in the member.
    /**
    @param hitemtable    : Tree item.
	@return the databse node handle
    */
	HTREEITEM	GetTableDatabaseName(HTREEITEM hitemtable);

    /// Selects the database name and stores it in the member variable of the object browser.
    /**
    @param hitemtable    : Tree item.
    */
	wyBool		GetDatabaseName(HTREEITEM hitemtable);

    /// Brings the selected item in the object browser in sync..
    /**
    @param pcquerywnd    : Query window pointer.
    */
	wyBool		SyncObjectDB(MDIWindow *pcquerywnd);
	wyBool		SyncObjectDBNocombo(MDIWindow *pcquerywnd);

    /// Returns the index of the image of the selected node.
    /**
    */
	wyInt32		GetSelectionImage();

    HTREEITEM   GetSelectionOnFilter(HWND hwnd);

 //   /// Returns the item image
 //   /**
 //   @param hitem    : Tree item.
 //   */
	//wyInt32		GetItemImage(HTREEITEM hitem);

    /// The function is called when a user presses insert in ObjectBrowser.
    /**
    */
	wyBool		ProcessInsert();

    /// The function is called when a user presses delete in ObjectBrowser.
    /**
    @param uisshiftpressed : Check for shift press
    */
	wyBool		ProcessDelete(wyBool uisshiftpressed);

    /// The function is called when a user presses F2 in ObjectBrowser.
    /**
    */
	wyBool		ProcessF2();

	/// The function is called when a user presses F6 in ObjectBrowser.
	/**
	*/
	wyBool		ProcessF6();

	wyBool		ProcessF4();
    /// The function is called when a user enter in ObjectBrowser.
    /**
    @param uisshiftpressed : Check for shift press
    */
	wyBool		ProcessReturn(wyBool uisshiftpressed);

    /// Double click manager.
    /**
    */
	wyBool		ShowTable(wyBool isnewtab);

    wyBool      OpenTable();

    wyBool      IsSelectionOnTable();

	void		setIsTruncate(wyBool);

    /// Double click manager.
    /**
    @param wparam   : Unsigned message parameter
    @param lparam   : LPARAM parameter
    */
	LRESULT		OnDBLClick(WPARAM wparam, LPARAM lparam);
	
    /// Selects the database which is passed as parameter and then expands the tables too.
    /**
    */
	wyBool		ExpandDatabase();

	/// Handle gives the child of treeview item. 
	/**
	@hitemobject : IN parent item
	@itemname    : IN Name of the item to be searched and returnd
    @return handle if found, else return NULL
	*/
	HTREEITEM	GetTreeObjectItem(HTREEITEM hitemobject, wyWChar *itemname);

    /// Function to store the info of the selected item.
    /**
    */
	wyBool		GetItemInfo();

    /// Calculates and returns the number of character which can come in the width of
    /// the edit box.
    /**
    @param pcquerywnd    : Query window pointer.
    */
	wyInt32		GetMaxLimit(MDIWindow * pcquerywnd);

	/// whether primary key is exist or not
    /// the edit box.
    /**
    @param pcquerywnd    : Query window pointer.
	@param primary       : primary key or unique key
	@param isbacktick    : backtick is required or not
    */
	wyBool		GetPrimaryKeyInfo(MDIWindow * pcquerywnd, wyString &primary, wyBool isbaktick);

    /// Function to drop all tables if the mysql version is greater than/equal to 5.0.10 */
    /**
    @param hwnd    : Window HANDLE.
    @param tunnel  : Tunnel.
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */
	wyInt32		DropTables(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);

	///Wrapper to execute query.Its added when implemented Disable FK-CHECK with http 
	/**
	@param tunnel  : IN Tunnel pointer  mysql, wyString &query
	@param         : IN sets wyTrue for http else wyFalse
	@param mysql   : IN PMYSQL handle
	@param query   : IN Query to execute
	@param istunnel: IN sets wyTRue if use http tunnel else sets wyFalse
	@return wyTrue on success else return wyFalse
	*/
	wyBool		HandleExecuteQuery(Tunnel *tunnel, PMYSQL mysql, wyString &query, wyBool istunnel);

    /// Function to drop all views if the mysql version is greater than/equal to 5.0.10 */
    /**
    @param hwnd    : Window HANDLE.
    @param tunnel  : Tunnel.
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */
	wyInt32		DropViews(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);

    /// Function to drop all procedure if the mysql version is greater than/equal to 5.0.10 */
    /**
    @param hwnd    : Window HANDLE.
    @param tunnel  : Tunnel.
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */
	wyInt32		DropProcedures(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);

    /// Function to drop all functions if the mysql version is greater than/equal to 5.0.10 */
    /**
    @param hwnd    : Window handler.
    @param tunnel  : Tunnel. 
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */
	wyInt32		DropFunctions(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);
	/// Function to drop all events if the mysql version is greater than/equal to 5.1.1 */
    /**
    @param hwnd    : Window handler.
    @param tunnel  : Tunnel. 
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */
	wyInt32		DropEvents(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);

    /// Creates a view with the given name.
    /**
    @param hwnd        : Window HANDLE.
    @param tunnel      : Tunnel. 
    @param mysql       : Mysql pointer.
    @param createstmt  : Create statement.
    */
	wyBool		GetCreateView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &createstmt);
    
	/// Alters a view with the given name.
    /**
    @param hwnd        : Window HANDLE.
    @param tunnel      : Tunnel. 
    @param mysql       : Mysql pointer.
    @param alterstmt   : Alter statement.
    */
	wyBool		GetAlterView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &alterstmt);

    /// Creates a procedure with the given name.
    /**
    @param hwnd        : Window HANDLE.
    @param tunnel      : Tunnel. 
    @param mysql       : Mysql pointer.
    @param createstmt  : Create statement.
    */
	wyBool		GetCreateProcedure(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &createstmt);

    /// Creates a function  with the given name.
    /**
    @param hwnd        : Window HANDLE.
    @param tunnel      : Tunnel. 
    @param mysql       : Mysql pointer.
    @param createstmt  : Create statement.
    */
	wyBool		GetCreateFunction(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &createstmt);

    /// Creates a trigger with the given name.
    /**
    @param hwnd        : Window HANDLE.
    @param tunnel      : Tunnel. 
    @param mysql       : Mysql pointer.
    @param createstmt  : Create statement.
    */
	wyBool		GetCreateTrigger(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &createstmt);

    /// Deletes the selected view.
    /**
    @param hwnd      : Window HANDLE.
    @param tunnel    : Tunnel.
    @param mysql     : Mysql pointer.
    @param dropstmt  : Drop statement.
    */
	wyBool	    GetDropView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &dropstmt);

    /// Deletes the selected procedure.
    /**
    @param hwnd      : Window HANDLE.
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    @param dropstmt  : Drop statement.
    */
	wyBool		GetDropProcedure(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &dropstmt);

    /// Deletes the selected function.
    /**
    @param hwnd      : Window HANDLE.
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    @param dropstmt  : Drop statement.
    */
	wyBool		GetDropFunction(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &dropstmt);

    /// Deletes the selected trigger.
    /**
    @param hwnd      : Window HANDLE.
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    @param dropstmt  : Drop statement.
    */
	wyBool		GetDropTrigger(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &dropstmt);
	
    /// Deletes view.
    /**
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    */
	wyBool		DropView(Tunnel * tunnel, PMYSQL mysql);
	/// Deletes event.
    /**
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    */
	wyBool      DropEvent(Tunnel * tunnel, PMYSQL mysql);

    /// Deletes function.
    /**
    @param tunnel    : Tunnel. 
    @param mysql     : Mysql pointer.
    */
	wyBool		DropFunction(Tunnel * tunnel, PMYSQL mysql);

	wyBool      DropDatabaseObject(Tunnel * tunnel, PMYSQL mysql, wyChar *objecttype);

    /// Deletes procedure.
    /**
    @param tunnel    : Tunnel pointer.
    @param mysql     : Mysql pointer.
    */
	wyBool		DropProcedure(Tunnel * tunnel, PMYSQL mysql);

    /// Deletes trigger.
    /**
    @param tunnel    : Tunnel pointer. 
    @param mysql     : Mysql pointer.
    */
	wyBool		DropTrigger(Tunnel * tunnel, PMYSQL mysql);

    /// Helps to refreshes the parent node.
    /**
    @param hselitem    : Tree item.
    */
	wyBool		RefreshParentNodeHelper	();

    /// Refreshes the parent node.
    /**
    @param hselitem    : Tree item.
    */
	wyBool		RefreshParentNode(HTREEITEM hselitem);
 
    /// Handles the table naming process.
    /**
    @param ptvdi    : Tree view display info.
    */
	wyBool		EndRenameTable(LPNMTVDISPINFO ptvdi);

    /// Handles the view naming process.
    /**
    @param ptvdi    : Tree view display info.
    */
	wyBool		EndRenameView(LPNMTVDISPINFO ptvdi);
	/// Handles the event naming process.
    /**
    @param ptvdi    : Tree view display info.
    */
	wyBool      EndRenameEvent(LPNMTVDISPINFO ptvdi);

    /// Handles the procedure naming process.
    /**
    @param ptvdi    : Tree view display info.
    */
	wyBool		EndRenameProcedure(LPNMTVDISPINFO ptvdi);

    /// Handles the trigger naming process.
    /**
    @param ptvdi    : Tree view display info.
    */
	wyBool		EndRenameTrigger(LPNMTVDISPINFO ptvdi);	

    /// Gets database in the given node.
    /**
    */
	HTREEITEM	GetDatabaseNode();

    /// Gets table in the given node.
    /**
    */
	HTREEITEM	GetTableNode();

    /// Gets node in a given tree.
    /**
    @param hitem    : Tree items.
    @param buff     : Text.
    @param buffsize : Size of buffer.   
    */
	HTREEITEM	GetNode(HTREEITEM hdbitem, wyInt32 type);

    /// Insert text in the nodes.
    /**
    */
	wyBool		InsertNodeText();

    /// Selects the first database.
    /**
    */
	wyBool		SelectFirstDB();

    /// Inserts filter databases.
    /**
    @param hparent  : Parent tree.
    @param filterdb : Filter database names.
    */
    void        InsertFilterDatabases(HTREEITEM hserver, wyString &filterdb);

    /// Displays context menu .
    /**
    @param wnd   : Query window.
    @param image : Index of images.
    @param pnt   : Point.
    @param incr  : increment.
    @param str   : String.
    */
    void        ShowContextMenu(MDIWindow *wnd, wyInt32 image, POINT *pnt, wyInt32 incr, wyWChar *str);

	/// Loads the menu for 'Tables' folder
	/**
	@param pnt   : POINT struct holds the mouse position.
	@return void
	*/
	VOID		LoadTablesMenu(POINT *pnt);

	/// Loads the menu for Table
	/**
	@param pnt   : POINT struct holds the mouse position.
	@param	incr : menu position
	@return void
	*/
	VOID		LoadTableMenu(POINT *pnt, wyInt32 incr);

    ///// Gets text in the given node.
    ///**
    //@param hitem    : Tree items.
    //@param buff     : Text.
    //@param buffsize : Size of buffer.   
    //*/
    //wyBool      GetNodeText(HTREEITEM hitem, wyWChar * buff, wyInt32 buffsize);

    /// Gets table's name in database.
    /**
    @param item     : Index of images to be selected.
    @param hitem    : Tree items.
    */
    wyBool      HandlerToGetTableDatabaseName(wyInt32 item, HTREEITEM hitem);

    /// Gets database name.
    /**
    @param item     : Index of images to be selected.
    @param hitem    : Tree items.
    */
    void        HandlerToGetDatabaseName(wyInt32 item, HTREEITEM hitem);

    /// Keeps all the expanded noded in an HTREEITEM array so we can use this to restore tree state
    /**
    @returns void
    */
    void    GetTreeState();

    /// Helps to restore the tree state
    /**
    @returns void
    */
    void    RestoreTreeState();

    /// Gets all expanded child nodes 
    /**
    @param hitem:   IN treenode 
    @returns void
    */
    void    GetAllExpandedNodes(HTREEITEM hitem);

    /// Helps to expanded  child table nodes 
    /**
    @param hitemdb:     IN database treenode 
    @param nodename:    IN database node name
    @returns void
    */
    void    ExpandTable(HTREEITEM hitemdb, wyWChar *nodename);

    /// Helps to expanded  child column nodes 
    /**
    @param hitemdb:     IN database treenode 
    @param nodename:    IN database node name
    @returns void
    */
    void    ExpandColumn(HTREEITEM hitemtable, wyWChar *nodename);


	/// Function while begin an item to Drag
	/*
	@param lParam : mouse position
	@returns wyTrue if item is Table, else wyFalse	
	*/
	wyBool	OnBeginDrag(LPARAM lParam);


    /// Handle double click on table, for dropping canvas for QueryBuilder
    /*
    @returns wyTrue on success else wyFalse
    */
    wyBool		 ProcessTable(); 

    ///Handles while dropping the table
	/*
	@ return VOID
	*/
    void		OnDropTable();

   /// Handle to add back ticks to all necessary places in query if enabled in preference
    /*
    @param isbacktick : IN flag tells whether the back quotes option enabled or not in preference 
    @ returns bactick if enabled or blankstring
    */
    wyChar		*IsBackTick(wyBool isbacktick);

	///function to select database in OB when database is changed in combobox   
	/**
	@param dbname	: Database name
	@returns wyTrue if successful.
	*/
	wyBool      OnComboChanged(wyWChar * dbname);

	/// Implements on item expanded message.
    /**
    @param tv       : Treevies items.
    */
	void 		OnItemExpanded(LPTVITEM  tvi);
	
	//Make sure Trigger folder contains only existing triggers, drop trigger when drop table for that
	/**
	@param hitemdb : IN treeview handle to database node
	@reurn wyTrue on success else return wyFalse
	*/
	wyBool		ReorderTriggers(HTREEITEM hitemdb);

	/// Command handler for Object Browser
	/**
   @param wparam		: IN Unsigned message parameter
	@returns void
    */
	void		OnWmCommand(WPARAM wparam);

	/// Function to drop all triggers */
    /**
    @param hwnd    : Window HANDLE.
    @param tunnel  : Tunnel.
    @param mysql   : Mysql pointer.
    @param db      : Database name.
    */    
    wyInt32		DropTriggers(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db);

	
	//Handles refreshing of Index/Coulmn folder on creation/delition/updation of corresponding node
	/**
	@param hwnd		: IN treeview handle
	@param hitem	: IN selected node handle
	@param imgsel	: IN Selected node's image id
	@param tunnel	: IN Tunnel pointer
	@param mysql	: IN mysql MySQL reference
	@param database	: IN db name
	@param table	: IN table name
	@param isindex  : IN wyTrue if function called for Index, for column its wyFalse
	@return wyTrue on success else return wyFalse
	*/
	wyBool		RefreshIndexesOrColumnsFolder(HWND hwnd, HTREEITEM hitem, wyInt32 imgsel, Tunnel *tunnel, PMYSQL mysql, const wyChar *database, const wyChar *table, wyBool isindex, wyBool isrefresh);


    ///Function refreshes the table while editing/deleting columns/indexes
    /**
    @param hitem    : IN     handle to the tree item currently selected
    @param image    : IN/OUT selection image
    @returns handle to the item
    */
    HTREEITEM   RefreshTableAndSelecItem(HTREEITEM hitem, wyInt32& image);

	//Mark the engine menu item
	/**
	@param wnd			: IN MDIWindow handle
	@param hmenuhandle	: IN menu handle
	@param index		: IN submenu position
	@return VOID
	*/
	VOID		SelectTableEngineMenuItem(MDIWindow *wnd, HMENU hmenuhandle, wyInt32 index);

    wyBool      IsHitOnItem(LPARAM lparam);

    wyBool      SetFont();

    void    HandleOBFilter(wyWChar  *text = NULL, wyBool isAutoCmplt = wyTrue);

    void    FilterTreeView(wyWChar str[]);

    void    FilterDeleteList(wyWChar str[]);
    
    void    FilterBothTreeViewAndDeleteList(wyWChar str[]);

    void    OnFilterDeleteItem(HTREEITEM hti, wyElem *dltElement);

    void    OnFilterInsertItem(HTREEITEM hti, wyElem *dltElement);

    wyBool  OnItemSelectionChanging(LPNMTREEVIEW lpnmtv);

    void    DeAllocateLParam(HTREEITEM  hti, wyInt32 image);

    void    PrepareMessageText(wyInt32 toImage, HTREEITEM hti, wyWChar msg[], HTREEITEM hItem);

    wyBool    HandleOnEscapeOBFilter();

    static LRESULT CALLBACK     StCtrlProc(HWND hwnd, UINT message, WPARAM wpwaram, LPARAM lparam);

    static LRESULT CALLBACK     FilterProc(HWND hwnd, UINT message, WPARAM wpwaram, LPARAM lparam);

    static void CALLBACK OBFilterTimerProc(HWND hwnd, UINT message, UINT_PTR id, DWORD time); 

    void PaintFilterWindow(HWND hwnd);

	wyBool m_isTruncate;
   
	/*void    AddToDeleteList(HWND hwnd, TVITEMEX &tvi);*/

    wyBool      m_isClearVisible;

    wyBool      m_AllowRename;

    wyString    m_prevString;

#ifndef COMMUNTIY
	wyBool		m_isRegexChecked;
	HWND		m_hwndRegex;
	HWND		m_hwndRegexText;
	WNDPROC		m_RegexProc;
#endif

    wyString    m_matchString;

    wyBool      m_OBFilterWorkInProgress;

    wyBool      m_isSelValid;

    wyBool      m_isBkSpcOrDelOrClick;

    DWORD       m_startCaret;
    
    DWORD       m_endCaret;

    /*HTREEITEM   m_htiSelected;*/

    wyWChar     m_strOrig[70];
	
    /// Select type.
    wyInt32		m_seltype;

    /// Select database.
	wyString	m_seldatabase;

    /// Select table.
    wyString    m_seltable;

	/// Select column.
    wyString    m_selcolumn;

    /// Filter database.
    wyString    m_filterdb;

    /// Select trigger.
    wyString    m_seltrigger;

    /// Database Character Set
    wyString    m_dbcharset;

    /// Database Collation
    wyString    m_dbcollation;

    /// Parent window HANDLE.
	HWND		m_hwndparent;

    /// Windows HANDLE.
	HWND		m_hwnd;

    /// Check for expand.
	wyBool      m_toexpand;

    /// Check for refresh.
    wyBool      m_isrefresh;

    /// Window Procedure.
	WNDPROC		m_wporigproc;

    /// Node Details.
	drag_state	m_treedragger;

    /// Image list.
	HIMAGELIST	m_himl;

    /// Keeps of all expanded tree item nodes at a time
    wyWChar      **m_expandednodes;

    /// number of expanded nodes
    wyInt32     m_nodecount;

    /// nodes buffer size
    wyInt32     m_nodebuffsize;

	/// Flag to check the Table dragged or not.
	wyBool		m_dragged;   	

	/// Flag set wyTrue while only expand/collapse (For adding the node text to editor only on double click the treeview item)
	wyBool		m_isexpcollapse;

	//Menu handle for Tables folder , for drawing icons
	HMENU		m_tablesmenusub;

	//Glag sets wyTrue when a parent node is refreshed(for avoiding object bwoser gets blank)
	wyBool		m_isnoderefresh;	

    wyBool      m_isenable;

    HFONT       m_hfont;

    HWND        m_hwndFilter;

    HWND        m_hwndStParent;

    HWND        m_hwndStMsg;

    HWND        m_hwndStFilter;

    HWND        m_hwndStClear;

	HWND        m_hwndHTML;

    WNDPROC     m_stWndProc;

    WNDPROC     m_FilterProc;

    wyInt32     m_height;

    List        m_allocatedList;

};


class CAdvProp
{
public:
	CAdvProp();
	~CAdvProp();

    /// Callback dialog procedure for the window.
    /**
    @param hwnd     : Window Handler.
    @param message  : Message.
    @param wparam   : Unsigned message parameter.
    @param lparam   : LPARAM parameter.
    */
	static	INT_PTR CALLBACK AdvPropDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam); 		

    /// Handles grid window procedure.
    /**
    @param hwnd     : Window Handler.
    @param message  : Message.
    @param wparam   : Unsigned message parameter.
    @param lparam   : LPARAM parameter.
    */
	static	wyInt32 CALLBACK GVWndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam); 		

    /// Creates the actual dialog box and also initializes values of the class.
    /**
    @param hwndparent   : Window Handler.
    @param tunnel       : Tunnel pointer.
    @param umysql       : Mysql pointer.
    @param database     : Database.
    @param table        : Table.
    */
	wyBool		 Create(HWND hwndparent, Tunnel *tunnel, PMYSQL umysql, wyChar *database, wyChar *table);

    /// We create the column header for the listview control.
    /**
    */
	wyBool		 CreateListViewColumns();

    /// Fill the listview with values.
    /**
    */
	wyBool		 FillData();

    /// Initialization grid window.
    /**
    @param myres    : Mysql result pointer.
    */
	HWND		 InitGrid(HWND hwnd);

    /// Pointer to mysql.
	PMYSQL		  m_mysql;

    /// Tunnel pointer.
	Tunnel		 *m_tunnel;

    /// Database name.
	wyChar		 *m_db;

    /// Table name.
    wyChar       *m_tbl;

    /// Parent windows HANDLE.
	HWND		  m_hwndparent;

    /// List window HANDLE.
    HWND          m_hwndlist;

    /// Dialog window HANDLE.
    HWND          m_hwnddlg;

    /// Grid window HANDLE.
    HWND          m_hwndgrid;

    /// Val type in index column.
	wyInt32		  m_valtype;

	// check for object browser regfresh

	wyBool		m_isrefresh;

};



class CShowValue
{

public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	CShowValue();

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~CShowValue();


    /// Callback dialog procedure for the window.
    /**
    @param hwnd         : Window HANDLE.
    @param message      : Windows messages.
    @param wparam       : WPARAM parameter.
    @param lparam       : LPARAM parameter.
    */
	static	INT_PTR	CALLBACK ShowValueDialogProc(HWND hwnd, wyUInt32 message, WPARAM wParam, LPARAM lParam); 

    /// Window procedure for other dialogs.
    /**
    @param hwnd         : Window HANDLE.
    @param message      : Windows messages.
    @param wparam       : WPARAM parameter.
    @param lparam       : LPARAM parameter.
    */
	static	wyInt32	CALLBACK OtherDialogGridWndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

    /// Creates the actual dialog box and also initializes some values of the class.
    /**
    @param hwndparent    : Mysql result pointer.
    @param tunnel        : Tunnel pointer.
    @param umysql        : Pointer to mysql.
    @param valtype       : Value type.
    */
	wyBool			Create(HWND hwndparent, Tunnel * tunnel, PMYSQL umysql, wyInt32 valtype);

    /// Function creates the columns in the grid.
    /**
    @param myres    : Mysql result pointer.
    */
	wyBool			CreateColumns(MYSQL_RES* myres);
    
    /// Function creates the columns in the grid with proper length.
    /**
    @param myres    : Mysql result pointer.
    @param gvcol    : Gridview structure.
    */
    wyBool          CreateColumnsWithProperLength(MYSQL_RES *result, PGVCOLUMN	gvcol, wyChar **heading);

    /// Fill the listview with values.
    /**
    */
	wyBool			FillData();

    ///  Initializes the grid window.
    /**
    @param hwnd         : Windows Handler.
    */
	HWND			InitGrid(HWND hwnd);

    ///  Context menu process for showvalues dialog.
    /**
    @param pcshowvalue  : Text to find.
    @param hwnd         : Window HANDLE.
    @param lparam       : Long message parameter.
    */
	static	wyBool	CallBackContextMenu(HWND hwnd, CShowValue *pCShowValue, LPARAM lparam);

    ///Function to kill the selected process.
    /**
    */
	wyBool			KillProcess();

    ///  Command process for showvalues dialog.
    /**
    @param pcshowvalue  : Text to find.
    @param hwnd         : Windwos Handler.
    @param wparam       : WPARAM HANDLE.
    */
	static wyBool	ShowValueDialogProcCommand(CShowValue *pcshowvalue, HWND hwnd, WPARAM wparam);

	///Function is called when refresh button is clicked.
    /**
    */
	wyBool			OnRefreshClick();

	///Handles the grid Splitter Move
	/**
	@param wparam		: IN Column number
	@returns wyBool
	*/
	wyBool			OnSplitterMove(WPARAM wparam);

    /// MySql pointer.
	PMYSQL			 m_mysql;

    /// Tunnel pointer.
	Tunnel			 *m_tunnel;

    /// Parent window HANDLE.
	HWND			 m_hwndparent;

    /// Dialog HANDLE.
    HWND             m_hwnddlg;

    /// Grid HANDLE.
    HWND             m_hwndgrid;

    /// Value types.
	wyInt32			 m_valtype; 

    /// Rows.
    wyInt32          m_prow;

    /// Columns.
    wyInt32          m_pcol;
};



class CSchema
{
public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	CSchema();

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~CSchema();

    /// The dialog proc for the dialog box.
    /**
    @param hwnd         : Window HANDLE.
    @param message      : Windows messages.
    @param wparam       : WPARAM parameter.
    @param lparam       : LPARAM parameter.
    */
	static	INT_PTR CALLBACK SchemaDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam); 		

    ///  Find a item in a listview.
    /**
    @param text : text to find.
    */
			wyInt32		FindItem(CHAR *text);

    /// Builds the schema window
    /**
    @param pcqueryobject    : Queryobject pointer
    @param tunnel           : Tunnel pointer
    @param mysql            : Pointer to mysql pointer
    @param hitem            : Tree items
    */
			wyBool		Build(CQueryObject *pcqueryobject, Tunnel *tunnel, PMYSQL mysql, HTREEITEM hitem);

    /// Function to initialize the listview with the table names.
    /**
    */
			wyBool		FillData();

    /// Changes the text of tablename edit box when the user clicks the
    /// listview with the table.
    /**
    @param hwndfrom : Window Handler.
    @param nitem    : Items.
    */
			wyBool		ChangeTableName(HWND hwndfrom, INT nitem);

    /// Selects tables on Change from listbox.
    /**
    */
			wyBool		SelectTableOnChange();

    /// Adds selected tables from listbox.
    /**
    */
			wyBool		AddSelTables	();

    /// Adds all tables to database.
    /**
    */
			wyBool		AddAllTables	();

    /// Removes selected tables from database.
    /**
    */
			wyBool		RemoveTables	();

    /// Removes all tables from database.
    /**
    */
			wyBool		RemoveAllTables	();

    /// The function creates the schema.
    /**
    @param hwnd     : Window Handler
    */
			wyBool		Create(HWND hwnd);

    /// Write the headers in the HTML file of the schema.
    /**
    */
			wyBool		WriteHeaders();

    /// Writes table header to file.
    /**
    */
			wyBool		WriteTableHeaders();

    /// Writes table to a file .
    /**
    @param hwndmessage  : Window HANDLE 
    */
			wyBool		WriteTables(HWND hwndMessage);

    /// Function writes fields information for the table.
    /**
    @param hwndmessage  : Window HANDLE .
    @param table        : table name.
    */
			wyBool		WriteFields(HWND hwndMessage, wyChar *table);

    /// Writes index information for the table.
    /**
    @param hwndmessage  : Window HANDLE.
    @param table        : table name.
    */
			wyBool		WriteIndexes(HWND hwndMessage, wyChar *table);

    /// Function writes fk information for the table.
    /**
    @param hwndmessage  : Window HANDLE. 
    @param table        : table name.
    */
			wyBool		WriteFK(HWND hwndmessage, wyChar *table);

    /// Parses a CREATE TABLE statement and checks if any FK info is present.
    /**
    @param createstmt   : Create statement.
    */
			wyBool		FkInfoPresent(wyString &createstmt);

    /// Writes FK information to column.
    /**
    @param info : information to write.
    */
			wyBool		WriteFkInformation(wyChar *info);

    /// Takes care of the Command in SchemaDialogProcCommand.
    /**
    */
			wyBool		WriteFooters();

    /// Takes care of idok in SchemaDialogProcCommand.
    /**
    @param hwnd        : Window Handler.
    @param pcsschema   : Schema.
    */
	static	wyBool		SchemaDialogProcIdok(HWND hwnd , CSchema *pcschema);

    /// Takes care of the Command in SchemaDialogProcCommand.
    /**
    @param hwnd        : Window Handler.
    @param pcsschema   : Schema.
    @param wparam      : WPARAM parameter.
    */
	static	wyBool		SchemaDialogProcCommand(HWND hwnd , CSchema *pcschema, WPARAM wparam);

    /// Writes table to file.
    /**
    @param text        : To write.
    */
            wyBool      WriteTablesToFile(const wyChar *text);

    ///  Writes FK table information to file.
    /**
    @param query        : Query. 
    @param myrow        : Mysql row pointer.
    @param myres        : Mysql result set.
    @param table        : Table name.
    @param create       :
    @param hwndmessage  : Window HANDLE.
    */
            wyBool      WriteFKTableInfo(wyString &query, MYSQL_ROW *myrow, MYSQL_RES **myres,
                                            wyChar *table, wyString &create,HWND hwndmessage, wyInt32 *retval);
    

    ///  Writes the string to file. 
    /**
    @param string    : String to write.
    */
			wyBool		CSchema::WriteToFile(const wyChar *string);

    ///  Writes FK to html. 
    /**
    @param myrow    : Mysql row pointer.
    */
            wyBool      WriteFKAddinHtml(MYSQL_ROW *myrow);

    /// Gets the field info for write indexes.
	/**
	@param query    : Query.
    @param table    : table name.
    */
            wyBool      WriteIndexesGetFeildInfo(wyString &query, wyChar *table);

    /// Writes Schema message on to the window.
	/**
	@param format   : Format of message.
    @param val      : variables in the message.
    @param hwnd	    : Window Handler.
    */
            void        WriteSchemaMessage(wyChar *format, wyChar *val, HWND hwnd);



	/// Pointer to mysql pointer.
			PMYSQL		 m_mysql;

	// Tunnel pointer.
			Tunnel		*m_tunnel;

	/// Flag.
			wyBool		 m_flg;

	/// Changes.
			wyBool		m_change;

	/// Database name. 
			wyString	 m_db;

	// File name.
			wyString	 m_file;

	/// Windows HANDLE.
			HWND		 m_hwndparent, m_hwnddlg, m_hwndtables, m_hwndseltables, m_hwndedit;

	/// File HANDLE.
			HANDLE		 m_hfile;
};


class CCopyTable
{
public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	CCopyTable();

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~CCopyTable();

	/// Callback dialog procedure for the window.
	/**
	@param hwnd			: Window Handler.
	@param message		: Window message.
	@param wparam		: WPARAM pointer.
	@param lparam		: LPARAM pointer.
	*/
	static	INT_PTR	CALLBACK CopyTableDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam); 		


	/// Function creates the actual dialog box and also initializes some values of the class.
	/**
	@param hwndparent	: Window Handler.
	@param tunnel		: Tunnel pointer.
	@param umysql		: Pointer to mysql.
	@param table		: Table name.
	@param hitemtable	: Handle tree items.
	@param hitemdb		: Handle tree items.
    */
			wyBool			Create(HWND hwndparent, Tunnel * tunnel, PMYSQL umysql, wyChar *db, wyChar *table, HTREEITEM hitemtable, HTREEITEM hitemdb);

	/// Initialize the various startthing for the dialog box windows.
	/**
	@param hwnd	: Window Handler.
    */
			wyBool			InitWindows(HWND hwnd);

	/// Fill the listview with all the column names from the table.
	/**
    */
			wyBool			FillData();

	/// Toggles the state of the listwindow whenever the all field checkbox is selected or
	/// unselected.
	/**
    */
			wyBool			ToggleListState();

	/// Sets checkboxes in the listview if allfield checkbox is selected 
	/**
	@param state		: allfield checkbox selected or not 
    */
			void			SetCheckBoxes(wyBool state);

	// Helper function to prepare query.

	/// Function prepares the key thing and puts it in the buffer which is passed as parameter.
	/**
	@param keys : Keys.
    */
			wyBool			GetKeys(wyString &keys);

	/// Function prepares the select stmt and stores it in the buffer passed as parameter.
	/**
	@param select : Selected items.
    */
			wyBool			GetSelectStmt(wyString &select, wyString &alterQuery , wyWChar new_table[]);

			/// Function prepares the select stmt and stores it in the buffer passed as parameter.
			/**
			@param select : Selected items.
			*/
			wyBool			GetCheckConstraints(wyString &select, wyString &alterQuery, wyWChar new_table[]);

			

	/// Function copies the initial field stmt. into the buffer.
	/**
	@param query	: Query.
	@param neewtable: New tablename.
    */
			wyUInt32		GetFields(wyString &query, const wyWChar *newtable);

	/// Checks if the column is selected in the listview.
	/**
	@param colname : Column name.
    */
			wyBool			IsColSelected(wyWChar *colname);

	/// Function adds the autoincrement value correctly.
	/**
	@param newtable : Table name.
    */
			wyBool			AddExtraInfo(const wyChar *newtable);

	/// Function checks if any column are selected or not.
	/**
    */
			wyBool			IsAnyColSelected();

	/// Function to prepare the query to copy the table and execute it.
	/**
    */
			wyBool			DoCopy();

    /// Function adds the timestamp default value correctly.
	/**
	@param newtable : Table name.
    */
			wyBool			AddTimestampDefault(const wyChar *newtable);


	/// Function to copy the foreign key relationships.
	/**
    */
			wyBool			AddRelation();

	/// Sets back to the old database which was there before the execution was done.
    /**
    */
			wyBool			SetOldDB();

	/// Makes the new DB the default for the connection.
    /**
    */
			wyBool			SetNewDB(HWND hwndparent);

	/// Copies trigger of a particular table.
    /**
    @param database : Database name.
    @param table	: Name of the current table. 
    @param newtable	: New table name.
    */
			wyBool			CopyTriggers(const wyChar *database, const wyChar *table, const wyChar *newtable);

    /// Handles the Command option for the CopyTableDialogProc.
    /**
    @param hwnd     : Window HANDLE.
    @param cpt      : Copy Table pointer. 
    @param wparam	: WPARAM parameter.
    */
	static	wyBool			CopyTableDialogProcCommand(HWND hwnd, CCopyTable *cpt, WPARAM wparam);

    /// Create Initials for the tree view.
    /**
    @param hwndparent     : Parent window HANDLE.
    @param tvi            : Tree view . 
    @param tvins          : Tree view structure.
    @param hitemdb        : Handles tree items for database.
    @param hitemtable     : Handles tree items for items.
    @param hitemnew	      : Handles tree items for new tree.
    */
            wyBool          CreatetviSet(HWND hwndparent, TVITEM &tvi, TVINSERTSTRUCT &tvins, 
                                         HTREEITEM &hitemdb,HTREEITEM &hitemtable, HTREEITEM &hitemnew);

    /// Takes care of other index in getkeys()
    /**
    @param myrow    : Mysql row pointer.
    @param myres    : Mysql result pointer. 
    @param flgindex	: flag index.
    @param index    : index name.
    @param tempindex: temp index created.
    */
            wyBool          GetKeysOtherIndex(MYSQL_ROW *myrow, MYSQL_RES *myres, wyString &index);

    /// Handles Include index for getkey()
    /**
    @param query    : Query.
    @param myres    : Mysql result pointer. 
    */
            wyBool          GetKeysIfIncluded(wyString &query, MYSQL_RES **myres);

    /// Check for primary key in getkey()
    /**
    @param myrow        : Myrow pointer.
    @param myres        : Mysql result pointer. 
    @param primary      : Primary key name.
    @param flgprimary   : Primary flag.
    */
            wyBool          GetKeysPrimeKeyCheck(MYSQL_ROW *myrow, MYSQL_RES *myres, wyString &primary );

    /// Handles Include index for getkey()
    /**
    @param hwnd     : Window Handler.
    @param hwndedit : Edit window HANDLE.
    @param title    : Window title.
    */
            wyBool          InitWindowsSetDefaultName(HWND hwnd, HWND hwndedit, wyString &title);

    /// Add columns in getkey()
    /**
    @param lvc          : Column window .
    @param rectwin      : Rect HANDLE.
    @param title        : Window title.
    @param hwndstatics  : Window Handler.
    */
            wyBool          InitWindowsAddColumn(LVCOLUMN *lvc, RECT *rectwin, wyString &title, HWND hwndstatic);
            
			
		/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void CopyTableResize(HWND hwnd);

	/* Function to reposition the controls
	*/		
			void PositionCtrls();

	/* Function to add the dialog controls to a list
	*/		
			
			void GetCtrlRects();

	/* Sets the minimum size of the window
	@param lparam       : Long message parameter
	*/	
			void OnWMSizeInfo(LPARAM lparam);

	///Function thats draws the window contents using double buffering
        /**
        @param hwnd             : IN window handle
        @returns void
        */
        void                    OnPaint(HWND hwnd);

        


	/// MySQL pointer.
			PMYSQL			 m_mysql;

	/// Tunnel pointer.
			Tunnel			*m_tunnel;

	/// Parent window HANDLE.
			HWND			 m_hwndparent; 

	/// Window HANDLE.
			HWND			 m_hwnd;

	/// List HANDLE.
			HWND			 m_hwndlist;

	/// Index HANDLE.
			HWND			 m_hwndindex;

	/// Handler to fields.
			HWND			 m_hwndallfield; 

	/// Handler to structure.
			HWND			 m_hwndstruc;

	/// Handler to structure data.
			HWND			 m_hwndstrucdata;
	
	/// DataBase name.
			wyString		m_db; 

	/// Table name.
			wyString        m_table; 

	/// New table name.
			wyString        m_newtable;

	/// Table type.
			wyString        m_type;

	/// Create table options
			wyString		m_createoptions;
	
	/// Collation.
			wyString		m_collate;

	/// Comment.
			wyString        m_comment;

	/// Persist object to store information.
			Persist			*m_p;		

	/// List to store the dialogbox component values
			List			m_controllist;

	/// To store the original size of the dialog box
			RECT			m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
			RECT			m_wndrect;
};

#endif
