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

/*********************************************

Author: Vishal P.R

*********************************************/

#ifndef _USERMANAGER_H_
#define _USERMANAGER_H_

#include "SQLMaker.h"
#include "Tunnel.h"
#include "Global.h"
#include "wySQLite.h"

#define U_MAXLIMITATIONS 4

//structure that stores the privilege and respective context in which it is used
struct Privileges
{
    wyString    priv;
    wyInt32     context;
};

//Helper class used as a temperory buffer for the selected item
class PrivilegedObject
{
    public:

        ///Parameterized constructor
        /**
        @param privcount    : IN total number of privileges
        @param privileges   : IN privileges array used to compare the context while initializing the structure
        @param context      : IN context of the object
        @param value        : IN vlaue used to initialize the array
        */
        PrivilegedObject(wyInt32 privcount, Privileges** privileges, wyInt32 context, wyInt32 value = -1);

        //Destructor
        ~PrivilegedObject();

        //database name of the item
        wyString    m_dbname;

        //object name of the item
        wyString    m_objectname;

        //column name of the item
        wyString    m_columnname;

        //a dynamic array that holds the privileges for the item
        wyInt32*    m_privileges;

        //an integer that represents the object type
        wyInt32     m_objecttype;
};

typedef struct userlist
{
	wyString	m_uname;
	wyString	m_itemvalue;
	wyBool		m_dropdown;
	userlist	*next;
	
}USERLIST;

//User Manager class
class UserManager
{
    public:

        //constructor
        UserManager();

        //destructor
        ~UserManager();
        
        ///Function creates the User Manager dialog
        /**
        @param hwnd             : IN parent window handle
	    @returns end dialog code
        */
        wyInt32                 Create(HWND hwnd);

    protected:

        ///Standard window procedure for User Manager dialog box
        /**
        @param hwnd             : IN window handle
        @param message          : IN message
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns TRUE if the message is handled, else FALSE
        */
        static INT_PTR CALLBACK DlgProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

        ///Function initializes the dialog box
        /**
        @param hwnd             : IN dialog handle
        @returns wyTrue on success else wyFalse
        */
        wyBool                  InitDialog(HWND hwnd);

        ///Function gets the details and privileges for the user selected
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  PopulateUserInfo();

        ///Function handles the WM_COMMAND message
        /**
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns wyTrue if the message is handled else wyFalse
        */
        wyBool                  OnWMCommand(WPARAM wparam, LPARAM lparam);

        ///Handler for New User button
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  OnNewUser();

        ///Function that populates the contents of user combo
        /**
        @returns -1 on MySQL error else, >= 0
        */
        wyInt32                 PopulateUserCombo();

		//Function that checks if current server supports  authentication plugin 
		/**
		*/
		boolean IsAuthPluginSupported();

		//Function that populates the contents of plugin combo
		/**
		@returns -1 on MySQL error else, >= 0
		*/
		wyInt32					PopulateAuthPluginCombo();

		//Function that updates the selection on plugin combo from current user definitions on DB
		/**
		@returns -1 on MySQL error else, >= 0
		*/
		wyBool					GetUserCurrentAuthPlugin();

        ///Enumeration procedure that enables/disables the children
        /**
        @param hwnd             : IN child window handle
        @param lparam           : IN flag tells whether to enable the children
        @returns TRUE if enumeration was successful else FALSE
        */
        static BOOL CALLBACK    EnableChildren(HWND hwnd, LPARAM lparam);

        ///Gets the RECTs of the controls and stores in a linked list for resizing purpose
        /**
        @returns void
        */
        void                    GetCtrlRects();

        ///Function that positions the controls in the dialog box while resizing
        /**
        @returns void
        */
        void                    PositionCtrls();

        ///Handler for CB_CHANGE notification. Function deletes the current memmory structures and fills with the user selected
        /**
        @returns void
        */
        void                    OnUserComboChange();

		void					OnHandleEditChange();

        ///Initializes the tree view
        /**
        @returns void
        */
        void                    InitTreeView();

        ///Creates the image lists and other icons used in the dialog
        /**
        @returns void
        */
        void                    CreateImageList();

        ///Handler for WM_NOTIFY message
        /**
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns wyTrue if the message is to be processed else wyFalse
        */
        wyBool                  OnWMNotify(WPARAM wparam, LPARAM lparam);

        ///Handles whenever the tree view selection is changed
        /**
        @param lparam           : IN the current selected item, if this is NULL, then it will get the current selection
        @returns void
        */
        void                    OnTreeViewSelChanged(LPARAM lparam = NULL);

        ///Function show/hides the controls based on the image index of the selection in the tree view
        /**
        @param imageindex       : IN image index of the item selected in the tree view
        @returns void
        */
        void                    ShowHideControls(wyInt32 imageindex);

        ///Sets the control ranges of the spin controls
        /**
        @param maxquery         : IN max range of the max query spin
        @param maxupdate        : IN max range of the max update spin
        @param maxconn          : IN max range of the max connection spin
        @param maxsimconn       : IN max range of the max user connection spin
        @returns void
        */
        void                    SetUDConrolsRange(wyInt32 maxquery = INT_MAX, wyInt32 maxupdate = INT_MAX, wyInt32 maxconn = INT_MAX, wyInt32 maxsimconn = INT_MAX);

        ///Function sets the dirty flag and enables/disables the Save/Cancel buttons based on the flag
        /**
        @param set              : IN whether to set/reset the flag
        @returns void
        */
        void                    SetDirtyFlag(wyBool set = wyTrue);

        ///Function enables/disables the Save/Cancel buttons based on the dirty flag
        /**
        @returns void
        */
        void                    EnableDisableSaveCancel();

        ///Handler for Save/Cancel buttons
        /**
        @returns void
        */
        void                    OnCancelChanges();

        ///Function sets the text in the various controls for the Edit User page
        /**
        @returns void
        */
        void                    FillUserInfo();

        ///Function enables the user page controls
        /**
        @param                  : IN flag tells the action to be performed for a new user
        @returns void
        */
        void                    EnableUserControls(wyBool isnewuser = wyTrue);

        ///Function handles the Delete User button
        /**
        @returns void
        */
        void                    OnDeleteUser();

        ///Function gets the available privileges for the server
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GetServerPrivileges();

        ///Function deletes all the user level items and reset any per user flag
        /**
        @returns void
        */
        void                    DeleteUserLevelItems();

        ///Function updates the list view contents based on the image index of tree view selection
        /**
        @param imageindex       : IN image index of the item selected in the tree view
        @returns void
        */
        void                    UpdateListView(wyInt32 imageindex);

        ///Function that handles whenever a list view item is checked/unchecked
        /**
        @param lparam           : IN notification pointer
        @returns void
        */
        void                    OnListViewItemChanged(LPARAM lparam);

        ///Function selects/deselects all the items in the list view control
        /**
        @returns void
        */
        void                    OnSelectAllCheck();

        ///Function adds the context sensitive privileges based on the image index of the tree view selection
        /**
        @param imageindex       : IN image index of the selected item in the tree view
        @returns void
        */
        void                    PopulatePrivsBasedOnContext(wyInt32 imageindex);

        ///Function checks the items in the list view that matches with the privileges of the selected item
        /**
        @param imageindex       : IN image index of the selected item in the tree view
        @returns void
        */
        void                    FillPrivileges(wyInt32 imageindex);

        ///Function inserts all the databases in the tree view
        /**
        @returns void
        */
        void                    InsertDatabases();

        ///Function inserts the basic nodes in the tree views
        /**
        @param hwndtv           : IN tree view handle
        @returns handle to the Object Level Privileges node
        */
        HTREEITEM               InitTreeViewNodes(HWND hwndtv);

        ///Function prmpts the user to save the changes and saves/discards accordingly
        /**
        @retusn wyTrue if the operation was successful else wyFalse
        */
        wyBool                  SavePrompt();

        ///Function handles the notification when an item in the tree view is about to expand
        /**
        @param lparam           : IN notification structure
        @returns wyTrue to allow expanding else wyFalse
        */
        wyBool                  OnTreeViewItemExpanding(LPARAM lparam);


		//Function that Saves the contents of plugin combo
		/**
		@returns treu on sucess false on failure>
		*/
		wyBool					SaveAuthPlugin();

		/*
		wyBool IsServerMariaDb(wyString version);

		wyString GetServerVersion();

		wyInt32 GetVersionNo(wyString version);
		*/

		///Function creates the two SQLite tables used to store the privileges in memeory
        /**
        @returns void
        */
        void                    CreatePrivilegeTables();

        ///Function inserts the formed privileged object into SQLite table
        /**
        @returns void
        */
        void                    InsertIntoSQLite(PrivilegedObject*);

        ///Function converts the image index given to currusponding context
        /**
        @param                  : IN image index to be converted
        @returns -1 if the conversion failed, >=0 for succesfull conversion
        */
        wyInt32                 ImageIndexToContext(wyInt32 imageindex);

        ///Function updates the information required to uniqly identify the selected object in the server
        /**
        @param imageindex       : IN image index of the item selected in the tree view
        @returns void
        */
        void                    SetSelectedObjectInfo(PrivilegedObject* privobj = NULL);

        ///Function updates the SQLite working copy table 
        /**
        @param context          : IN context id of the object selected
        @param skipoptimization : IN flag tells whether to check the current list view item state with initial state
        @returns void
        */
        void                    UpdateSQLite(wyInt32 context, wyBool skipoptimization);

        ///Handler for the notification when the selection in the tree view is about to change
        /**
        @param lparam           : IN notification structure
        @returns void
        */
        void                    OnTreeViewSelChanging(LPARAM lparam);

        ///Helper function that gets the user privileges by calling other functions
        /**
        @retruns void
        */
        void                    GetUserPrivileges();

        ///Function gets the global privileges for the user
        /**
        @retruns void
        */
        void                    GetGlobalPrivileges();

        ///Function gets the DB level privileges for the user
        /**
        @retruns void
        */
        void                    GetDBPrivileges();

        ///Function gets the routine level privileges for the user
        /**
        @retruns void
        */
        void                    GetRoutinePrivileges();

        ///Function gets the table level privileges for the user
        /**
        @retruns void
        */
        void                    GetTablePrivileges();

        ///Function gets the column level privileges for the user
        /**
        @retruns void
        */
        void                    GetColumnPrivileges();

        ///Function gets the privileges-column mapping 
        /**
        @param key              : IN the string for which the mapping is required
        @param iscolumnname     : IN flag that tells whether the key should be interpreted as column name
        @returns mapping for the key given on success else NULL
        */
        wyChar*                 GetPrivilegeTableMapping(wyString* key, wyBool iscolumnname = wyFalse);

        ///Function gets the available privs in the DB level
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GetServerPrivsForDB();

        ///Function gets the available privs for tables and columns
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GetServerPrivsForTableAndColumn();

        ///Function gets the available privs for rountines
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GetServerPrivsForRoutine();

        ///Function identifies all the privileges given in the first parameter and stores the currusponding mapping index in the second parameter
        /**
        @param value            : IN  string to parse
        @param indexarray       : OUT array that holds the mapping indices of the privileges found in the value string
        @param istype           : IN  flag tells whether the first parameter need to be processed as a data type
        @returns the number of privileges successfuly indexed
        */
        wyInt32                 GetPrivilegeIndexes(wyString& value, wyInt32* indexarray, wyBool istype);

        ///Compare function used in qsort
        /**
        @param elem1            : IN first element
        @param elem2            : IN second element
        @returns -1 if elem1 < elem2, 0 if elem1 = elem2, 1 if elem1 > elem2
        */
        static wyInt32          CompareFunct(const void* elem1, const void* elem2);

        ///Function saves/discards all the changes made by the user
        /**
        @param issave           : IN flag tells whether to save or discard
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ApplyChanges(wyBool issave);

        ///Handle for Save button
        /**
        @returns void
        */
        void                    OnSaveChanges();

        ///Function truncates the SQLite desttable and copies the contents of srctable
        /**
        @param desttable        : IN target SQLite table name
        @param srctable         : IN source SQLite table name
        @param validate         : IN the flag tells to discard rows with no > 0 value in the privileg columns while copying
        @returns void
        */
        void                    TruncateAndReplacePrivTable(wyChar* desttable, wyChar* srctable, wyBool validate = wyTrue);

        ///Function inserts the privileged objects into the tree view
        /**
        @returns void
        */
        void                    InsertPrivilegedObject();

        ///Helper function that recursively inserts any parent item needed before inserting the item selected from the SQLite table
        /**
        @param hobjectpriv      : IN handle to the parent item
        @param context          : IN context of the item to be inserted
        @returns handle to the inserted item
        */
        HTREEITEM               InsertPrivilegedObjectHelper(HTREEITEM hobjectpriv, wyInt32 context);

        ///Function checkes whether the item selected from the SQLite table is a view or not
        /**
        @returns wyTrue if it is a view else wyFalse
        */
        wyBool                  IsView();

        ///Function processes the two SQLite tables and calls the functions to prepare and execute the query required
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ProcessGrantRevoke();

        ///Fucntion prepares the grant or revoke query required
        /**
        @param privstmt         : IN handle to the SQLite stmt for the working copy
        @param privorigstmt     : IN handle to the SQLite stmt for the original copy
        @returns wyTrue on succes else wyFalse
        */
        wyBool                  PrepareGrantRevokeQuery(sqlite3_stmt** privstmt, sqlite3_stmt** privorigstmt);

        ///Function executes the grant and revoke query supplied
        /**
        @param grantquery       : IN grant query to be executed
        @param revokequery      : IN revoke query to be executed
        @return wyTrue on succes else wyFalse
        */
        wyBool                  ExecuteGrantRevoke(wyString* grantquery, wyString* revokequery);

        ///Function returs the visible tree view handle
        /**
        @returns the handle of the visible tree view
        */
        HWND                    GetTreeViewHandle();

        ///Function switches the visiblity of the two tree views
        /**
        @param show             : IN flag tells whether to show the tree view showing all the objects
        @returns void
        */
        void                    ShowAllObjects(wyBool show = wyTrue);

        ///Function executes the query required to drop the user
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  DropUser();

        ///Function executes the queries required to rename the user
        /**
        @returns -1 if no change detected, 1 on successfull renaming and 0 on failiure
        */
        wyInt32                 RenameUser();

        ///Function executes the query required to apply the limiations
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ApplyLimitations();

        ///Helper function used to execute various queries in User Manager
        /**
        @param query            : IN query to be executed
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ExecuteUMQuery(wyString& query);

        ///Function executes the queries required to add a new user
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  AddNewUser();

        ///Function checks whether there is any password changes and executes the query required to save the changes
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  SavePassword();

		
        ///Helper function to execute FLUSH PRVILEGES
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  FlushPrivileges();

        ///Function handles the context help request
        /**
        @param ishelpclicked    : IN flag tells whether the function is invoked using the help button
        @returns void
        */
        void                    HandleHelp(wyBool ishelpclicked = wyFalse);

        ///Function handles WM_RESIZE message
        /**
        @returns void
        */
        void                    OnResize();

        ///Function thats draws the window contents using double buffering
        /**
        @param hwnd             : IN window handle
        @returns void
        */
        void                    OnPaint(HWND hwnd);

        ///Function to set the minimum size of the dialog
        /**
        @param lparam           : IN message structure
        @returns void
        */
        void                    OnWMSizeInfo(LPARAM lparam);

        ///Subclassed procedure to draw the frme gripper on a static control
        /**
        @param hwnd             : IN window handle
        @param message          : IN message
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns TRUE if the message is handled, else FALSE
        */
        static BOOL CALLBACK    GripProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

        ///Function to maintain the persistance while repopulating the privileged objects
        /**
        @param hobjpriv         : IN handle to the Privileged Objects tree view node
        @param privobj          : IN structure that uniquely identifies an object in the server
        @returns the handle to the tree view item that previously were selected or the immediate parent
        */
        HTREEITEM               GetPrevSelection(HTREEITEM hobjpriv, PrivilegedObject* privobj);

        ///Function to find a node with the name given under a given parent in the tree view
        /**
        @param hwndtv           : IN handle to the tree view
        @param hparent          : IN handle to the parent item
        @param name             : IN name of the item to be searched for
        @param imageindex       : IN image index of that item
        @returns the handle to the node if found else NULL
        */
        HTREEITEM               FindNode(HWND hwndtv, HTREEITEM hparent, wyString* name, wyInt32 imageindex);

        void                    OnUMError(const wyChar* query, wyBool isinitializing = wyFalse);

        wyString&               EscapeSQLiteString(const wyChar* str, wyString& string);

        wyString&               EscapeMySQLString(const wyChar* str, wyString& string);

        void                    SetResetDBContext(wyBool isset);

    private:

		USERLIST				*m_userlist;

        //whether to show the respective limitations
        wyBool                  m_showlimitations[U_MAXLIMITATIONS];

        //static array to hold the mysql privileges and currusponding column names till date
        static wyChar*          m_privmapping[];

        //dialog handle
        HWND                    m_hwnd;

        //flag tells whether unsaved changes are there
        wyBool                  m_isedited;

        //mdi window pointer
        MDIWindow*              m_hmdi;

        //selected index in the combo box
        wyInt32                 m_selindex;

        //user name of the selected user
        wyString                m_username;

        //host name of the selected user
        wyString                m_host;

		wyInt32					m_usercount;

        //tree view image list
        HIMAGELIST              m_himagelist;

        //flag tells whether the initialization is done, the same flag is also used some placed to get the desired behaviour
        static wyBool           m_initcompleted;

        //flag tells whether the new user page is in use
        wyBool                  m_isnewuser;
        
        //an array that holds the structures representing each available privileges and the context it is used
        Privileges**            m_privarray;

        //total number of privileges available for the server
        wyInt32                 m_privcount;

        //the index of the GRANT privilege in the m_privarray; used for easy access to that particular item
        wyInt32                 m_grantoptionindex;

        //array holds the available limitations
        wyInt32                 m_limitations[U_MAXLIMITATIONS];

        //flag tells whether the change is automated or manual
        wyBool                  m_isautomatedchange;

        //flag tells whehter the user clicked on select all check
        wyBool                  m_isselectallcheck;

        //handle to the note window
        HWND                    m_hwndnote;

        //the image index of the root icon in image list
        wyInt32                 m_umimageindex;

        //total image count in the image list
        wyInt32                 m_imagecount;

        //flag tells whether the server is >= 5.02
        wyBool                  m_ismysql502;

		//flag tells whether the server is MariaDb
		wyBool                   m_ismariadb;
		//flag tells whether the server is MariaDb higher then 10.4
		wyBool                   m_ismariadb104;
		//flag tells whether the server is MariaDb higher then 10.4
		wyBool                   m_ismariadb55;
        //name of the currently selected db in tree view
        wyString                m_currentdb;

        //name of the currently selected object in the tree view
        wyString                m_currentobject;

        //name of the currently selected column in the tree view
        wyString                m_currentcolumn;

        //SQLite wraper class object
        wySQLite                m_sqlite;

        //tells whether the user want to see all the objects or only privileged objects
        wyBool                  m_isallradiochecked;

        //index of the last checked item in the list view; used to implement functionality of shift click select
        wyInt32                 m_lastcheckedindex;

        //flag tells whether the user has modified the password field
        wyBool                  m_ispasswordchanged;

        //flag tells the whether the user combobox selection is changing
        wyBool                  m_isusercombochanging;

        //handle to the user icon
        HICON                   m_husericon;

        //handle to the global priv icon
        HICON                   m_hglobalprivicon;

        //handle to the object priv icon
        HICON                   m_hobjprivicon;

        //a list used to store all the controls and some attributes that is used for resising the dialog
        List                    m_controllist;

        //the client rectangle of the dialog box
        RECT                    m_dlgrect;

        //window rectangle of the dialog box
        RECT                    m_wndrect;

        //member stores the subclassing procedure for the static control showing the gripper
        WNDPROC                 m_gripproc;

        wyBool                  m_isdeleteuser;

        //member stores the database context on starting UM and restore it before closing
        wyString                m_selecteddatabase;

		//plugin name of the selected user
		wyString                m_authpluginname;
	
		wyInt32					m_authplugincount;
	
		// stores the current server version number
		wyInt32					m_serververno;

		// holds current server full version string 
		wyString				m_versionfull;

		// holds current default plugin for authentication
		wyString		m_defaultAuthPlugin;
};


///API to invoke User Manager
/**
@returns the return value of Create method of User Manager
*/
wyInt32    CreateUserManager();

#endif _USERMANAGER_H_