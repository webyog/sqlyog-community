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


#ifndef _FILE_INCLUSION_
#define _FILE_INCLUSION_

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <crtdbg.h>
#include <Richedit.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include <unknwn.h>
#include <mstask.h>
#include <MLang.h>

#ifndef VC6

#include <gdiplus.h>
#include <GdiPlusGraphics.h> 
#include <olectl.h>

#endif

#include "Include.h"
#include <mysql.h>
#include "EditorQuery.h"
#include "ObjectBrowser.h"
#include "StatusbarMgmt.h"
#include "FrameWindowSplitter.h"
#include "TabMessage.h"
#include "CustGrid.h"
#include "Tunnel.h"
#include "ConnectionBase.h"
#include "PreferenceBase.h"
#include "Verify.h"
#include "AppInfo.h"
#include "CustomSaveDlg.h"

#if _MSC_VER > 1000

#pragma warning(disable: 4706)
#pragma warning(disable: 4127)
#pragma warning(disable: 4245)
#pragma warning(disable: 4100)
#pragma once
#endif

#define		IS_FROM_WINDOWS_STORE	0

// #defines of character constants for better readability using in lexer and editing things.
#define		C_TAB										'\t'
#define		C_SPACE										' '
#define		C_NULL										'\0'
#define		C_NEWLINE									'\n'
#define		C_CARRIAGE									'\r'
#define		C_OPEN_BRACKET								'('
#define		C_CLOSE_BRACKET								')'
#define		C_COMMA										','
#define		C_SEMICOLON									';'
#define		C_PLUS										'+'
#define		C_GREATER_THEN								'>'
#define		C_LESS_THEN									'<'
#define		C_EQUAL										'='
#define		C_EXCLAMATION								'!'
#define		C_AND										'&'
#define		C_OR										'|'
#define		C_STAR										'*'
#define		C_FRONT_SLASH								'/'
#define		C_BACK_SLASH								'\\'
#define		C_DASH										'-'
#define		C_HASH										'#'
#define		C_SINGLE_QUOTE								'\''
#define		C_DOUBLE_QUOTE								'\"'
#define		C_TILDE										'`'

// User defined messages used in various procedures.

#define		WM_INITCONNDIALOG							WM_USER+50
#define		WM_INITGRIDVALUES							WM_USER+51
#define		WM_CLOSEALLWINDOW							WM_USER+61
#define		WM_INITDLGVALUES							WM_USER+62
#define		WM_INITIMPORTDATA							WM_USER+63
#define		WM_INITEXPORTDATA							WM_USER+64
#define		UM_HILITE									WM_USER+65
#define		UM_CONTENT_CHANGE							WM_USER+66
#define		UM_PARSE_BUFFER								WM_USER+67
#define		UM_MOVEBUTTON								WM_USER+68
#define		UM_CREATEGRID								WM_USER+69
#define		UM_GETTABLETYPE								WM_USER+70
#define		UM_MOVELEFT									WM_USER+71
#define		UM_MOVERIGHT								WM_USER+72
#define		UM_CLOSE									WM_USER+73
#define		UM_REEDITLABEL								WM_USER+74
#define		UM_FOCUS									WM_USER+75
#define		UM_SETWINDOWTITLE							WM_USER+76
#define		UM_MOVEWINDOWS								WM_USER+77
#define		UM_INITUSERDATA								WM_USER+78
#define		UM_SELENDOK									WM_USER+79
#define		UM_FILLCOMBO								WM_USER+80
#define		UM_INITGRIDVALUES							WM_USER+81
#define		UM_INITTABLEGRIDVALUES						WM_USER+82
#define		UM_INITCOLGRIDVALUES						WM_USER+83
#define		UM_GETINITUSERDATA							WM_USER+84
#define		UM_AFTEROK									WM_USER+85
#define		UM_SETFLGDIRTY								WM_USER+86
#define		UM_CALCRICHEDIT								WM_USER+87
#define		UM_COMBOCHANGE								WM_USER+88
#define		UM_DISABLEWINDOW							WM_USER+89
#define		UM_ENABLEWINDOW								WM_USER+90
#define		UM_EXPANDTREENODE							WM_USER+91
#define		UM_SETCHECKSTATE							WM_USER+92
#define		UM_CHECKUNCHECKALL							WM_USER+93
#define		UM_SCROLL									WM_USER+94
#define		UM_SELECTITEM								WM_USER+95
#define		UM_CREATETABLEQUERY							WM_USER+96
#define		UM_CHANGETABLE								WM_USER+97
#define		UM_SYNCJOB									WM_USER+98
#define		UM_SYNCDBSTRUCTURE							WM_USER+99
#define		UM_CREATESCHEMA								WM_USER+100
#define		UM_COPYDATABASE								WM_USER+101
#define		UM_BACKUPTABLE								WM_USER+102
#define		UM_RESTORETABLE								WM_USER+103
#define		UM_REFRESHOBJECT							WM_USER+104
#define		UM_SETDIALOGTITLE							WM_USER+105
#define		UM_SELECTTABLE								WM_USER+106
#define		UM_SETTABLEDATAFOCUS						WM_USER+107
#define		UM_RESULTTABFOCUS							WM_USER+108
#define		UM_MESSAGETABFOCUS							WM_USER+109
#define		UM_INFOTABFOCUS								WM_USER+110
#define		UM_HISTORYTABFOCUS							WM_USER+111
#define		UM_TAGFILE_UPDATE_START						WM_USER+112
#define		UM_TAGFILE_UPDATE_END						WM_USER+113
#define		UM_TAGFILE_UPDATE_CLOSE						WM_USER+114
#define		SHOW_MYSQL_ERROR							WM_USER+115
#define		SHOW_SSH_ERROR								WM_USER+116
#define		SHOW_CONFIRM								WM_USER+117
#define		UM_INITDLGVALUES							WM_USER+118
#define		UM_PROCGRIDVALUE							WM_USER+119
#define		MAX_WINDOW                                  WM_USER+120
#define		UM_CHANGETEXT								WM_USER+121
#define     UM_SPLITTERRESIZED							WM_USER+122
#define		UM_DESTROY_CONNTAB							WM_USER+123 
#define		UM_UPDATEMAINTOOLBAR    					WM_USER+124
#define		UM_SHOWERROR								WM_USER+125
#define		UM_CONNECTFROMLIST							WM_USER+126
#define		UM_QUERYCOMPLETED							WM_USER+127
#define		UM_MDIGETACTIVE								WM_USER+128
#define		UM_GETEDITORTEXT							WM_USER+129
#define		UM_QBITEMFROM								WM_USER+130
#define		UM_QBITEMTO									WM_USER+131
#define		UM_CHECKSTATECHANGE							WM_USER+132


// some column #defines for validation.

#define		ODBC_CNAME		0
#define		ODBC_DATATYPE	2
#define		ODBC_LENGTH		3
#define		ODBC_DEFAULT	4
#define		ODBC_PRIMARY	5   
#define		ODBC_BINARY		6
#define		ODBC_NOTNULL	7
#define		ODBC_UNSIGNED	8
#define		ODBC_AUTOINCR	9
#define		ODBC_ZEROFILL	10

#define		SMALLBUF	20
#define		MEDIUMBUF	30

#define		VSPLITTER_WINDOW_CLASS_NAME_STR				L"VSplitter"
#define		HSPLITTER_WINDOW_CLASS_NAME_STR				L"HSplitter"	
#define		INSERT_UPDATE_WINDOW_CLASS_NAME_STR			L"InsertUpdate"	
#define		QUERY_WINDOW_CLASS_NAME_STR					L"QueryWindow"
#define		DATA_WINDOW									L"DataDisplayWindow"
#define		SD_CANVAS_WINDOW							L"SDCanvasWindow"
#define		QB_CANVAS_WINDOW							L"QBCanvasWindow"
#define     PQA_RESULT_WINDOW							L"PQAHtmlWindow"
#define     INFO_HTML_WINDOW							L"InfoHtmlWindow"
#define		DBSEARCH_HTML_WINDOW						L"Db search Html window"
#define     FORMVIEW_HTML_WINDOW						L"HtmlFormViewWindow"
#define     HTTP_ERROR_WINDOW							L"PQAHtmlWindow"
#define		MAIN_WINDOW_CLASS_NAME_STR					L"MainWindow"
#define     TABLE_TABINTERFACE_WINDOW                   L"TableTabInterface"
#define     TTI_BOTTOMFRAME_WNDCLS                      L"TableTabInterfaceBottomFrame"
#define     TTI_SUBTABS_COMMONWNDCLS                    L"TabInterfaceCommonWindow"
#define     TTI_FIELDS_TAB_WNDCLS                       L"TabInterfaceFieldsTabWindow"
#define     TTI_ADVPROP_TAB_WNDCLS                      L"TabInterfaceAdvPropTabWindow"
#define     TTI_INDEXES_TAB_WNDCLS                      L"TabInterfaceIndexesTabWindow"
#define     TTI_FK_TAB_WNDCLS                           L"TabInterfaceFKTabWindow"
#define     TTI_PREVIEW_TAB_WNDCLS                      L"TabInterfacePreviewTabWindow"
#define		ANNOUNCEMENTS_WINDOW						L"AnnouncementsWindow"
#define		ANNOUNCEMENTS_MAIN_WINDOW					L"AnnouncementsMainWindow"

#define		CONNECTION_FILE								"connection.ini"
#define		KEYSHORT_FILE								"keyshort.txt"
#define		VERSIONHIST_FILE							"version.txt"
#define		SQLYOGINI_FILE								"\\sqlyog.ini"
#define		INI_FILE									"sqlyog.ini"
#define		SECTION_NAME								"SQLYOG"
#define		VSPLITTER_SECTION_NAME						"VSplitter"
#define		HSPLITTER_SECTION_NAME						"HSplitter"
#define		SCHEMASYNC_SECTION							"SchemaSync"
#define		TABLEMAKER_SECTION							"TableMaker"
#define		MANRELATION_SECTION							"ManageRelationship"
#define		HANDLERELATION_SECTION						"HandleRelationship"
#define		INDEXMANAGER_SECTION						"ManageIndex"
#define		HANDLEINDEX_SECTION							"HandleIndex"
#define		BLOBVIEWER_SECTION							"BlobViewer"
#define		USERMANAGER_SECTION							"UserManager"
#define		SHOWVALUE_WARNING_SECTION					"ShowWarnings"
#define		SHOWVALUE_PROCESSLIST_SECTION				"ShowProcessList"
#define		SHOWVALUE_STATUS_SECTION					"ShowStatus"
#define		SHOWVALUE_VARIABLE_SECTION					"ShowVariable"
#define     SHOWSUMMARY_SECTION							"ShowSummary"
#define		SHOWPREVIEW_SECTION							"ShowPreview"
#define		KEYSHORTCUT_SECTION                         "KeyBoardShort"
#define		COLUMNMAP_SECTION							"ColumnMap"
#define     USERMANAGERINTERFACE_SECTION                "UserManagerInterface"
#define     EXPORTCONNECTIONDETAILS_SECTION             "ExportInterface"
#define     IMPORTCONNECTIONDETAILS_SECTION             "ImportInterface"
#define		TABLEDIAGNOSTICS_SECTION					"TableDiagnostics"
#define		COPYDATABASE_SECTION						"CopyDatabaseSec"
#define		SQLDUMP_SECTION								"SQLDump"
#define		EXPORTAS_SECTION							"ExportAs"
#define		COLUMNREORDER_SECTION						"ColumnReorder"
#define		ADDFAV_SECTION								"AddFavoritesSec"
#define		ORGFAV_SECTION								"OrganizeFavourites"
#define		JOBMAN_SECTION								"JobmanagerSection"
#define		COPYTABLE_SECTION							"CCopyTable"
#define		SQLTEMPLATE_SECTION							"SQLTemplate"
#define		STRING_NULL									"(NULL)"
#define		STRING_BLOB									"BLOB..."
#define		STRING_JSON									"JSON..."
#define		BINARY_DATA									"(Binary/Image)"
#define		STR_DEFAULT									" [default]"
#define		SHIFTED										0x8000
#define		MAX_SIZE									4*1024*1024
#define		MAX_CONN									999
#define		NODBSELECTED								_(L"No database selected")
#define		DATABASEFILTER_SECTION						"DatabaseFilter"
#define		TABLEFILTER_SECTION							"TableFilter"
#define		COLUMNFILTER_SECTION						"ColumnFilter"
#define		DATATYPEFILTER_SECTION						"DatatypeFilter"
#define		CONNECTIONLIST_SECTION						"ConnectionList"

#define		NOT_ENOUGH_MEMORY							_("Not enough memory, application terminated! ")

#define		STR_DEF_SSHPORT								"3310"
#define		STR_DEF_HIGHLIMIT							"25"
#define		HIGHLIMIT_DEFAULT							1000
#define		STR_HIGHLIMIT_DEFAULT						"1000"
#define		INT_DEF_HIGHLIMIT							50
#define		NSERVER										0
#define		NDATABASE									1
#define		NFOLDER										2
#define		NTABLE										3
#define     NEXPFOLDER                                 0
#define     NEXPTABLE                                  1
#define     NEXPVIEW                                   2
#define     NEXPFUN                                    3
#define     NEXPPROCED                                 4
#define     NEXPTRIGGER                                5
#define     NEXPEVENT                                  6
#define		NCOLUMN										4
#define		NSYNCTRIGGER                                6
#define     NSYNCEVENT									12
#define		NSYNCFKEY									11
#define		NSYNCFUNCTION								10
#define		NSYNCSTOREDPROC								9
#define		NSYNCFOLDER									7
#define		NSYNCVIEW									8
#define		NINDEX										5
#define		NSP											6
#define		NSPITEM										7                                 
#define		NFUNC										8
#define		NFUNCITEM									9
#define		NVIEWS										10
#define		NVIEWSITEM									11
#define		NTRIGGER									12
#define		NTRIGGERITEM								13
#define		NSTATUS										6
#define		NPROCESS									7	
#define		NVARIABLES									8
#define		NADDUSER									9
#define		NEDITUSER									10
#define		NMANAGEUSER									11
#define		NODBCIMPORT									12	
#define		NTABLES										14
#define     NEVENTS                                     16
#define     NEVENTITEM                                  15
#define     NPRIMARYKEY									17
#define		NPRIMARYINDEX								18

#define		WIDTHBETBUTTON								10
#define		RIBBONHIGHT									25
#define		CELLBUTTONMINWIDTH							80

//for appending the list of child window names to window menu
#define		MDISUBMENU									9

// #defines for telling the advproperties what to show.
#define		ADVPROP										1
#define		STATUS										2
#define		VARIABLES									3
#define		PROCESSLIST									4
#define		WARNINGS									5

#define		FILTEREQUAL									1
#define		FILTERNOTEQUAL								2
#define		FILTERMORE									3
#define		FILTERLESS									4

#define		SQLINDEX									0
#define		CSVINDEX									1
#define		HTMINDEX									2
#define		XMLINDEX									3
#define		LOGINDEX									4
#define		SCHEMAXMLINDEX								5
#define		BMPINDEX									6
#define     TEXTINDEX									7
#define     ACCESSINDEX									8
#define     EXCELINDEX									9
#define     QUERYXMLINDEX                               10
#define		ZIPINDEX									11
#define		SQLYOGFILEINDEX								12
#define     PPKINDEX                                    13
#define     CONMANINDEX                                 14
#define     SESSIONINDEX                                15

//#define		SQL										L"SQL Files(*.sql)\0*.sql;*.txt\0All Files(*.*)\0*.*\0"

//compress backup
#define		ZIP											L"ZIP Files(*.zip)\0*.zip;All Files(*.*)\0"	
//#define		SQL											L"SQL Files(*.sql)\0*.sql\0All Files(*.*)\0*.*\0"
#define		CSV											L"CSV Files(*.csv)\0*.csv\0All Files(*.*)\0*.*\0"
#define     JSON                                        L"JSON Files(*.json)\0*.json\0All Files(*.*)\0*.*\0"
#define		HTML										L"HTML Files(*.htm;*.html)\0*.htm\0All Files(*.*)\0*.*\0"
#define		XML											L"XML Files(*.xml)\0*.xml\0All Files(*.*)\0*.*\0"
#define		LOG											L"LOG Files(*.log)\0*.log\0All Files(*.*)\0*.*\0"
#define		EXCEL										L"XML Files(*.xml)\0*.xml\0All Files(*.*)\0*.*\0"
//#define		SCHEMAXML									L"XML Files(*.schemaxml)\0*.schemaxml\0All Files(*.*)\0*.*\0"
#define     TEXTFILE									L"TEXT Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0"
#define		BMPFILE										L"Bitmap(*.bmp)\0*.bmp\0All Files(*.*)\0*.*\0"
#define		ACCESSFILE									L"Access Files(*.mdb, *.accdb)\0*.mdb;*.accdb\0All Files(*.*)\0*.*\0"
#define		EXCELFILE									L"Excel Files(*.xls, *.xlsx, *.xlsm, *.xlsb)\0*.xls;*.xlsx;*.xlsm;*.xlsb\0All Files(*.*)\0*.*\0"
//#define     QUERYXML                                    L"XML Files(*.queryxml)\0*.queryxml\0All Files(*.*)\0*.*\0"
//#define     SQLYOG_FILES								L"SQLyog Files(*.sql; *.queryxml; *.schemaxml)\0*.sql;*.queryxml;*.schemaxml\0All Files(*.*)\0*.*\0"  
#define		PPKFILE									    L"PuTTY Private Key Files(*.ppk)\0*.ppk\0All Files(*.*)\0*.*\0"
#define     CONMANFILE                                  L"SQLyog Connection Settings(*.sycs)\0*.sycs;"
#define		SESSIONFILE									L"SQLyog Session Savepoints(*.ysav)\0*.ysav;"
#define     SQLYOG_FILES		L"SQLyog Files(*.sql; *.queryxml; *.schemaxml)\0*.sql;*.queryxml;*.schemaxml\0\
SQL Files(*.sql)\0*.sql\0\
QueryXML Files(*.queryxml)\0*.queryxml\0\
SchemaXML Files(*.schemaxml)\0*.schemaxml\0\
All Files(*.*)\0*.*\0"  

#define		SQL					L"SQL Files(*.sql)\0*.sql\0\
QueryXML Files(*.queryxml)\0*.queryxml\0\
SchemaXML Files(*.schemaxml)\0*.schemaxml\0\
SQLyog Files(*.sql; *.queryxml; *.schemaxml)\0*.sql;*.queryxml;*.schemaxml\0\
All Files(*.*)\0*.*\0"

#define     QUERYXML            L"QueryXML Files(*.queryxml)\0*.queryxml\0\
SchemaXML Files(*.schemaxml)\0*.schemaxml\0\
SQL Files(*.sql)\0*.sql\0\
SQLyog Files(*.sql; *.queryxml; *.schemaxml)\0*.sql;*.queryxml;*.schemaxml\0\
All Files(*.*)\0*.*\0"

#define		SCHEMAXML	  		L"SchemaXML Files(*.schemaxml)\0*.schemaxml\0\
QueryXML Files(*.queryxml)\0*.queryxml\0\
SQL Files(*.sql)\0*.sql\0\
SQLyog Files(*.sql; *.queryxml; *.schemaxml)\0*.sql;*.queryxml;*.schemaxml\0\
All Files(*.*)\0*.*\0"


#define		DEFAULT_DB_TEXT								_(L"(Select a database...)")
#define		DEFAULT_NO_TABLE							_("(Read Only)")
#define		DEFAULT_NO_TABLEW							_(L"(Read Only)")
#define		DEFAULT_READONLY_RESULT						_(L"Read-only result")
#define		BUYLABEL									"click here"
#if			IS_FROM_WINDOWS_STORE == 1
#define		BUYURL										"https://www.webyog.com/product/sqlyog/?utm_source=sqlyogapp&utm_medium=referral&utm_campaign=windowsstore"
#else
#define		BUYURL										"http://www.webyog.com/product/sqlyogpricing"
#endif
#define		BUYURL_POWERTOOLS							"http://www.webyog.com/product/sqlyogpricing/?ref=community.powertools"
////#define		BUYURL_NAGBEGIN								"http://www.webyog.com/product/sqlyogpricing/?ref=community.nagbegin"
#define		BUYURL_TOOLBARRIBBON						"http://www.webyog.com/product/sqlyogpricing/?ref=community.toolbarribbon"
#define		BUYURL_NAGEND								"http://www.webyog.com/product/sqlyogpricing/?ref=community.nagend"
#define     HOMEURL_ABOUTUS								"http://www.webyog.com/?ref=community.aboutus"
#define     BUYURL_STATUSBAR							"http://www.webyog.com/product/sqlyogpricing/?ref=community.statusbar"
#define     HOMEURL_UPGRADECHECK						"http://www.webyog.com/?ref=community.upgradecheck"
////#define		BUYURL_BUYNAGBEGIN							"http://www.webyog.com/product/sqlyogpricing/?ref=community.buynagbegin"
#define		BUYURL_BUYNAGEND							"http://www.webyog.com/product/sqlyogpricing/?ref=community.buynagend"
#define		SUPPORTURL									"https://www.webyog.com/customer?ref=sqlyog"

#if			IS_FROM_WINDOWS_STORE == 1
#define     BUYENT										"https://www.webyog.com/product/sqlyog/?utm_source=sqlyogapp&utm_medium=referral&utm_campaign=windowsstore"
#else
#define     BUYENT										"http://www.webyog.com/product/sqlyogpricing"
#endif
#define		IMAGEURL									"http://www.webyog.com"
#define     PRODUCTURL                                  "https://www.webyog.com/product/sqlyog"
#define     COMMUNITYURL                                "https://www.webyog.com/community" 

// prototypes for some magic numbers
#define		SIZE_64										64
#define		SIZE_128									128
#define		SIZE_192									192
#define		SIZE_256									256
#define		SIZE_512									512
#define		SIZE_1024									1024	
#define		SIZE_5120									(20*1024)	

#define		CURRENT_TIMESTAMP							"CURRENT_TIMESTAMP"

// prototypes of column names which are used in manage indexes.
#define		UNIQUECOL									1
#define		KEYNAME										2
#define		KEYCOLNAME									4
#define		FULLTEXT									9

// prototypes of column index when various table is processed for user information.
#define		USER_SELECT									3
#define		USER_INSERT									4
#define		USER_UPDATE									5
#define		USER_DELETE									6
#define		USER_CREATE									7
#define		USER_DROP									8
#define		USER_RELOAD									9
#define		USER_SHUTDOWN								10
#define		USER_PROCESS								11
#define		USER_FILE									12
#define		USER_GRANT									13
#define		USER_REFERENCE								14
#define		USER_INDEX									15
#define		USER_ALTER									16

#define		USER_SHOWDB									17
#define		USER_SUPER									18
#define		USER_CREATETMP									19
#define		USER_LOCKTABLE								20
#define		USER_EXECUTE								21
#define		USER_REPLSLAVE								22
#define		USER_REPLCLIENT								23

#define		USER_CREATEVIEW								24
#define		USER_SHOWVIEW								25
#define		USER_CREATEROUTINE							26
#define		USER_ALTERROUTINE							27
#define		USER_CREATEUSER								28

#define		USER_EVENT									29
#define		USER_TRIGGER								30

// prototypes of column index when we get table information for Table Advance Properties
// in Create and Alter Table.

#define		TTYPE										1
#define		ROWFORMAT									2
#define		AVGROWLENGTH								4
#define		TABLECOMMENT								14
#define		MINMAXROW									13

#define		TV_ASCENDING								1
#define		TV_DESCENDING								2
#define		TV_UNKNOWN									3

/* some macros about colors in the treeview in diff tool */
#define		RGB_DIFFBLACK				RGB(0,0,0)
#define		RGB_DIFFWHITE				RGB(255,255,255)
#define     RGB_DIFFBLUE                RGB(81, 131, 184)
#define		RGB_DIFFGREY				RGB(128,128,128)
#define		RGB_DIFFGREEN				RGB(0,128,0)
#define		RGB_DIFFGREY_NEW			RGB(128,128,128)
#define		RGB_DIFFGREEN_NEW			RGB(0,128,0)
#define		RGB_DIFFSILVER				RGB(200,200,200)
#define		RGB_DARK_BLUE				RGB(0, 0, 128)

# define	STACKED_DEFAULT					1
# define	LINEBREAK_DEFAULT				1
# define	ASANDALIASALIGN_DEFAULT			1
# define	INDENTATION_DEFAULT				2

#define		FORMATTERPREFA				"FORMATTERPREF"

#define		MAX_COL_WIDTH				256

#define		MAXFILESIZE_2MB				2097152
#define		MAXFILESIZE_20MB			20971520

#define MENUBMPX		16
#define MENUBMPY		16

#define	ICON_SIZE		16

#define	ICON_SIZE_24	24
#define	ICON_SIZE_32	32
#define	ICON_SIZE_28	28

#define	 YYSMALL			"yyyy-mm-dd"
#define	 YYLONG				"yyyy-mm-dd hh:mm:ss"

#define	 WRITETOFILE(a,b,c)	VERIFY(WriteFile(a,b,strlen(b)*sizeof(wyChar),c,NULL));	

#define	 SHOWVALERROR(a,b) yog_message(a,b,pGlobals->m_appname.GetAsWideChar(),MB_ICONERROR | MB_OK); \
							SetFocus(a);

#define	SQLITEFILE_DATATAB "ColumnAttributes.db"

#define MAX_HISTORY_SIZE		(1024*1024*16)

//datatype mask values
#define NUMERIC_DATA		1
#define SHORTSTRING_DATA	2
#define LONGSTRING_DATA		4
#define BLOB_DATA			8
#define DATETIME_DATA		16
#define BIN_DATA			32
#define JSON_DATA			64
/// Function to notify user with various stuff.
/**
@param hwnd         : IN Window HANDLE
@param message      : IN Window message
@param wparam       : IN Unsigned int message pointer
@param lparam       : IN Long message pointer
@returns long pointer
*/
typedef LRESULT(CALLBACK* GVWNDPROC)(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

/// Function to compare items, used by qsort and bsearch 
/**
@param arg1     : IN First argument 
@param arg2     : IN second argument
@returns zero if both the strings are identical else non zero value
*/
wyInt32	compare		(const void *arg1, const void *arg2);

/// This is a general function which adds all the query which has been executed in 
/// the program into the current MDIWINDOWS QueryHistory Edit Box.
/**
@param wnd          : IN Current window pointer
@param tunnel       : IN Tunnel pointer
@param mysql        : IN Pointer to mysql pointer
@param query        : IN Query string
@param length       : IN Length of the query
@param batch        : IN Batch mode query execution
@param isend        : IN Checks for all query execution
@param stop         : IN Specific to ssh tunnel stopping
@param querycount   : IN Number of queries to execute
@param profile      : IN Enables display in history tab
@param currentwnd   : IN Current window
@returns zero on success else non zero value
*/
wyInt32 my_query(MDIWindow *wnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *query, wyUInt32 length, wyBool batch = wyFalse, 
                 wyBool isend = wyFalse, wyInt32 *stop = 0, wyInt32 querycount = 0, wyBool profile = wyTrue, 
				 wyBool currentwnd = wyTrue, bool isread = false, wyBool isimport = wyFalse, wyBool fksethttpimport = wyFalse, 
				 HWND fortransactionprompt = NULL);

/// Profiling the query or comment(SQLyog reconnected/failed) to history tab
/**
@param wnd			: IN Con. window pointe
@param timetaken	: IN time in milliseconds	
@param proftext		: IN query/comment to added to history
@param iscomment	: IN def. flag tells whether its query or comment
@return wyTrue on success else return wyFalse
*/
wyBool  my_queryprofile(MDIWindow *wnd, wyInt64 timetaken, const wyChar *proftext, wyBool iscomment = wyFalse, wyInt32 errnum=0, const wyChar *errormsg=NULL);

/// This function is wrapper to the Win32 messagebox. Before calling MessageBox it gets the HWND
/// which has focus and after the message box has been shown it returns the focus back to that HWND
/// It returns the return value of the MessageBox originally indented.
/**
@param hwnd         : IN Window HANDLE
@param lptext       : IN Text
@param lpcaption    : IN Caption 
@param utype        : IN Type
@returns zero on success else non zero value
*/
wyInt32 yog_message(HWND hwnd, LPCTSTR lptext, LPCTSTR lpcaption, wyUInt32 utype);

/// Gets the active mysql pointer
/**
@returns pointer to mysql pointer
*/
PMYSQL GetActiveMySQLPtr();

/// Exports data to files.
/**
@param hwnd     : IN Window HANDLE
@param message  : IN Window messages
@param wparam   : IN Unsigned message parameter
@param lparam   : IN LPARAM parameter
@returns 0 on success
*/
wyInt32 CALLBACK	ExpDataDlgProc	 (HWND hwnd, wyUInt32 message, WPARAM wParam, LPARAM lParam);

/// Shows shortcutkey dialogbox
/**
@param hwnd     : IN Window HANDLE
@param message  : IN Window messages
@param wparam   : IN Unsigned message parameter
@param lparam   : IN LPARAM parameter
@returns 0 on success
*/
INT_PTR CALLBACK	KeyShortCutsDlgProc	(HWND hwnd, wyUInt32 message, WPARAM wParam, LPARAM lParam);

/// Shows version history dialogbox
/**
@param hwnd     : IN Window HANDLE
@param message  : IN Window messages
@param wparam   : IN Unsigned message parameter
@param lparam   : IN LPARAM parameter
@returns 0 on success
*/
wyInt32 CALLBACK VersionHistDlgProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam);

/// Determines whether the field is number or not
/**
@param t		: IN field type
@returns wyTrue if number  else wyFalse
*/
wyBool IsNumber(enum_field_types t);

/// Function to escape a string while exporting data
/**
@param pstr     : IN String to Escape
@param size     : IN Source size 
@param newsize  : OUT Target size
@param esc      : IN Escape char
@param isfterm  : IN Field escaping require or not
@param fterm    : IN The escape character for fields
@param isfenc   : IN Enclosing escape character required or not
@param isfenc   : IN Enclosing escape character
@param islterm  : IN Line termination escape is required or not
@param lterm    : IN Escape character for line termination
@param isescaped: IN Escaped character is valid or not
*/
LPSTR MySqlEscape(LPCSTR pstr, DWORD size, wyUInt32* newsize, wyChar esc, 
                  wyBool isfterm, wyChar fterm, wyBool isfenc, wyChar fenc, 
                  wyBool islterm, wyChar lterm, wyBool isescaped = wyTrue, wyBool isescdoublequotes = wyFalse);

/// Function writes a formatted text on a text buffer and return the pointer.
/**
@param tunnel   : IN Tunnel pointer
@param myres    : IN Mysql result set, MYSQL_DATAEX*
@param myfield  : IN Mysql field
*/
wyChar* FormatResultSet(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield, MySQLDataEx *mdata = NULL);

wyInt32 GetMaxFieldLength();

wyBool    VerifyMemory(LPSTR* lpstr, wyUInt32* ptotalsize, wyUInt32 nsize, wyUInt32 bytestobeadded = 0);

/// Function to format create table resultset, unlike FormatResultSet, this function is very specific to
///	create table statement and does not create unwanted whitespace for the big create table function
/**
@param tunnel   : IN Tunnel pointer
@param myres    : IN Mysql result set
@param myfield  : IN Mysql field
*/
wyChar* FormatCreateTableResultSet(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield);

/// Calculates total count required for showing the title and dash.
/**
@param myres    : IN Mysql result set
@param myfield  : IN Mysql field
*/
wyUInt32    GetTitleCount(MYSQL_RES * myres, MYSQL_FIELD * myfield);

/// Returns the max length of the longest line in create table statement
/**
@param tunnel   : IN Tunnel pointer
@param myres    : IN Mysql result set
*/
wyUInt32    GetMaxLineCount(Tunnel * tunnel, MYSQL_RES * myres);

/// Specific function to get title length for create table query formatting to text
/**
@param tunnel   : IN Tunnel pointer
@param myres    : IN Mysql result set
@param myfield  : IN Mysql field
*/
wyUInt32    GetCreateTableTitleCount(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield);

/// Function to format the foreign reference things properly 
/**
@param key		: IN Key
*/
wyChar*	FormatReference(wyChar * key);

/// Font for the window
/**
@param logf     : IN Font details
@param hwnd     : IN Window HANDLE
*/
void	FillEditFont(PLOGFONT logf, HWND hwnd);

/// Font for the window data
/**
@param logf     : IN Font details
@param hwnd     : IN Window HANDLE
*/
VOID	FillDataFont(PLOGFONT logf, HWND hwnd);

/// Gets the max data length after which the rest is trunc
/**
@returns wyTrue if TruncData found else wyFalse
*/
wyBool	IsDataTrunc();

// Functions receives handle to edit control as parameter.
// Reads the value from .ini file about the word wrapping mode and sets it accordingly.
/**
@param hwnd         : IN Window Handler
@param isrich       : IN Rich editor or not
@param scintilla    : IN Scintilla editor or not    
*/
wyInt32		SetEditWordWrap(HWND hwnd, wyBool isrich = wyTrue, wyBool scintilla = wyFalse);

// Employs advanced heuristic algorithm to get the best  width when the user has selected
// data truncation option.
/**
@param hwnd         : IN Window Handler
@param field_list   : IN Mysql field list
@param skipwidth    : IN Width to skip
@param field_count  : IN no of fields
*/
wyUInt32*	CalculateCorrectWidth(HWND hwnd, MYSQL_FIELD * field_list, wyInt32 skipwidth, wyInt32 field_count);

// Helper function to know whether the main window is in maximized state or not. If its 
// then we need to increase the menu 
/**
*/
wyBool	IsWindowMaximised();

/// Functions to maintain the state of current active database for the connection
/**
@param text			: IN Text
*/
const wyChar *LeftPadText(const wyChar *text);

/// Changes the context database.
/**
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Pointer to mysql pointer
@param query         : IN Query
@param changeincombo : IN To change in combo box or not
*/
wyBool	ChangeContextDB(Tunnel * tunnel, PMYSQL mysql, const wyChar *query, wyBool changeincombo = wyTrue);
#ifndef COMMUNITY
wyBool		ChangeTransactionState(MDIWindow *wnd, const wyChar *query);
wyBool		CheckTransactionStart(MDIWindow *wnd, const wyChar *query);
wyBool		ReadOnlyQueryAllow(wyString *str);
wyString	RemoveExtraSpaces(wyString query);
#endif
/// Gets the database name from the query.
/**
@param query		: IN Query string
@param db			: IN Database name
*/
void	GetDBFromQuery(const wyChar *query, wyString  &db);

/// Redraws scintilla
/*
@param wnd			: IN/OUT Window HANDLE
@param redraw		: IN Redraw state
*/
VOID	SciRedraw(MDIWindow	*wnd, wyBool redraw);

/// Function fills up the COMBOBOX with database names from the object browser in the current
///	active query window.
/*
@param hwndCombo    : IN String to rotate
@param isextended   : IN Extended or not
*/
wyBool	FillComboWithDBNames(HWND hwndCombo, wyBool isextended = wyTrue);


/// Function reads the global .ini file and returns whether to warn user on update row
/// problem
/**
@returns wyTrue if PromptUpdate found else wyFalse
*/
wyBool	UpdatePrompt();

/// Checks for the backquotes option for query query in the .ini file
/**
@returns wyTrue if ProfileQuery found else wyFalse
*/
wyBool  AppendBackQuotes();

/// Checks for the show data in the .ini file
/**
@returns wyTrue if ShowData found else wyFalse
*/
wyBool	ShowData();

/// Function returns whether to set focus on edit control or result pane after execution of
/// query
/**
@returns wyTrue if FocusOnEdit found else wyFalse
*/
wyBool  IsEditorFocus();
/// function is using for  finding transaction support(whether required or not)
/**
@retuens wyTrue if enable transaction is checked
*/
wyBool IsStartTransactionEnable();
///checking  refresh option  is changed from F5 to f9
/**
@returns wyTrue if switch shortcut button is checked in preference
*/
wyBool	IsRefreshOptionChange();
/// Checks for the confirm on tab close in the .ini file
/**
@returns wyTrue if ConfirmOnTabClose found else wyFalse
*/
wyBool	IsConfirmOnTabClose();

/// Checks whether smart keyword is switched on or not
/**
@returns wyTrue if smart keyword is enabled else wyFalse
*/
wyBool  IsSmartKeyword();

/// Checks for the 'insert text on double click condition' from the info file
/**
@returns wyTrue if GetTextOnDBClick found else wyFalse
*/
wyBool	IsInsertTextOnDBClick();

/// Checks for the get object info always from the info file
/**
@returns wyTrue if always else wyFalse
*/
wyBool	IsGetObjectInfoAlways();

/// Checks whether tables folder open or not  when we click on database 
/**
@returns wyTrue if always else wyFalse
*/
wyBool  IsOpenTablesInOB();

/// Checks if connection restore is enabled
/**
@returns wyTrue if always else wyFalse
*/
wyBool  IsConnectionRestore();

///Checks whether Show all is selected in Data Tab or not
/*
@returns wyTrue if Show All is Selected otherwise wyFalse
*/
wyBool		IsShowAllInTableData();

///Whether the Format option selected in Info tab is HTML or Text
/**
@return wyTrue if its HTML format, return wyFalse for Text Format
*/
wyBool		IsInfoTabHTMLFormat();

//return the function case used to display functoions in scintilla editor
wyInt32  GetFunCase();

//return the keyword case used to display keywords in scintilla editor 
wyInt32  GetKeyWordCase();

/// Gets the column name
/**
@param field    : IN/OUT Field
@returns void
*/
void	GetColumnName(wyWChar *field);

int		IsColumnTypeJson(wyWChar *field);

/// Ensures a range is visible in the scintilla control
/**
@param hwnd					: IN Window Handler
@param posstart				: IN Starting position
@param posend				: IN Ending position
@returns void
*/
void	    EnsureRangeVisible(HWND hwnd, wyInt32 posstart, wyInt32 posend);

/// Handle and dispatch all messages till a event is signaled */
/**
@param event				: IN Event handle
@returns void
*/
void	    HandleMsgs(HANDLE event, wyBool istranslateaccelerator = wyTrue,HWND hwnd = NULL);

/// 
/**
@param tunnel				: IN Tunnel pointer.
@param myfieldres			: IN Mysql result set.
@param myres				: IN Mysql result set.
@param field				: IN Column name.
@returns default value on success else NULL
*/
wyChar*		GetDefaultValue(Tunnel* m_tunnel, MYSQL_RES * myfielres, MYSQL_RES * res, const wyChar * field);

/// Generic function returns the width of a column for the grid.
/**
@param grid					: IN Grid window HANDLE
@param field				: IN Mysql field names.
@param index				: IN Indexes
@returns width of the columns
*/
wyInt32		GetColWidth(HWND grid, MYSQL_FIELD * field, wyInt32 index);

/// Function to return the alignment of the column based on the type.
/**
@param field				: IN Mysql field names.
*/
wyInt32		GetColAlignment(MYSQL_FIELD * field);

/// Generic function to add enum or set values to a column in a grid.
/**
@param hwndgrid				: IN Grid HANDLE
@param tunnel				: IN Tunnel pointer.
@param name					: IN Column name.
@param myfieldres			: IN Mysql result set.
@param res					: IN Mysql result set.
@param index				: IN Indexes
@returns wyTrue on success else wyFalse
*/
wyBool		AddEnumValues(HWND hwndgrid, Tunnel * tunnel, const wyChar * name, 
						   MYSQL_RES * myfieldres, MYSQL_RES * res, wyInt32 index);

/// Common methods to find whether a given column name is of type of set or enum
/**
@param tunnel				: IN Tunnel pointer.
@param myfieldres			: IN Mysql result set.
@param myres				: IN Mysql result set.
@param name					: IN Column name.
@returns wyTrue if enum else wyFalse
*/
wyBool		IsColumnEnum(Tunnel * tunnel, MYSQL_RES * fieldres, MYSQL_RES * myres, const wyChar * name);

/// Function to check whether the given column is enum or set.
/**
@param tunnel				: IN Tunnel pointer.
@param myfieldres			: IN Mysql result set.
@param res					: IN Mysql result set.
@param name					: IN Column name.
@returns wyTrue if set else wyFalse
*/
wyBool		IsColumnSet(Tunnel * tunnel, MYSQL_RES * myfieldres, MYSQL_RES * res, const wyChar * name);

/// Searches for the column name in the key res and finds if its part of a primary key column
/**
@param tunnel				: IN Mysql tunnel pointer
@param myfieldres			: IN Mysql result set
@param column				: IN Column name
@returns wyTrue if set else wyFalse
*/
wyBool		IsColumnPrimary(Tunnel * tunnel, MYSQL_RES * myfieldres, wyChar * column);

/// Whether there is a primary key for the table or not.
/**
@param tunnel				: IN Tunnel Pointer.
@param mykeyres				: IN Mysql result.
@returns wyTrue if primary else wyFalse
*/
wyBool		IsAnyPrimary(Tunnel * tunnel, MYSQL_RES * mykeyres, wyInt32 * pcount = NULL);

/// Checks if a field can be made null.
/**
@param tunnel				: IN Tunnel pointer.
@param myfieldres			: IN Mysql result.
@param field				: IN Field name.
@returns wyTrue if nullable else wyFalse
*/
wyBool		IsNullable(Tunnel* tunnel, MYSQL_RES * myfieldres, wyChar * field);

///Generic function to write text in an xml file handling characters.
/*
@param file					: IN File HANDLE.
@param text				: IN Text to write
@param crlf					: IN carriage return.
@returns wyTrue on success else wyFalse
*/
wyBool      WriteXMLToFile(HANDLE file, const wyChar * text, wyBool crlf);

/// Returns the datatype of the perticular field.
/*
@param tunnel				 : IN Tunnel pointer.
@param myfieldres			 : IN Mysql result pointer.
@param  field				 : IN Field.
@returns type of data
*/
wyChar*		GetDataType(Tunnel* tunnel, MYSQL_RES * myfieldres, const wyChar * field);

/// Gets the current window version
/**
@returns void
*/
VOID		CollectCurrentWindowsVersion();

/// Converts multibyte to widechar string
/**
@param strtoconv			: String to convert.
@returns converted string
*/
WCHAR*      GetWideString(const wyChar *strtoconv);

/// Loads the Help file(.chm).
/**
@returns wyTrue on success else wyFalse
*/
//wyBool      LoadHelpFile();//not needed anymore, removing helpfile from version 12.05

/// Initializes the custom control grid and tab.
/**
@returns void
*/
void        InitCustomControls();

/// Writes the content of buffer to a file.
/**
@param hfile			: IN File HANDLE.
@param buffer			: IN Char buffer
@returns void
*/
void        WriteToFile(HANDLE hfile, const wyChar *buffer);

/// Writes to xml file
/**
@param hwndpage			: IN Window Page
@param hfile			: IN File HANDLE
@param id				: IN Id of the container control
@param crlf				: IN carriage return
@returns void
*/
void        HandlerToWriteXMLToFile(HWND hwndpage, HANDLE hfile, wyInt32 id, wyBool crlf);

/// Writes tag for SMTP
/**
@param hwndpage			: IN Window Page
@param hfile			: IN file HANDLE
@param tag				: IN Tag 
@param id				: IN id
@param crlf				: IN carriage return
@returns void
*/
void        WriteSmtpTag(HWND hwndpage, HANDLE hfile, const wyChar *tag, wyInt32 id, wyBool crlf);

/// Creates the connection type
/**
@returns new connection
*/
ConnectionBase* CreateConnection();

/// Creates the preference type
/**
@returns new preferences
*/
PreferenceBase* CreatePreferences();

/// Gets the tab size
/**
@returns tab width
*/
wyInt32     GetTabSize();

/// Remove the comment from a given string
/**
@param buffer		: IN/OUT String
@returns the uncommented string
*/
wyChar      *GetCommentLex(wyChar *buffer);

/// Get the column length information
/**
@param row      : IN Mysql row
@param col      : IN Column number
@pram len       : OUT Length of column
*/
void            GetColLengthArray(MYSQL_ROW row, wyInt32 col, 
                                        wyUInt32 *len);

/// Initializes the GLOBAL structure
/**
@param pg		: Global struct object
returns void
*/
void InitGlobals(PGLOBALS pg);

/// Initializes the GLOBAL structure
/**
@param wnd		: IN con. window pointer
@param val      : IN is it a group process or not
returns void
*/
void SetGroupProcess(MDIWindow *wnd, wyBool val);

/// rebuilding specific 
/**
@param database  : IN database name
returns void

*/
void  SpecificDBRebuild(wyChar *database);


wyBool  RebuildACTags_SpecificDB(ConnectionInfo &src, const wyChar *db_name);


/// Takes Query Buffer replaces all '\n' with ' ' for proper display in history tab
/**
returns void
*/
wyChar* FormatQuery(const wyChar *query, wyInt32 &position);

/// Returns column UTF8 length
/**
@param row          : IN Row 
@param num_cols     : IN Number of columns
@param col          : IN Column number
@param collength    : OUT Column length
*/
void            GetUtf8ColLength(MYSQL_ROW row, wyInt32 num_cols, 
                                        wyInt32 col, wyUInt32 *collength, wyBool ismysql41 = wyTrue);

void			FetchFileNameFromCmdline(const wyChar *filename);

///Setting the width of the combobox
/**
@param hwndCombo	: IN Combobox Handle
@returns width of the combobox
*/
wyInt32			SetComboWidth(HWND hwndCombo);

///Setting focus to the query editor
/*
@param wnd			: IN parent connection window pointer.
@param hwnd			: IN handle to the parent editor tab.
returns void 
*/
void SetFocusToEditor(MDIWindow *wnd, HWND hwnd);

/// checks for Whether we need to refresh tabledata tab automatically or not .
/**
@returns wyTrue if we need to refresh automatically else wyFalse
*/
wyBool IsRefreshTableData();

/// Setting the size of the tool bar icons
/**
@returns size of icons.
*/
wyInt32 GetToolBarIconSize();

///checks for Whether we will use sapces for tabs or not.
/**
@returns wyTrue if spaces for tabs otherwise wyFalse
*/
wyBool        IsInsertSpacesForTab();

///checks whether we use the resized column width or the default width.
/**
@returns wyTrue if resized width otherwise wyFalse
*/
wyBool			IsRetainColumnWidth();

/// Checks for the Stacked or not Stacked in Formatter in the .ini file
/*
*/
wyBool	IsStacked();

/// Gets the LineBreak in Formatter in the .ini file
/*
*/
wyInt32 GetLineBreak();

/// Checks for the AS And Alias Alignment in Formatter in the .ini file
/*
*/

wyBool	IsASAndAliasAlignment();

/// Gets the SpaceIndentation for Formatter from the .ini file
/*
*/
wyInt32 GetIndentation();

///return back tick count is even or odd
/*
@param Str  :
@return wyTrue or wyFalse
*/
wyBool      IsEvenBackticks(const wyChar *str, wyChar chartype = '`');

///return no chars in side open paranthesis and its corresponding paranthesis
/**
@param query            :OUT parsing query
@returns   no of characters        
*/
wyInt32	  GetStrLenWithInParanthesis(const wyChar *query, wyBool isformatter = wyFalse);

/// match string aganist pattern
	/**
	@param query            :OUT parsing subject.
	@param pattern          :IN  matching pattern
	@param complieoptions   :IN pcre compling options.
	@param  ovector         :OUT output vector for substring information.
	@returns regex return values.
	*/
wyInt32	  MatchStringPattern(const wyChar*subject, wyChar *pattern, wyInt32 *ovector, wyInt32 complieoptions);

/// whether it matches  string aganist pattern
/**
@param subject            :OUT parsing subject.
@param pattern          :IN  matching pattern
@param complieoptions   :IN pcre compling options.
@returns 1 if it matches otherwise -1.
*/
wyInt32	 IsMatchStringPattern(const wyChar *subject, wyChar* pattern , wyInt32 pcrecompileopts);

///Generic function to add text in an xml file handling characters.
/*
@param buffer		: IN/OUT String
@param text			: IN Text to write
@param crlf			: IN carriage return.
@returns wyTrue on success else wyFalse
*/
wyBool      AddXMLToBuffer(wyString * buffer, const wyChar * text, wyBool crlf);

///Setting the width of the listbox
/**
@param hwndlist	: IN Listbox Handle
@returns width of the listbox
*/
wyInt32		SetListBoxWidth(HWND hwndlist);

///Show warnings in messages tab
wyBool		IsShowWarnings();


///Halt Query Execution on Errors
wyBool      IsHaltExecutionOnError();


/// Helper function to write the database information to xml file
/**
@param hfile                : IN handle to the file
@param dbname               : IN database name
@returns wyTrue on success else wyFalse
*/
wyBool                      WriteDatabaseInfo(HANDLE hfile, const wyChar *dbname);


/// Function to write xml tags
/**
@param hfile                : IN handle to the file
@param tags                 : IN the tags that are to be written
@returns wyTrue on success else wyFalse
*/
wyBool                      WriteXMLtags(HANDLE hfile, wyString *tags);


/// Refreshing the Object browser
/**
@param database				: IN data base to be selected in object browser
@return void
*/
void						ObjectBrowserRefresh(wyWChar *database);

/// Gets the list of tables in a given database
/**
@param database             : IN  database name
@param dbtablelist          : OUT list to store the tables
@param isview               : IN  differentiate between a VIEW and BASE TABLE
@returns 0 on success and -1 on mysql error
*/
wyInt32                     DatabaseTablesGet(const wyChar *database, List *dbtablelist, HWND hwndparent, wyBool isview = wyFalse);

/// Gets the default limit values.
/**
@returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
*/
wyBool  GetLimitValues();

//Whether the Pging option is enabled or not
/**
@return wyTrue if enabled, else return wyFalse
*/
wyBool	IsResultTabPagingEnabled();

void    GetTabPositions();

#endif
