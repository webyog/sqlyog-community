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


#ifndef __CSHAREDFUNCS__
#define __CSHAREDFUNCS__

#include "Datatype.h"
#include "sqlite3.h"
#include "Global.h"
#include <HtmlHelp.h>
#include <shlobj.h>
#include "FileHelper.h"
#include "MySQLVersionHelper.h"
#include "wyTheme.h"

class MySQLDataEx;
class MDIlist;
class tabdetailelem;


#if defined _WIN32 && ! defined _CONSOLE
#include "Scintilla.h"
#include "L10n.h"
#endif

#define GENERALPREF L"GENERALPREF"
#define MYSQL_SERVER_GONE	2006
#define MNU_OTHEROPT_INDEX   11

#define		TXT_COLUMNS			_(L"Columns")
#define		TXT_INDEXES			_(L"Indexes")
#define		TXT_PROCEDURES		_(L"Stored Procs")
#define		TXT_FUNCTIONS		_(L"Functions")
#define		TXT_VIEWS			_(L"Views")
#define		TXT_TABLES			_(L"Tables")
#define		TXT_EVENTS			_(L"Events")
#define		TXT_TRIGGERS		_(L"Triggers")

//Colors for color combo box
#define COLOR_RED			RGB(255, 227, 227)
#define COLOR_BLUE			RGB(244, 236, 255)
#define COLOR_GREY			RGB(245, 245, 245)
#define COLOR_GREEN2		RGB(234, 234, 174)
#define COLOR_GREEN			RGB(249, 255, 239)
#define COLOR_BROWN			RGB(255, 240, 245)
#define COLOR_YELLOW		RGB(255, 250, 205)
#define COLOR_PINK			RGB(255, 230, 255)
#define COLOR_GREEN1		RGB(240, 255, 240)
#define COLOR_BLUE2	        RGB(224, 236, 255)
#define COLOR_WHITE	        RGB(255, 255, 255)
#define COLOR_BLUE3			RGB(207, 250, 254)

//Minimum number of items in a combobox
#define	CBMINVISIBLEITEM    30

#define		RETAINWIDTH_DBNAME		L"__webyog"
#define		TABLEDATAOPTION_STARTROWS "__startrows"
#define		TABLEDATAOPTION_NUMROWS "__numrows"

#define		RESUTTAB_PAGING			"__crcquery"

#define		INFOTABFORMATOPTION_DEFAULT		1 //1 for 'HTML', 0 for 'Text'

//...........................................................................................................
//.CSS Classes
#define CSS_CLASSES ".resultcaptionstyle	{font: 14px  \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left;}\
.colcaptionstyleleft {font: bold 12px \"Courier New\", Courier, mono; background:#E0E0E0; text-align:left; padding-left:10px; padding-right:10x}\
.colcaptionstyleright{font: bold 12px \"Courier New\", Courier, mono; background:#E0E0E0; text-align:right; padding-right:10px; padding-left:10x}\
.cellstyleleft{text-align:left;padding-left:10px; padding-right:10px;}\
.cellstyleright{text-align:right;padding-right:10px; padding-left:10px;}\
.captionfontstyle{font: bold 12px \"Courier New\", Courier, mono; text-align:right;}\
.datafontstylerowodd{font: 12px \"Courier New\", Courier, mono; text-align:right; background:#E2E5EE;height:18px;}\
.datafontstyle{font: 12px \"Courier New\", Courier, mono; text-align:right;}\
.datafontstyleroweven{font: 12px \"Courier New\", Courier, mono; text-align:right; background:#FFFFFF;height:18px;}\
.tablestyle{border:2px solid #EEE1FF;}\
.statustablestyle{border:2px solid #EEE1FF;}\
.extendedlevelcaptionstyle{font: bold 12px \"Courier New\", Courier, mono; text-align:right;}\
.extendedleveldatastyle{font: 12px \"Courier New\", Courier, mono; text-align:right;}\
.warningstyle{font: 13px  \"Courier New\", Courier, mono; text-align:left; color:grey;}\
.pkcolcaptionstyle{background:#E0E0E0;}"  

#define CSS_OPTIMIZE_CLASS "\
.datafontstyle_optimize{font: 12px \"Courier New\", Courier, mono; text-align:right; background:#FF0000;height:18px;}"

#define CSS_WARNING ".tabcaptionstyle{font: 14px \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left; color:grey;}\
					.tabresultpagestyle{font: 14px \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left; color:red;}"

#define CSS_CLASS2 ".extendedcodecaptionstyle{font: bold 12px \"Courier New\", Courier, mono; text-align:right; padding-left:10px; padding-right:10px;}\
.extendedcodedatastyle{font: 12px \"Courier New\", Courier, mono; text-align:right; padding-left:10px; padding-right:10px;}"  

typedef struct treeviewparams
{
	HWND		hwnd;
	LPTVITEM	tvi;
	Tunnel		*tunnel;
	PMYSQL		mysql;
	wyWChar		*database;
	wyBool		issingletable;
	wyBool		isopentables;
	wyBool		isrefresh;
	wyBool		checkboxstate;
}TREEVIEWPARAMS;

//File types with SQLyog.This used to handle the 'Open' file process
enum SQLyogFileType
{
	SQLFILE = 0,
	SDFILE	= 1,
	QBFILE	= 2,
	FILEINVALID = -1
};

class DltElement : public wyElem
{
public:
    DltElement(TVITEM    &tvi);

    ~DltElement();

    TVITEM    m_tvi;

    List      m_child;
};


class OBDltElementParam : public wyElem
{
public:
    wyWChar m_filterText[70];
    List    m_deleteList;
    LPARAM  m_lvalue;

    OBDltElementParam(LPARAM value);
    ~OBDltElementParam();
};


//Helper class to store the controls in the dialg
class DlgControl : public wyElem
{
    public:

        ///Parameterized constructor
        /**
        @param hwnd         : IN handle to the control
        @param id           : IN control id
        @param rect         : IN bounding rectangle of the control
        @param issizex      : IN horizontal sizing enabled
        @param issizey      : IN vertical sizing enabled
        */
        DlgControl(HWND hwnd, wyInt32 id, RECT* rect, wyBool issizex, wyBool issizey);

        //Destructor
        ~DlgControl();

        //handle to the control
        HWND    m_hwnd;

        //bounding rectangle of the control
        RECT    m_rect;

        //control id
        wyInt32 m_id;

        //horizontal resizing
        wyBool  m_issizex;

        //vertical resizing
        wyBool  m_issizey;
};

/// Executes the sql code
/**
@param db           : IN Database openhandle
@param query        : IN Query
@param xcallback    : IN Sqlite callback routine
@param parg         : IN First argument to xCallback()
@param errmsg       : IN The error message, if occurs
@returns sqlite message
*/
wyInt32 YogSqliteExec(sqlite3 *db, const wyChar *query, sqlite3_callback xcallback, void *parg);//, wyChar **errmsg);

/// Sets the SQLite connection to non sync mode 
/**
@param db           : IN Database openhandle
@returns sqlite message
*/
wyInt32 SetSqliteSyncMode(sqlite3 *db);


/// Sets the SQLite cache size
/**
@param db           : IN Database openhandle
@returns sqlite message
*/
wyInt32	SetSQLiteCache(sqlite3 *db);

/// Sets the SQLite Journal mode to WAL
/**
@param db           : IN Database openhandle
@returns sqlite message
*/
wyInt32 SetSqliteJournalMode(sqlite3 *db);

/// Sets the SQLite cache size
/**
@param db           : IN Database openhandle
@returns sqlite message
*/
wyInt32 SetSQLitePage(sqlite3 *db);

/// Sets the SQLite connection to temp store to MEMORY
/**
@param db           : IN Database openhandle
@returns sqlite message
*/
wyInt32 SetTempStore(sqlite3 * hdb);

/// Used for sqlite to count number of rows that query returns. 
/**
@param count        : IN Number of rows
@param argc         : IN Argument count
@param argv         : IN Argument vector
@param colname      : IN Number of rows    
@returns total number of rows
*/
wyInt32 callback_count(void *count, wyInt32 argc, 
                       wyChar **argv, wyChar **colname);

/// Function to initialize the tags structure. 
/**
@param tags         : OUT Structure to store keyword information retrieved from sqlite database
@param tagtype      : IN Type of tag
@param typeflag     : IN Checks whether to be included or not
@returns void
*/	
void InitTagsArray(tagArray *tags, wyInt32 tagtype, 
                   wyBool typeflag);

/// Opens the keywords.db file
/**
@param phdb         : IN Pointer to sqlite 3 structure
@returns wyTrue on success else wyFalse
*/
wyBool OpenKeyWordsDB(sqlite3 **phdb);

/// Converts the given string to utf8.
/**
@param ansistr      : OUT String to convert.
@return utf8 string
*/
wyChar *GetUtf8String(const wyChar *ansistr);

/// Gets the active connection window pointer
/**
@return MDIWindow pointer
*/
MDIWindow * GetActiveWin();
MDIWindow * GetActiveWinByPost();
/// Gets back the max bulk size
/**
@param              : OUT The resultant size
@returns true on success
*/
wyInt32 GetBulkSize(wyInt32 * bulksize);

///// Rotate string left , bitwise
///**
//@param  str             : IN/OUT String to rotate
//@returns void
//*/
//void	RotateBitRight(UCHAR *str);
//
///// Rotate string left , bitwise
///**
//@param  str          : IN/OUT String to rotate
//@returns void
//*/
//void    RotateBitLeft (UCHAR *str);

/// Checks for the autocomplete 
/**
@param path          : OUT The resultant path for auto complete
@returns wyTrue on success else wyFalse
*/
wyBool CheckForAutoCompleteDir(wyString &path);

/// Function enables and disables various File menu item like cut, copy, paste etc. 
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					ChangeFileMenuItem(HMENU hmenu);

/// Function enables and disables various edit menu item like cut, copy, paste etc. 
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					ChangeEditMenuItem(HMENU hmenu);

///Handle the Edit menu(Remove Formatter options with PRO)
/**
@param hmenu         : IN Handle to the menu
@return void
*/
VOID					HandleFomatterOptionsPRO(HMENU hmenu);

void                    HandleRemoveExplainOptions(HMENU hmenu);

void                    SetExplainMenuItems(HMENU hmenu, HWND hwnd);

void                    GetCurrentQuery(HWND hwndedit, MDIWindow *wnd, wyString *query, wyInt32 &pos);

void                    GetCurrentQueryForAuto(HWND hwndedit, MDIWindow *wnd, wyString *query, wyInt32 &pos);
///Handle the File menu(Remove QB, SD options with PRO)
/**
@param hmenu         : IN Handle to the menu
@return void
*/
VOID					HandleFileMenuOptionsProEnt(HMENU hmenu);

///Handle the DB/Table menu
/**
@param hmenu         : IN Handle to the menu
@return void
*/
VOID					HandleDBMenuOptionsPRO(HMENU hmenu);

///Handle the Power Tools menu(with PRO)
/**
@param hmenu         : IN Handle to the menu
@return void
*/
VOID					HandlePowertoolsOptionsPRO(HMENU hmenu);

/// Function disables or enables Tool menu item depending upon what type of item is 
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnableToolItems(HMENU hmenu);

/// Function disables or enables Power menu item depending upon what type of item is selected in the object browser.
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnablePowerToolItems(HMENU hmenu);

/// Function to enable and disable all the items in DB menu depending upon the item selected in the object browser.
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnableDBItems(HMENU hmenu);

/// Function to enable and disable all the items in Table menu depending upon the item selected in the object browser.
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnableTableItems(HMENU hmenu);

/// Function to enable and disable all the items in Objects menu depending upon the item selected in the object browser.
/**
@param hmenu         : IN Handle to the menu
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnableColumnItems(HMENU hmenu);

/// Function to enable favorite menu items
/**
@param hmenu         : IN Favorites menu handle
@returns wyTrue if success, otherwise wyFalse
*/
wyBool					EnableFavoriteMenu(HMENU  hmenu);

/// Disables the advance menu
/**
@param hmenu         : IN Menu HANDLE
@returns void
*/
void					DisableAdvMenu(HMENU hmenu);

/// Adds the tool bar buttons
/**
@param hinst		: IN Resource module HANDLE
@param htoolbar		: IN HANDLE to toolbar
@param himglist		: IN HANDLE to image list
@param command		: IN Button ids
@param size			: IN Number of commands
@param states		: IN State of buttons
@param imgres		: IN Image resource
@returns void
*/
void                    AddToolBarButtons(HINSTANCE hinst, HWND htoolbar, HIMAGELIST himglist, wyInt32 *command, 
                            wyInt32 size, wyUInt32 (*states)[2], wyInt32 *imgres);

/// Initializes the connection information
/**
@param consrc       : IN Connection information
@param contgt       : OUT Connection information for plink shell 
*/
void                    InitConInfo(ConnectionInfo &consrc, ConnectionInfo &contgt);

/// Sets the character set for the particular connection
/**
@param wnd          : IN Current window pointer
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param profile      : IN Perform in background or not
*/
void				SetCharacterSet(MDIWindow *wnd, Tunnel *tunnel, MYSQL *mysql, wyString &cpname, wyBool reconnect, wyBool profile = wyTrue);

/// Destroys the dialog
/*
@param hdlg			: IN Window HANDLE
@param nresult		: IN Value to be returned that created that dialog
*/
wyBool	yog_enddialog(HWND hdlg, INT_PTR nresult);


/// Checks for empty query
/**
@param query		: IN query
*/
wyBool IsQueryEmpty(const wyWChar *query);

/// Shows the current MYSQL error
/**
@param hdlg          : IN Dialog window handle
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Mysql pointer
@returns void
*/
void				ShowMySQLError(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query = NULL, wyBool val = wyFalse);

/// Shows the current MYSQL warnings
/**
@param hdlg          : IN Dialog window handle
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Mysql pointer
@param query         : IN query to be dusplayed
@param reswarning    : IN mysql result set for 'show warnings'
@returns void
*/
void				ShowMySQLWarnings(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query, MYSQL_RES *reswarning);

/// writes the connection details in the ini file
/** 
@param hdlg          : IN Dialog window handle
@returns void
*/
void					WriteConnDetails(HWND hdlg);

/// Shows Help on particular topic
/**
@param helpid		: IN Help id
*/
wyBool	ShowHelp(wyChar * helpid);

// Function to compare two values sent by the qsort and binary search function.
/**
@param arg1			: IN First argument 
@param arg2			: IN second argument
@returns zero if both the strings are identical else non zero value
*/
wyInt32 compareid	(const	void *arg1,	const void *arg2);

/// Checks for existence of connection.
/**
@param hwndcombo   : IN Window Handler 
@param path        : IN Path
@param connname    : IN connection name
@param connspe     : IN Connection Specifics
@returns wyTrue on success else wyFalse
*/
wyInt32     IsConnectionExists(HWND hwndcombo, wyString &path, wyWChar *connname, wyChar *connspe);

/// Redraws the scintilla window
/**
*/
void        RepaintTabModule();

/// Get the field index for columns. 
/**
@param result		: IN Mysql result set
@param colname		: IN Column name
*/
wyInt32		GetFieldIndex(MYSQL_RES * result, wyChar * colname, Tunnel *tunnel, PMYSQL mysql = NULL);

/// Gets the MYSQL error 
/**
@param hdlg         : IN Dialog window handle
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param err          : IN Error string
@returns void
*/
wyInt32					GetMySQLError(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, wyString &err);

/// Tells whether the OS is Win95/98/Me or not.
/**
@returns wyTrue if 9x else wyFalse
*/
wyBool	IsWin9x();

/// making sure the crash dump functions are supported
/**
@returns wyTrue if supported else wyFalse
*/
wyBool  IsCrashDumpSupported();

/// Overwrites an existing file
/**
@param hwnd			: IN Window HANDLE
@param file			: IN File name
*/
wyBool	OverWriteFile(HWND hwnd, const wyWChar * file);

// Function to display formatted error message in a yog_message.
// It will show the szExtraText first with the standard system error at end.
/**
@param dwlasterror	: IN Last error
@param extratext	: IN Extra text to be printed along with the 
@param hwnd         : IN handle to the parent window
@returns void
*/
void DisplayErrorText(wyUInt32 dwlasterror, const wyChar *extratext, HWND hwnd = NULL);

/// Initializes the connection info structure
/**
@param con  : IN structure to initialize
@returns void
*/
void  InitializeConnectionInfo(ConnectionInfo &con);

/// Wrapper to set the window in ceter of the screen
/**
@param hwnd: IN window to set window pos
@return void
*/
void HandlerToSetWindowPos(HWND hwnd);

///setting Execute query submenus and object browserrefresh when switching F5 and F9 functionalities
/*
@param					: IN  Handle to the menu 
@param                  : IN  menu index
@returns void			
*/
void			   ChangeMenuItemOnPref(HMENU hmenu, wyInt32 menuindex);
/// setting menu item information
/**
@param					: IN  Handle to the menu 
@param					: IN  menu name
@param					: IN  menu item identifier
@returns void
*/
void			   SetMenuOnPreferenceChange(HMENU hmenu, wyWChar *menuname, wyInt32 menuid);


/// Initializes the combobox displayed in the create dialog with list of MySQL supported character sets
/**
@param hwnd		        : IN Parent window HANDLE
@returns void
*/
void                InitCharacterSetCombo(HWND hwnd);

/// click on Tree view item 
/**
@param  LPARAM   : IN Lparam value
@param  WPARAM   : IN wParam value
@@param wyInt32  : IN dlgitem id value to distinguewish two dlgs
@returns handle to the tree item on success else NULL
*/
HTREEITEM           HandleTreeViewItem(LPARAM lParam, WPARAM  wParam, wyBool spacekeydown = wyFalse);

///based on parent elements check un check its children elements
/**
@param HWND      : IN Treeview window handle
@param HTREEITEM : IN htreeitem of selected element in the tree
@returns void
*/
void                ClickOnFolderTreeViewItem(HWND treehandle, HTREEITEM htreeitem);
///Based on child items check un check root (if all childs are uncheck then remove check of parent
/// if any one child is check  then child its corresponding parent
/**
@returns wyBool.
*/
wyBool              CheckUnCheckTreeViewItems(HWND treehandle, HTREEITEM htreeitem);

///Get the Server Charset
/**
@param charset : OUT server charset
*/
wyBool				GetServerCharset(wyString &charset);

/// Gets the Selcted Character Set from the combo box and if a invalid charset is typed then to fetch it we are using GetWindowtext() iscopy parameter is used. 
/**
@param hwnd		        : IN Parent window HANDLE
@param selname          : IN pointer keeps the selected combo item
@param iscopy           : IN Mode of Fetching the text    
@returns void
*/
void				FetchSelectedCharset(HWND hwnd, wyString *selname ,wyBool iscopy);

/// return wytrue if  atleast one object is selected in the treeview else return wyfalse 
/**
@param HWND     :  IN tree handle
@returns wyBool.
*/
wyBool              IsObjectsSelected(HWND hwnd);

/// Gets the create table string(GUI)
/**
@param wnd			: IN parent connection window pointer.
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param db           : IN Database name
@param tbl          : IN Table name
@param strcreate    : OUT The CREATE statement
@returns wyTrue on success else wyFalse
*/
wyBool				GetGUICreateTableString(MDIWindow *wnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db, 
					 const wyChar* tbl, wyString &strcreate, wyString &query, wyBool isbacktick = wyTrue);


/// Gets the create View string
/**
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Mysql pointer
@param db           : IN Database name
@param tb          : IN Table name
@param strcreate    : OUT The CREATE statement
@returns wyTrue on success else wyFalse
*/
wyBool	GetCreateViewStatement(MDIWindow *wnd, Tunnel * tunnel, PMYSQL pmysql, const wyChar *db, 
							 const wyChar* tb, wyString *strcreate);

/// Gets the mySQL variable 'lower_case_table_names' value
/**
//http://dev.mysql.com/doc/refman/5.0/en/identifier-case-sensitivity.html
@param wnd : IN con window pointer
@return value assigned to 'lower_case_table_names'
*/
wyInt32						GetmySQLCaseVariable(MDIWindow *wnd);


wyBool	IsLowercaseFS(MDIWindow *wnd);
// getting the exact query length; 
// this is used to implement the single 
// query execution even if tyhe cursor is outside the query
wyInt32 GetQuerySpaceLen(int orglen, const wyChar *query);

///fills Content-Type combobox
/**
@param hwnd		:IN Combobox Handle
@returns void
*/
void FillContenttypeCombo(HWND hwndcombo);

//Menu Related functions 
//**********************************************************************

/// Gets the command name
/**
@param menustring       : IN/OUT Menu string
@param cmdstring        : IN/OUT Command string
*/
wyInt32						GetMenuItemName(wyWChar *menustring, wyWChar *cmdstring);

/// Gets the keyboard shortcuts - Menu
/**
@param menustring       : IN/OUT Menu string
@param shortcut         : IN/OUT Shortcut
@returns the shortcut length
*/
wyInt32                 GetKBShortcut(wyWChar *menustring, wyWChar *shortcut);

/// Change all the instance of CR in LF.
/**
@param text				: IN/OUT Text
@returns wyTrue			
*/
wyBool					ChangeCRToLF(wyChar * text);

//Handles On treeview item expand
/**
@param treeparam : IN trview node struct handle
@param isrefreshnode : sets to Wytrue for objectbrowser on 'Refresh' option , for aoviding flicker on 'Refresh'
@return wyTrue on success else return wyFalse
*/
wyBool		OnItemExpanding(TREEVIEWPARAMS &treeparam, wyBool isrefreshnode = wyFalse, wyBool iscalledfromusermanager = wyFalse);

///Inserting 'Folder' for tbles
/**
@param hwnd : IN tree-view handle
@param hDatabase : IN tree-view item handle
@param isrefresh : IN 'set wytrue for object browser for refresh option, to avoid flickering on Refresh
@return wyTrue on success else return wyFalse
*/
wyBool		InsertTableParent(HWND hwnd, HTREEITEM hDatabase, wyBool isrefresh = wyFalse);

/// Inserts dummy node in the tree.
/**
@param hparent : Parent tree.
*/
HTREEITEM   InsertDummyNode(HWND hwnd, HTREEITEM hparent);

/// Function to add all the views.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
*/
wyBool		InsertViews(HWND hwnd, HTREEITEM hDatabase);

/// Function to add all the events.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
*/
wyBool		InsertEvents(HWND hwnd, HTREEITEM hDatabase );


/// Function to add all the triggers.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
*/
wyBool		InsertTriggers(HWND hwnd, HTREEITEM hDatabase );

/// Function to add all the stored procs.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
*/
wyBool		InsertStoredProcs(HWND hwnd, HTREEITEM hDatabase );

/// Function to add all the functions.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
*/
wyBool		InsertFunctions(HWND hwnd, HTREEITEM hDatabase );

/// Function to add all the tables in the database.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/  
wyBool		InsertTables(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM hDatabase, wyWChar *dbname, wyBool checkboxstate, wyBool isrefresh);

/// Expands the table name.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandTableName(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyBool isrefresh, wyBool iscalledfromusermanager = wyFalse);

 /// Expands the procedure view.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandProcedures(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *dbname, wyBool checkstate, wyBool isrefresh);

/// Expands the function view.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandFunctions(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *dbname, wyBool checkstate, wyBool isrefresh);

/// Expands the views in the tables.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandViews(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *dbname, wyBool checkstate, wyBool isrefresh);

/// Expands the events.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandEvents(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *dbname, wyBool checkstate, wyBool isrefresh);

/// Expands the trigger view.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param dbname    : Database name
@param checkstate: Checkbox for node set 'tick' if its wyTrue else 'untick'
@param isrefresh : Flag sets wyTrue once the node(parent) is refreshing, this for avoiding flicker on refresh
*/
wyBool		ExpandTriggers(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *dbname, wyBool checkstate, wyBool isrefresh);

/// Deletes the child nodes.
/**
@param hparent    : Parent tree.
@param hparent	  : Parent node
@param isrefresh  : whetehr the node refreshed or not, this for avoiding flickering on refresh parent node
*/
VOID		DeleteChildNodes(HWND hwnd, HTREEITEM hparent, wyBool isrefresh = wyFalse);

/// Inserts an node in the given tree.
/**
@param hparent  : Parent tree.
@param caption  : Caption.
@param image    : Image index.
@param selimage : Selected image.
@param lparam   : LPARAM parameter.
@paraam ischekbox : WyTrue if node got check box else its wyFalse
@param checkstate : wyTrue for ticking the checkbox, wyFalse for unticking
*/
HTREEITEM   InsertNode(HWND hwnd, HTREEITEM hparent, const wyWChar *caption, wyInt32 image, wyInt32 selimage, LPARAM lparam, wyBool ischekbox = wyFalse, wyBool checkstate = wyTrue);

 /// Gets text in the given node.
/**
@param hitem    : Tree items.
@param buff     : Text.
@param buffsize : Size of buffer.   
*/
wyBool      GetNodeText(HWND hwnd, HTREEITEM hitem, wyWChar * buff, wyInt32 buffsize);

 /// Gets text in the given node.
/**
@param hdbitem  : Tree item.
@param type     : Type .  
*/
wyInt32		GetNodeText(HWND hwnd, wyWChar *buff, wyInt32 buffsize);

/// Inserts column information into the object browser with respect to the table whose 
/// handle is passed as parameter.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param table     : table name
*/
wyBool		InsertColumns(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *table, wyWChar *db = NULL, wyBool iscalledfromusermanager = wyFalse);

/// Function to insert index information about the table in the object browser.
/**
@param tunnel    : Tunnel pointer.
@param mysql     : Pointer to mysql pointer.
@param htable    : Tree items
@param table     : table name
*/
wyBool		InsertIndex(HWND hwnd, Tunnel *tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *table, wyWChar *db = NULL);

/// Returns the item image
/**
@param hitem    : Tree item.
*/
wyInt32		GetItemImage(HWND hwnd, HTREEITEM hitem);

///It executes query and sets the MY_RES pointer
/**
@param hwnd	   : IN handle to the window
@param wnd	   : IN MDIwindow pointer
@param tunnel  : IN tunnel pointer
@param mysql   : IN mySQL pointer
@param myres   : OUT MY_RES pointer
@param query   : IN query to be executed
@return mySQL result count if success else return -1
*/
wyInt32		GetObjectsMyRes(HWND hwnd, MDIWindow *wnd, Tunnel *tunnel, PMYSQL mysql, MYSQL_RES	*myres , wyString *query);

//For exporitng/copying single table from object browser
/**
@param hwnd		: IN dialog handle
@param hwndtree : IN treeview handle
@param table    : IN table name
@return wyTrue on success else return wyFalse
*/
wyBool		ProcessSingleTable(HWND hwnd, HWND hwndtree, wyWChar *table);

///Keeps the dialog cordinates in .ini for dialog persistance
/**
@param hwnd : IN handle to dialog
@param dlgprfifix : IN text specifying dialog name
@return void
*/
void		StoreDialogPersist(HWND hwnd, wyChar *dlgprfifix);

///Gets the values from .ini file and position the dialog
/**
@return void
*/
void			SetDialogPos(HWND hwnd, wyChar *dlgprfifix);

//Handles the flicer of Fk-dialogs
/**
@param hwnddlg  : IN handle to the dialog
@param hwndgrid : IN handle to the grid
@return void
*/
void			HandleIndexDlgsFlicker(HWND hwnddlg, HWND hwndgrid);

//Resize function for Show-> processlist, status, values, warning(csv import, Table->Abcvancs properties
/**
@param hwnd : IN Handle to dialog
@param hwndgrid : IN Handle to grid
@return void
*/
void		    ShowValueResize(HWND hwnd, HWND hwndgrid);

//Resize function for keyboard shortcut dialog
/**
@param hwnd : IN Handle to dialog
@return void
*/
void		    KeyShortCutResize(HWND hwnd);

/// Formats and add the resultset in the window.
/**
@param tunnel		: IN tunnel pointer
@param myres		: IN MY_RES pointer
@param hwndedit		: IN editor handle
@param isformatted	: IN formatted or not
@retunes wyTrue on success else wyFalse
*/
wyBool			FormatAndAddResultSet(Tunnel *tunnel, MYSQL_RES *myres, HWND hwndedit, MySQLDataEx *mdata, wyBool isformatted = wyFalse, wyBool istextview = wyFalse);


///get the value of child window is maximized or not from ini file
/*
@returns wyTrue if child window is maximized otherwise wyFalse
*/
wyBool GetChildMaximized();

///write child window is maximized or not to ini file
/**
@param hwnd			: IN child window handle
@retunes void
*/
void SetChildMaximized(HWND hwnd);

///Creates sqlyog icon for resizable dialogs
/**
@iconid : IN id of icon to be loaded
@return handle to HICON
*/
HICON CreateIcon(wyUInt32 iconid);

///Sets the scintilla modes(color, read only, wrap mode, etc)
/**
@param hwndeditor		: IN handle to Scintilla editor
@param fromini			: IN From ini file.
@param keywordstring	: IN Keywords to handle 
@param functionstring   : IN funcs words
@param isreadonly		: IN tells editor is read only or not
@return void
*/
void	SetScintillaModes(HWND hwndeditor, wyString &keywordstring, 
				  wyString &functionstring, wyBool fromini, wyBool ireadonly = wyFalse, wyChar* lexlanguage = "MySQL");


//Draws the 'Size Grip' at the bottom right of the resizble dialogs
/**
@param hdlg	: IN Handle to the dialog
@return void
*/
void	DrawSizeGripOnPaint(HWND hdlg);

/// To paint the Tables on Schema designer, once minimize and then maximize the Application
/**
@return void
*/
void		RepaintTabOnSize();

/// The callback function helps to apply changes  to all windows 
/**
@param hwnd			: IN Windows HANDLE
@param lparam		: IN Long message parameter
*/
static	wyInt32 CALLBACK	EnumChildProc(HWND hwnd, LPARAM lParam);

//Get time in HH:MM:SS:MSS format
/**
@param sec : IN milliseconds
@param timestring: OUT formatted string
@returns void
*/
void	GetTime(wyInt64 sec,wyString &timestring);

/// inserts tab 
/**
@param hwndcustomtab : IN custom tab
@param index		 : IN tab position
@param image		 : IN tab image
@param caption		 : IN tab text to display
@param lparam		 : IN long value to store
@returns wyBool, wyTrue if success, otherwise wyFalse.
*/
wyBool      InsertTab(HWND hwndcustomtab, wyInt32 index, wyInt32 image, wyString &caption, LPARAM lparam);

///Gets the Font setting in Prefernce->Editor->Other controls
/**
@param hwnd : IN handle to window for getting the device context
@returns the font selected if successful, else return NULL
*/
HFONT		GetOtherControlsFont(HWND hwnd);

///Gets keywords and Functions for Scintilla editor
/**
@param keyword	: wyTrue if keywords else wyFalse
@param buffer	: Stores the keywords
@returns wyTrue on success else wyFalse
*/
wyBool		GetAllScintillaKeyWordsAndFunctions(wyBool bkeyword, wyString &buffer);

///Is the selected query support formatting or not
/**
@param hwnd	: IN handle to the editor
@returns wyTrue on success else wyFalse
*/
wyBool		IsQuerySupportFormatting(HWND hwnd);

//Check whether query is SELECT query or not
/**
@param query : query to check
@return wyTrue on success else return wyFalse
*/
wyBool		IsQuerySELECT(const wyChar* query);



wyBool      IsQueryDeleteInsertReplaceUpdate(const char *query);

//Identify whether LIMIT present in Outer query or not
/**
@return wyTrue if LIMIT present in outer query else return wyFalse
*/
wyBool		IstoAddLimitClausewithSelect(const wyChar* query);

//Check SELECT options for those LIMIT clause is not valid
/**
@param ptr  : IN Formatter class pointer passed as class pointer
@selectstmt : IN select statement to look the pattern against
@return wyFalse if pattern mattched so LIMIT no need to append, else return wyTrue
*/
wyBool		CheckforOtherSelectOptions(VOID *ptr, wyString *selectstmt); 

//Cheking whether Create Table schema has got AUTO_INCREMENT option or not
/**
@param createtableorg : IN/OUT Create table schema, trim the AUTO_INCREMENT and set
@return wyTrue if AUTO_INCREMENT option is present
*/
wyBool		CheckAutoIncrementTableOption(wyString *createtableorg);

///Format the 'SELECT' query in 'Create View' statement
/**
@param query		: IN Create View statement
@param objectname	: IN view name
@param ishtml		: IN flag sets wyTrue for HTML type format, wyFalse for Text type format
@return VOID
*/
VOID			FormatCreateViewStatement(wyString *query, const wyChar *objectname, wyBool ishtml);

wyBool			FormatCreateTablestatement(wyString *schema);

wyBool			FormatQueryByRemovingSpaceAndNewline(wyString *schema);

void            CopyWithoutWhiteSpaces(HWND hwnd);

//Checking for comment
/**
@param hwndeditor : IN editor handle
@param isfunc : IN whether its function or not
@return wyTrue on success else return wyfalse
*/
wyBool		IsComment(HWND hwndeditor, wyBool isfunc = wyFalse);

///Setting the Static Text Font
/**
@param hwnd	: IN handle to the Static Text Window
@returns the font selected if successful, else return NULL
*/
HFONT		GetStaticTextFont(HWND hwnd);

///Saving the column name and column width of the grid, while resizing a column, . 
/**
@param hwndgrid	: IN handle to the Grid
@param dbname	: IN Database name
@param tblname	: IN Table name
@param colname  : IN Column name
@param column	: IN Column number 
@returns void
*/
void		SaveColumnWidth(HWND hwndgrid, wyString *dbname, wyString *tblname, wyString *colname, wyInt32 column);

///Writing the column attribute (database name, table name, column name and column width)to SQLite database. 
/**
@param hdb      : IN SQLite db handle
@param hwndgrid	: IN handle to the Grid
@param dbname	: IN Database name
@param tblname	: IN Table name
@param colname  : IN Column name
@param column   : IN Column number
@returns void
*/
void		WriteColumnWidthToFile(sqlite3 *hdb, HWND hwndgrid, wyString *dbname, wyString *tblname, wyString *colname, wyInt32 column);

///Getting the column width from the SQLite database. 
/**
@param dbname				: IN Database name
@param table				: IN Table name
@param colname				: IN column name
@returns width of the column
*/
wyInt32		GetColumnWidthFromFile(wyString *dbname, wyString *table,wyString *colname);

//Data tab LIMIT values persistance
/**
@param dbname : IN Database name
@param table  : IN Table name
@param startrows : IN/OUT low limit value
@param numrows   : IN/OUT high limit value
@param insertdata : wyTrue for inserting to SQLite table, wyFalse for retriving for SQLite table
@return VOID
*/
wyBool		HandleDatatabLimitPersistance(const wyChar *dbname, const wyChar *table, wyInt64 *startrows, wyInt32 *numrows, wyBool insertdata = wyFalse);

wyInt32		HandleTableViewPersistance(const wyChar *dbname, const wyChar *table, wyInt32 viewtype = 0, wyBool insertdata = wyFalse);

//Handle the Result tab display option persistance
/**
@param querycrc : IN CRC of query
@param highlimit : IN value to store, if NULL accecc the value for SQLite table
@return 0 if writing to table, else return the 'highlimit' value that stored for that query
*/
wyInt32    HandleResulttabPagingQueryPersistance(wyUInt32 querycrc, wyInt32 *highlimit = NULL);

/// Used for sqlite to get the column width that query returns. 
/**
@param count        : IN Column width
@param argc         : IN Argument count
@param argv         : IN Argument vector
@param colname      : IN Number of rows    
@returns total number of rows
*/
wyInt32		callback_width(void *count, wyInt32 argc, wyChar **argv, wyChar **azColName);

/// 64-bits ineger that holds the 'start' rows in the LIMIT option with Datatab
/**
@param count        : IN Column width
@param argc         : IN Argument count
@param argv         : IN Argument vector
@param colname      : IN Number of rows    
@returns total number of rows

*/
wyInt32		callback_strtrows64(void *count, wyInt32 argc, wyChar **argv, wyChar **azColName);

/// Function to implement the find and replace menu option
	/**
	@param pcquerywnd	: IN/OUT window HANDLE
	@param isreplace	: IN Checks for replace option
	@returns wyTrue on success else wyFalse
	*/
wyBool		FindTextOrReplace(MDIWindow * pcquerywnd, wyBool uIsReplace);

//get styled text
#if defined _WIN32 && ! defined _CONSOLE
VOID 	   GetStyledText(HWND hwndeditor, wyChar *text, TextRange *range, 
		                 wyInt32 kwcase, wyInt32 funcase);
#endif

//copy the scintilla styled text (with cases) to clip board
VOID       CopyStyledTextToClipBoard(HWND hwnd);

wyBool	   HandleScintillaStyledCopy(HWND hwnd, WPARAM wparam);

///Handles the grid Splitter Move
/**
@param hwndgrid		: IN Grid Handle
@param tbl name		: IN Column name
@param wparam		: IN Column number
@returns wyBool
*/
wyBool	OnGridSplitterMove(HWND hwndgrid, wyString *tblname, WPARAM wparam);

///* formate the query*/
void	    GetFormatQuery(wyString *query, wyBool ishtmlform);

//Check whether the query is SELECT query or not
/**
@param query : IN query to be executed
Return wyTre if the query to be executed is SELECT query
*/
wyBool		IsQuerySELECT(const wyChar* query);

/// Creates the file ina application data folder
/**
@returns wyTrue on success else wyFalse
*/
wyBool      CreateInitFileInApplData();

/// This function creates the sqlyog.ini file in the current directory, if it not exists
/** 
@returns wyTrue if SUCCESS, otherwise wyFalse
*/
wyBool		CreateInitFile();

///Gets the 'Config path' and 'file name to be opend' from command line
/**
@param cmdline : IN Command line string
@return wyTrue on success else return wyFalse
*/
wyBool		GetConfigDetails(wyChar *cmdline);

//Handle the wait_timeout option in con.dialog
/**
@param hwnd		: IN Handle to dialog
@param coninfo	: IN ConnectionInfo struct pointer
@param id		: IN control id to be unchecked
@return VOID
*/
wyBool		HandleTimeoutOption(HWND hwnd, ConnectionInfo *coninfo, wyUInt32 idcheck, wyUInt32 iduncheck);


/// Shows the error message
/**
@param hdlg				: IN Dialog window handle
@param query			: IN query
@param errmsg			: IN err msg
@returns void
*/
void		ShowErrorMessage(HWND hdlg, const wyChar * query, const wyChar *errmsg);

///Formats the Field coulmn(statement) in HTML format by handling tab, space, newline, etc
/**
@param query		: IN statemnt to handle
@param ismorewidth	: OUT 
@return void
*/
VOID			FormatHtmlModeSQLStatement(wyString *query, wyBool	&ismorewidth);

//For breaking the long string for HTML mode
/**
@param htmlstr : IN string to be handled
@return VOID
*/
VOID				FormatHtmlLongString(wyString *htmlstr);

/// function initializes the choose color structure and opens up the choose color dialog
/**
@param hwnd		  : IN Dialog window handle
@param rgbcolor   : OUT Color rgbcolor
@return wyTrue on success else return wyFalse
*/
wyBool		ChColor(HWND hwnd, LPCOLORREF rgbcolor);

/// Function fills up the COMBOBOX with colors and custom color option
/*
@param hwnd		  : IN Dialog window handle
@param lpds		  : IN drawitem struct
@param rgbcolor   : IN Color rgbcolor
@return wyTrue on success else return wyFalse
*/
wyBool	OnDrawColorCombo(HWND hwnd, LPDRAWITEMSTRUCT lpds, COLORREF rgbcolor);


/// Function fills up the COMBOBOX with foreground colors and custom color option
/*
@param hwnd		  : IN Dialog window handle
@param lpds		  : IN drawitem struct
@param rgbcolor   : IN Color rgbcolor
@return wyTrue on success else return wyFalse
*/
wyBool	OnDrawColorCombo(HWND hwnd, LPDRAWITEMSTRUCT lpds, COLORREF rgbcolor);

/// Command handler for color combo box
/**
@param hwnd			: IN Window HANDLE
@param wparam		: IN Unsigned message parameter
@param lparam		: IN Long message parameter
*/
void OnWmCommandColorCombo(HWND hwnd, WPARAM wparam, LPARAM lparam);


/// Function fills up the COMBOBOX with colors and custom color option
/*
@param hwndCombo    : IN combo box  HANDLE
*/
wyBool ColorComboInitValues(HWND hwndCombo);

/// Function fills up the COMBOBOX with colors and custom color option
/*
@param hwndCombo    : IN combo box  HANDLE
*/
wyBool ColorComboFgInitValues(HWND hwndCombo, wyBool isBk = wyFalse, COLORREF bkcolor = -1);

///HAndle the query 'display options' persistance of query
/**
@param query : IN Query
@param highlimit : NULL for acceeing value, if Not NULL then store this value to SQLite table
@return 0 if write operation, else return the 'highlimit' value during read operation
*/
wyInt32			HandleQueryPersistance(wyChar *query, wyInt32 *highlimit = NULL);

///sets up font
/**
@param hwnd					: IN Handle to the window for setting the font
@param fontheight           : IN height of the font
@returns void
*/
VOID	                    WindowFontSet(HWND hwnd, wyInt32 fontheight = 8);

///Sets the dialog cordinates according to dialog persistance save in ini
/**
@param hwnd			: IN handle to dialog
@param dlgprfifix	: IN text specifying dialog name
@return wyTrue on success else return wyFalse
*/
wyBool                      SetInitPos(HWND hwnd, wyChar *dlgprfifix);

///Keeps the dialog cordinates in .ini for dialog persistance
/**
@param hwnd			: IN handle to dialog
@param dlgprfifix	: IN text specifying dialog name
@return wyTrue on success else return wyFalse
*/
wyBool                      SaveInitPos(HWND hwnd, wyChar *dlgprfifix);

///Checks the given window whether it is blank or not
/**
@param hwnd                 : IN handle to the window
@returns -1 if field contain only white spaces, 0 if the field contain nothing, 1 otherwise 
*/
wyInt32                     IsFieldBlank(HWND hwnd);

//Detecting the Extension of file to be opening
/**
@param filename : IN File name
@returns filetype
*/
SQLyogFileType				OpenFileType(wyString *filename);

//Gets the engine of the table
/**
@param wnd: IN handle to MDIwindow
@param dbname : IN context db
@param tablename : IN selected table in obj.browser
@param engine    : OUT engine of table
*/
void						GetsTableEngineName(MDIWindow *wnd, const wyChar *dbname, const wyChar *tablename, wyString *engine);		

//Function perfoms FindNext on F3
/**
@param wnd                  : IN mdi window handle
@returns void
*/
void                        OnAccelFindNext(MDIWindow* wnd);

//Handle drag & drop file to editor, sd & qb
/**
@param phandle  : IN HDROP handle
@param wnd		: IN MDIWindow handle
@return void
*/
void						HandleOnDragAndDropFiles(HDROP phandle, MDIWindow *wnd);

void						SeekCurrentRowFromMySQLResult(MYSQL_RES *res, MYSQL_ROWS **rowswalker, Tunnel *tunnel, MYSQL_ROW *currentrow, wyULong **rowlength);


//Sets connection name with connection color in combobox
/*
@param hwnd		  : IN Dialog window handle
@param lpds		  : IN drawitem struct
@return wyTrue on success else return wyFalse
*/
wyBool						OnDrawConnNameCombo(HWND hwnd, LPDRAWITEMSTRUCT lpds,  wyBool isconndbname = wyFalse);

//Sets item in conection name combobox
/*
@param hwnd		  : IN Dialog window handle
@param lpds		  : IN drawitem struct
@param cr	      : IN color for connection 
@return wyTrue on success else return wyFalse
*/
wyBool						OnDrawComboItem(HWND hwndcombo, LPDRAWITEMSTRUCT lpds, COLORREF cr, wyBool isconndbname = wyFalse);

/// function to get measurestruct of custom draw combobox
/**
@param lpmis	: IN info about how to paint an owner-drawn combobox
@returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool						OnComboMeasureItem(LPMEASUREITEMSTRUCT lpmis);

///Function handles scintilla notification messages like paranthesis matching
/**
@param wparam               : IN notification parameter
@param lparam               : IN notification parameter
@param isquerytab           : IN flag tells whether the caller is Query Tab or not
@returns 1 if the message is handled else 0
*/
wyInt32                     OnScintillaNotification(WPARAM wparam, LPARAM lparam, wyBool isquerytab);

///Function sets the styles for parantesis highlighting
/**
@param hwnd                 : IN handle to the Scintilla window
@returns void
*/
void                        SetParanthesisHighlighting(HWND hwnd);

///Function enables folding in Query Editor
/**
@param hwnd                 : IN handle to the Scintilla window
@returns void
*/
void                        EnableFolding(HWND hwnd);

///Function positions the given window in the center of the monitor where the window given in first parameter resides
/**
@param handletoget          : IN window handle to the window based on which the monitor to be identified
@param handletoset          : IN window to be positioned center to the monitor
*/
void                        PositionWindowCenterToMonitor(HWND handletoget, HWND handletoset);

//Covert the string to Base2 format , to handle the BIT s=datatype handling
/**
@param data : IN Data to be converted
@param colwidth : IN Width of the BIT type column
@param str : OUT Buffer holds the Base2 format data
@return VOID
*/
void						ConvertStringToBinary(wyChar *data, wyUInt32 len, wyInt32 colwidth, wyString *str);

//void                       PopulateColorArrayConnDb(wyInt32 * arrayofdbcolor, wyInt32 numdb, COLORREF rgbconn);

wyBool						OnDrawConnDbNameCombo(HWND hwndcombo, LPDRAWITEMSTRUCT lpds);

wyBool						OnDrawComboDbItem(HWND hwndcombo, LPDRAWITEMSTRUCT lpds);

wyBool                      SetWindowPositionFromINI(HWND hwnd, wyChar* section, wyInt32 minwidth, wyInt32 minheight);

void HandleListViewCheckboxItems(HWND hListView, LPARAM lParam, wyBool flagListItemsLoaded);

//...Handles the OnMouseOverChangeIcon event
LRESULT CALLBACK StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

wyInt32	SetAsLink(HDC hdc);

//...Sets the File path to the Current Directory

void SetFilePathToCurrentDirectory(wyString *dir);

//...Opens the LOG file

void OpenLogFile(wyString *str);

wyInt32 IsAnyListItemChecked(HWND hListView);

void FreeMenuOwnerDrawItem(HMENU hmenu, wyInt32 pos = -1);

wyInt64 GetHighPrecisionTickCount();

int GetDatabasesFromOB(wyString*** dblist);

wyInt32	GetTextLenOfStaticWindow(HWND hwnd);

wyInt32	GetStaticTextHeight(HWND hwnd);

RECT GetTextSize(wyWChar* text, HWND hwnd, HFONT hfont);

void    CreateCustomTabIconList();

//Sets the Scintilla control color to that of Message/Table Data/Info Tab style
void SetMTIColor(HWND hwnd);

wyBool GetTabOpenPersistence(wyInt32 tabimage, wyBool isset = wyFalse, wyBool valuetoset = wyFalse);

void EnableDisableExportMenuItem();

LPARAM  TV_GetLParam(HWND hwnd, HTREEITEM hti);

void EscapeHtmlToBuffer(wyString* buffer, const wyChar* text);

wyBool CreateSessionFile();

wyBool GetSessionFile(wyWChar *path);

wyBool GetSessionDetails(wyWChar* conn, wyWChar* path, ConnectionInfo *conninfo, wyIni *inimgr);

wyBool GetSessionDetailsFromTable(wyWChar* path, ConnectionInfo *conninfo, wyInt32 id, MDIlist* tempmdilist);
wyBool GetHistoryDetailsFromTable(wyWChar* path, wyInt32 id, wyString* historydata);
wyBool GetOBDetailsFromTable(wyWChar* path, wyInt32 id, wyString* obdb);
wyBool GetSessionfileDetailsFromTable(wyWChar* path, wyInt32 *isedited, wyString* obdb);
wyBool GetTabDetailsFromTable(wyWChar* path, wyInt32 id, List* temptablist);
//wyBool WriteSessionDetails(wyChar* title, ConnectionInfo *conninfo, wyInt32 connectionno, wyBool isfocus);

void WriteFullSectionToFile(FILE *fstream, wyInt32 conno, ConnectionInfo *coninfo, const wyChar *title, wyBool isfocussed);
//Returns wyTrue on success
wyBool WriteFullSectionToTable(wyString *sqlitequery, wyInt32 id, wyInt32 position, ConnectionInfo *coninfo, const wyChar *title, wyBool isfocussed, wySQLite	*ssnsqliteobj = NULL);
//wyBool WriteTabDetailsToTable(tabeditorelem *temptabeditorele, CTCITEM quetabitem,wyInt32 tabid, wyInt32 position, TabEditor *tabqueryactive, MDIWindow *wnd);
#endif


