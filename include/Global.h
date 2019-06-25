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


#ifndef	_CGlobal_H_
#define _CGlobal_H_

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <commctrl.h>
#include <mysql.h>
#include <string>
#include <sqlite3.h>
#include "wyString.h"
using namespace std;
#include "datatype.h"

#include "wyIni.h"
#include "wyFile.h"

class FrameWindow;
class MDIWindow;
class CQueryObject;
class EditorQuery;
class StatusBarMgmt;
class TabMessage;
class TabResult;
class CQueryList;
class CAddUser;
class Tunnel;
class List;
class wyString;
class Announcements;
class wySQLite;
//class MDIlist;


#define IS_NUMBER(t) ((t)==FIELD_TYPE_LONG || (t)==FIELD_TYPE_LONGLONG || (t)==FIELD_TYPE_DECIMAL || (t)==FIELD_TYPE_DOUBLE || (t)==FIELD_TYPE_FLOAT || (t)==FIELD_TYPE_INT24 || (t)==FIELD_TYPE_YEAR || (t)==FIELD_TYPE_SHORT || (t)==FIELD_TYPE_TINY || (t)== FIELD_TYPE_NEWDECIMAL)
#define IS_DATE(t)	 ((t)==MYSQL_TYPE_DATE || (t) == MYSQL_TYPE_TIME || (t) == MYSQL_TYPE_DATETIME || (t) == MYSQL_TYPE_YEAR ||(t) == MYSQL_TYPE_NEWDATE || (t)==FIELD_TYPE_TIMESTAMP)

#define CSVSECTION	"CSVESCAPING"
#define EXPORTCSVSECTION	"EXPORTCSVESCAPING"

#define	GENERALPREF L"GENERALPREF"
#define	GENERALPREFA "GENERALPREF"

#define REFRESHLIST			_(L"(View all databases)")
#define SELECTDB			_(L"(Select a database...)")

#define CUSTOM_DBCLICK		12345
#define	UM_SETFOCUS			7898
#define	UM_ADDBACKQUOTEONAC	8001

#define MNUFILE_INDEX			1
#define MNUEDIT_INDEX			2
#define MNUFAV_INDEX			3
#define MNUDB_INDEX				4
#define MNUTBL_INDEX			5
#define MNUOBJ_INDEX			6
#define MNUTOOL_INDEX			7
#define MNUPTOOL_INDEX			8
#define MNUTRANSACTION_INDEX	9
#define MNUWINDOW_INDEX			10
#define MNUHELP_INDEX			11
#define MNUENGINE_INDEX			20

#define DEF_TAB_SIZE			8
#define TOOLBARICONSIZE_DEFAULT	"LARGE"

#define WM_SETACTIVEWINDOW      WM_USER+200

#define IDT_TIMER				WM_USER+201
#define AUTO_THR_EXIT			WM_USER+202
#define AUTO_THR_EXIT2			WM_USER+203
#define AUTO_CHILD_THR_EXIT		WM_USER+204
#define WM_CUSTCONTEXTMENU		WM_USER+205
#define UM_SETFOCUSAFTERFINDMSG WM_USER+206

#define SQLITE_TIMEOUT			1000
#define WAIT_FOR_AUTOTHREADEXIT 5000
#define TYPE_PROCEDURE			6
#define TYPE_FUNCTION			7
#define TYPE_TABLE				5
#define DEF_IMG_WIDTH			16
#define DEF_IMG_HEIGHT			16
#define TOOLBAR_HEIGHT			(DEF_IMG_HEIGHT + 14)
#define TOOLBAR_WIDTH			(20 * DEF_IMG_WIDTH) + 70
#define TOOLBAR_COMBO_WIDTH		150
#define MAX_TAB_BUTTON_COUNT	30
#define STATUS_BAR_FIRSTSECTION_PERCENTAGE 0.30
#define STATUS_BAR_HEIGHT		20

#define MDI_VERTICAL_POS		30
#define MAX_SECTION_SIZE		32768
#define AC_PRE_KEYWORD			1
#define AC_PRE_FUNCTION			2
#define AC_DATABASE				3
#define AC_TABLE				4
#define AC_COLUMN				5
#define AC_SP					6
#define AC_FUNCTION				7
#define ROWSIZE_128				128
#define AC_WRDLEN_128			128
#define AC_WRDLEN_256			256
#define KEYWORD_XPM {"    10    12        8            1","` c #05055c",". c #dbdef8","# c #1f257a","a c #f2f7fb","b c #05058d","c c #443e9c","d c #1a1993","e c #141754","aaaaa.aaaa","a.aaeaaaaa","aaeee.aaaa","aaa#e..aaa","aa.`bad`ee",".aaed.bdaa","aa.#bbdaaa","aa.ddbd`aa","a.adb..bd.","aae`#`dd`c","aa.aaaaaaa","aaaaaaaaa."}
#define FUNCTION_XPM {"    10    12        8            1","` c #a41629",". c #fcd6e0","# c #fcedeb","a c #f8fcf8","b c #b71c28","c c #773222","d c #df0e27","e c #a02e32","a#a#####aa","aa##cccaaa","aa#cc#a#aa","a##ee.#aaa","a#`ddb#aaa","a#.dd.#aaa","aa.b`.aaaa","a#.`e#aaaa","aa.bb.#aaa","aae``e##aa","aaaaaa#aaa","aaaaaaaaaa"}
#define DATABASE_XPM {"    10    12        8            1","` c #5c5d6c",". c #b6b5d0","# c #e5e5f9","a c #8c8ba0","b c #cccce5","c c #747689","d c #a5a5bd","e c #fafbfc","eeeeeeeeee","e`ab.da`ee","e###bd.dee","ed..bbadee","e###e#b.ee","ecb#.b.aee","eb#bb##dee","ecbb..daee","ebeb.bbdee","ecdbdd.`ee","e#`da.`#ee","eeeee##eee"}
#define TABLE_XPM {"    10    12        8            1","` c #868794",". c #c8cad9","# c #e6e7f7","a c #a8a9b1","b c #d9daea","c c #b9bac6","d c #fafcfc","e c #9092a1","dddddddddd","daaeeea`dd","#.`e#e`.dd","d.dd.d#cdd","d#.#b.bbdd","dcd#b##add","dbe`.eecdd","da.#c.#add","d.#db#dcdd","dbb#.b#.dd","ddd#dddddd","dddddddddd"}
#define FIELD_XPM {"    10    12        8            1","` c #040404",". c #e4e4e4","# c #f4f5f4","a c #242224","b c #101010","c c #ecedec","d c #fcfdfc","e c #0c0a0c","dddddcdddd","dcdd#d.ddd","###ddddddd","ddd#d.dddd","d#.e`a`ddd","cd`bd.``dd","dd``dd``dd","d#``#ddddd","d#ebdd#`dd","cdc````ddd","d.ddd#dddd","d#dcddcddd"}
#define SP_XPM {"    10    12        8            1","` c #343314",". c #c3a73a","# c #f3df2c","a c #eedeb4","b c #faf9ef","c c #b8a472","d c #d2bf83","e c #dac134","bbbdabbbbb","ba``caabbb","bc`d#eedbb","bae##e.dbb","ba#e##.abb","bc###..cbb","ade#eebabb","bceee.bbbb","bacdadbbbb","bbabbbbbbb","bbbbbbbbbb","bbbbbbbbbb"}
#define FUNC_XPM {"    10    12        8            1","` c #040404",". c #e8e8e8","# c #141614","a c #fbfcfb","b c #101010","c c #242624","d c #f2f1f2","e c #0c0a0c","aadaddaadaaaaa","addddaaaaaaaaa","a.aaaaa.aaaaaa","aadd`aaaaaaaaa","daa``aadaaaaaa","aa````baaaaaaa","daa`#aadaaaaaa","aaa``adadaaaaa","daac`a.aaadaaa","dad`bdadaaaaaa","daade``daaaaaa","ddadaaaaaaaaaa","daaadaaaaaaaaa","ad.aadaaaaaaaa","daaaaaadaaaaaa","aaaadadaaaaaaa","aaaaaaaaaaaaaa","aaaaaaaaaaaaaa"}
#define ALIAS_XPM {"    10    12        8            1","` c #5d5d5f",". c #9da9dc","# c #7c82a5","a c #8f94ad","b c #cad0e9","c c #b6bace","d c #666a7c","e c #dce4f8","``````````","#cc.c.aaad",".b..a....a","a..b##cb##","aa`dddd`##","a.ceaabea#","aa`ddddd##","a..b##cb#d","aaaa##aa#d","#.a##a#d#d","``````````","dddddddddd"};



/// Nag screen reponses
#define TRIAL_REG				1
#define TRIAL_CLOSE				2
#define TRIAL_LATER				3
#define TRIAL_LITE				4

#define FAV_ITEM_START_INDEX	3

#define	FAVORITEMENUID_START	50001
#define	FAVORITEMENUID_END		55000
#define ENGINE_ID_START         49001
#define ENGINE_ID_END           49100

#define IDT_WND_TIMER			30000

#define QUOTE_ERR_MSG			L"Error in using backquotes(`). This may be because of - \n"\
								L"1) If you are using backquote in the first place; you need to end that text with backquote.\n"\
								L"2) If you want to enter a backquote; you need to surround it with two backquote(``)."

#define INVALID_ROW_ARG			"Invalid Row argument! Please take a screenshot and contact WEBYOG"
#define INVALID_COL_ARG         "Invalid Col argument! Please take a screenshot and contact WEBYOG"

#define SORTICONWIDTH           15

#define	ININAMEDOBJECT			L"9046FD35-2E88-SQLyog85-INI-8EC2"

// The most important typedef .. change MYSQL * to PMYSQL which will be used everywhere

typedef MYSQL** PMYSQL;

/**
   * An enum.
   * Enum to store whether a word found in lex is what type.
	
    KEYWORD			0,
	FUNCTION		1,
	COMMENT			2,
	VALUE			3,
	ARITH			4,

*/
//enum TYPE
//{
//	KEYWORD,
//	FUNCTION,
//	COMMENT,
//	VALUE,
//	ARITH,
//};


/**
   * An enum.
   * Enum values to state which state the HTREEITEM is in
	
    SIMILAR		0,
	NEW			1,
	DIFFERENT	2,

*/
enum CHANGESTATE
{
	SIMILAR,
	NEW,
	DIFFERENT,
};


/*! \struct yogIcons
	\brief Icons information
    \param wyUInt32    m_menuid;        // Menu id
	\param wyUInt32    m_iconid;        // Icon id
*/
struct	yogIcons
{
	wyUInt32    m_menuid;
	wyUInt32    m_iconid;
};

/**
   * An enum.
   * Enum to store types of comment.
	
    COMMENT_OFF		0,
	COMMENT_HASH	1,
	COMMENT_DASH	2,
	COMMENT_START	3,
	INSIDE_TILDE	4.

*/
enum TOKCOMMENT
{
	COMMENT_OFF,
	COMMENT_HASH,
	COMMENT_DASH,
	COMMENT_START,
	INSIDE_TILDE
};


/**
   * An enum.
   * Enum to store types of OBJECTS.
	
    OBJECT_SERVER			0,
	OBJECT_DATABASE			1,
	OBJECT_TABLES			2,
	OBJECT_TABLE			3,
	OBJECT_PROCEDURES		4,
	OBJECT_PROCEDURE		5,
	OBJECT_FUNCTIONS		6,
	OBJECT_FUNCTION			7,
	OBJECT_VIEWS			8,
	OBJECT_VIEW				9,
	OBJECT_TRIGGERS			10,
	OBJECT_TRIGGER			11,
	OBJECT_EVENTS			12,
	OBJECT_EVENT			13,
	OBJECT_INDEX			14,
	OBJECT_TABLEDDL			15,
	OBJECT_VIEWDDL			16,
*/ 

enum OBJECT
{
	OBJECT_SERVER,
	OBJECT_DATABASE,
	OBJECT_TABLES,
	OBJECT_TABLE,
	OBJECT_PROCEDURES,
	OBJECT_PROCEDURE,
	OBJECT_FUNCTIONS,
	OBJECT_FUNCTION,
	OBJECT_VIEWS,
	OBJECT_VIEW,
	OBJECT_TRIGGERS,
	OBJECT_TRIGGER,
	OBJECT_EVENTS,
	OBJECT_EVENT,
	OBJECT_INDEX,
	OBJECT_TABLEDDL,
	OBJECT_VIEWDDL,
};


/*! \struct Listinfo
	\brief Icons information
    \param wyInt32		m_value;			// Id of the string
	\param wyInt32		m_adv;				// Whether to use advance editor
	\param wyChar	    m_caption[32];		// Text for the template
*/
struct Listinfo
{
	wyInt32		m_value;
	wyInt32		m_adv;
	wyChar	    m_caption[32];
};


/*! \struct tagCodePage
	\brief Icons information
    \param wyString     m_cpname;			// Code page name
	\param wyString		m_cpdesc;			// Code page description
	\param wyString		m_encodingname;		// Code page encoding
	\param wyInt32		m_ncharset;			// Code page charactor set
*/
typedef struct tagCodePage
{
	wyString     m_cpname;
	wyString	 m_cpdesc;
	wyString	 m_encodingname;
	wyInt32		 m_ncharset;
}CODEPAGE;

typedef struct SSHPipeEnds
{
	HANDLE		m_hreadpipe;
	HANDLE      m_hwritepipe;
	HANDLE      m_hreadpipe2;
	HANDLE		m_hwritepipe2;
}PIPEHANDLES;


/*! \struct __struct_tunnelauth
	\brief structure to keep information about tunnel authentication
		   basically used when there is a new connection or editing of an existing connection 

    \param bool		isproxy;					// Checks for proxy
	\param bool		ischallenge;				// Checks if challenge exists for the tunnel

	\param char		proxy[128];					// Proxy server name
	\param char		proxyusername[128];			// Proxy user name
	\param char		proxypwd[512];				// Password for proxy
	\param int		proxyport;					// Port number for the proxy

	\param char		chalusername[128];			// Challenge user name
	\param char		chalpwd[512];				// Challenge password
*/
typedef struct __struct_tunnelauth
{
	bool		isproxy;
	bool		ischallenge;
	bool		isbase64encode;
	bool		isencrypted;

	wchar_t		proxy[128];
	wchar_t		proxyusername[128];
	wchar_t		proxypwd[512];
	int			proxyport;

	wchar_t		chalusername[128];
	wchar_t		chalpwd[512];

	wchar_t 	content_type[128];
}TUNNELAUTH;


/*! \struct ConnectionInfo 
	\brief structure to keep information about tunnel authentication
		   basically used when there is a new connection or editing of an existing connection 

    \param wyString	m_db;							// database name
	\param wyString	m_title;						// Window Title
	\param MYSQL		*m_mysql;					// Mysql pointer
	\param Tunnel		*m_tunnel;					// Tunnel pointer

	\param CODEPAGE	m_codepage;						// Codepage details
	
	\param HANDLE		m_hprocess;					// HANDLE to process
	\param wyBool		m_isssh;					// Is the connection ssh ?
	\param wyString		m_sshuser;					// Ssh user name
	\param wyString		m_sshpwd;					// Ssh user password
	\param wyString		m_sshhost;					// Ssh host name
	\param wyInt32		m_sshport;					// Port number
	\param wyString		m_localhost;				// Local host name
	\param wyInt32		m_localport;				// Local port number
	\param wyInt32		m_origmysqlport;			// Mysql port number

    \param wyBool       m_ispassword;               // Is password selected or public key
    \param wyString     m_passphrase;               // SSH Pass phrase
    \param wyString     m_privatekeypath;           // SSH Private key path


    \param wyBool       m_issslchecked;             // Is ssl option checked
    \param wyBool       m_issslauthchecked;         // Is ssl authentication checked
    \param wyString     m_clikey;                   // Client key file name
    \param wyString     m_clicert;                  // Client certificate file name
    \param wyString     m_cacert;                   // Client CA file name
    \param wyString     m_cipher;                   // Client cipher

*/
struct ConnectionInfo 
{
	wyString    m_user;
    wyInt32     m_port;
	wyString	m_db;
    wyString    m_pwd;
    wyString    m_host;

	wyBool		m_isstorepwd;
	wyBool		m_iscompress;
	//wyBool		m_ispwdcleartext;
	wyBool		m_isdeftimeout;
	wyString	m_strwaittimeout;
	wyBool		m_isreadonly;
	wyString	m_title;
	MYSQL		*m_mysql;
	Tunnel		*m_tunnel;

	CODEPAGE	m_codepage;

	PIPEHANDLES m_sshpipeends;
	SOCKET		m_sshsocket;

	HANDLE		m_hprocess;
    wyString    m_charset;

    wyBool      m_ishttp;
    wyString    m_url;
    wyInt32     m_timeout;
	wyBool      m_ishttpuds;
	wyString    m_httpudspath;
					
	wyBool		m_isssh;
	wyString	m_sshuser;
	wyString	m_sshpwd;           /// SSH password, becomes passphrase when private key auth is selected
	wyString	m_sshhost;
	wyInt32		m_sshport;
	wyString	m_localhost;
	wyInt32		m_localport;
	wyInt32		m_origmysqlport;
    wyString    m_forhost;
    wyInt32     m_forport;
    wyBool      m_issshsavepassword; /// Flag to save or not to save the ssh password

    wyBool      m_ispassword;       /// Password or Privatekey
    wyString    m_privatekeypath;   /// Private key path

    wyBool      m_issslchecked;
    wyBool      m_issslauthchecked;
    wyString    m_clikey;
    wyString    m_clicert;
    wyString    m_cacert;
    wyString    m_cipher;	

	wyString	m_sqlmode;
    wyString    m_initcommand;
	wyBool		m_isglobalsqlmode;
    wyUInt32    m_keepaliveinterval;
	
	//color for object browser
	COLORREF	m_rgbconn;	
	COLORREF	m_rgbfgconn;

	//persist history tab
	wyBool		m_isHistOpen;

	//persist info tab
	wyBool		m_isInfoOpen;

	wyUInt32	m_isencrypted;
	
};


class ESCAPECHAR
{
public:

	ESCAPECHAR ();
	~ESCAPECHAR();

	/// Reads the contents of an .ini file
	/**
	@param  escape		: IN Section to search in the file 
	*/
	VOID	ReadFromFile(wyChar *escape);

	/// Writes the contents to an .ini file
	/**
	@param  escape		: IN Text to write in the file 
	*/
	VOID	WriteToFile(wyChar *escape);

	/// Removes escape chars from a given string
	/**
	@param in			: IN Text to remove escape chars from
	@returns void
	*/
	VOID	RemoveEscapeChars(wyChar *in, wyChar *out);
	
	/// NULL value replace by character
	wyChar		m_nullchar[10];

	/// Escaped by character.
	wyChar		m_escape[10];		

	/// Field escape.
	wyChar		m_fescape[10];		

	/// Line escape.
	wyChar		m_lescape[10];		

	/// Field enclosed by
	wyChar		m_enclosed[10];

	/// Is fields of fixed length ?
	wyBool		m_isfixed;
	
	/// Is is escaped ?
	wyBool		m_isescaped;

	/// Is line termination enabled ?
	wyBool		m_islineterminated;

	/// Is fields enclosed by enabled ?
	wyBool		m_isenclosed;

	/// Is field terminated by enabled ?
	wyBool		m_isfieldsterm;

	/// Is NULL replaced by enabled ?
	wyBool		m_isnullreplaceby;

	/// Is optionally enabled ?
	wyBool		m_isoptionally;

	/// Is add field names enabled ?
	wyBool		m_isincludefieldnames;

	/// Is the data ready to be written ?
	wyBool		m_towrite;

	/// Is include column names enabled ?
	wyBool		m_iscolumns;

	/// Is copy from clipboard option
	wyBool		m_isclipboard;
};

typedef ESCAPECHAR* PESCAPECHAR;

/*! \struct tagArray
    \brief used to retrieve the keywords/functions from the sqlite.db databases
			It is used for showing the image on auto complete dropdown
    \param wyInt32		rowcount				keywords/functions count
    \param wyInt32		maxcount				max count allocated
    \param wyChar		**keys					pointer to all the keyword/function array
    \param wyInt32		type					keyword type searching for
    \param wyBool		bflag					type tag need to be appended or not(E.g.: select?1, ...)
    
*/
struct tagArray
{
	wyInt32		rowcount;
	wyInt32		maxcount;
	wyChar      **keys;
	wyInt32		type;
	wyBool	    bflag;
};

/*! \struct tagsqlite
    \brief 
    \param wyInt32		rowcount				keywords/functions count
    \param wyInt32		maxcount				max count allocated
    \param wyChar		**keys					pointer to all the keyword/function array
    \param wyInt32		type					keyword type searching for
    \param wyBool		bflag					type tag need to be appended or not(Eg: select?1, ...)
    
*/
typedef struct tagsqlite
{
	wyString    m_connectiondb;
	sqlite3		*m_hdb;
    wyBool      m_signalled;
	wyInt32     m_refcount;
    HANDLE		m_hstorethrd;
	HANDLE		m_hmainevt;
	tagsqlite  *m_next;
} SQLITENODE, *PSQLITENODE;


/*! \struct tagGlobals
    \brief This structure is kept as a global variable and contains some global data.

    \param wyInt32		m_colcount;							// Number of column
    \param wyInt32		m_conncount;						// Number of connection

    \param wyString		m_lastdatabase;						// Last database selected

	\param wyBool		m_isautocomplete;					// Is autocomplete enabled ?
	\param wyBool		m_isautocompletehelp;				// Is autocomplete help enabled ?
    \param wyBool       m_isrefreshkeychange;                                   // find whether F5 and F9 functionality changes 
	\param wyBool		m_isshowtooltip;					// Is show tool tip enabled	?
	\param wyBool		m_findreplace;						// Is find and replace option selected ?
	
	\wyBool				m_isdefaultqb						// Is QB create by default with a new connection
	\wyBool				m_isdefaultsd						// Is SD create by default with a new connection
	\wyBool				m_isadddbcounter					// Is Count add to database objects in obect browser

	\param HWND			m_hwndclient;						// MDI client window HANDLE.
	\param FrameWindow* m_pcmainwin;						// Frame window HANDLE
	\param HINSTANCE	m_hinstance;						// Handle to an instance
    \param PSQLITENODE	m_psqlite;							// Structure that help to interact with sqlite db
	
    \param wyChar		m_helpfile[MAX_PATH+1];				// Path to help file
    \param wyString		m_appname;							// Application name

    \param wyInt32		m_tooltipdelay;						// Time for which the tool tip should be displayed
	\param wyInt32		m_sshdupport;						// SSH duplication 

	\param DWORD		m_windowsmajorversion;				// Future use only
	\param DWORD		m_windowsminorversion;				// Future use only
	\param DWORD		m_windowsbuild;						// Future use only

    \param HINSTANCE	m_entinst;							// Enterprise module HANDLE
    \param HANDLE		m_hmapfile;							// HANDLE to the memory map file

    \param wyInt32		m_modulenamelength;					// Current exe file name
    \param wyInt32		m_statusbarsections;				// Number of section in the status bar

   
    
*/ 

typedef struct tagGlobals
{
	wyInt32		m_colcount;
    wyInt32		m_conncount;

    wyString    m_lastdatabase;

	wyBool      m_isautocomplete;
    wyBool      m_isautocompletehelp;
	wyBool		m_isrefreshkeychange;
	wyBool      m_isshowtooltip;
	wyBool      m_expflg;
	wyBool      m_expchange;
	wyBool      m_findreplace;

	wyChar      m_autocompletetagsdir[MAX_PATH+10];
				
	HWND		m_hwndclient;	
	FrameWindow* 	m_pcmainwin;
	HINSTANCE	m_hinstance;
    PSQLITENODE	m_psqlite;

   // wyChar      m_helpfile[MAX_PATH+1];
	wyString	m_filename;
    wyString    m_appname;

    wyInt32     m_tooltipdelay;
	wyInt32     m_sshdupport;

	DWORD		m_windowsmajorversion;
	DWORD		m_windowsminorversion;
	DWORD		m_windowsbuild;

    HINSTANCE   m_entinst;
    HANDLE      m_hmapfile;

    wyInt32     m_modulenamelength;
    wyInt32     m_statusbarsections;
	wyBool		m_isconnected;

	wyBool		m_ispqaenabled;
	wyBool		m_ispqashowprofile;
	wyBool		m_pqaprofileenabled; // is Profiler option enabled or not (preference)
	wyInt32		m_pqaprofilerbuffsize; //profiler buf size to reset to original
		    
    CRITICAL_SECTION m_csglobal;
    wyBool      m_issshchecked;
	CRITICAL_SECTION m_csiniglobal;	
	CRITICAL_SECTION m_cssshglobal;	
	

	//Using Find dialog, how many times the selected text is found 
	wyInt32		m_findcount;

	// Explit path for .ini, tags, logs etc(For portable SQLyog)
	wyString	m_configdirpath;	

	//Licance name of product
	wyString	m_entlicense;
	   		
	//High limit & low limit
	wyInt32		m_lowlimitglobal;
	wyInt32		m_highlimitglobal;
	
	//Flag sets true when 'Result tab page' option is enabled in preference
	wyBool		m_resuttabpageenabled;

	//result tab page option
	wyBool		m_retainpagevalue;

	//Application uuid to send request
	wyString	m_appuuid;

	wyString    m_menutableengine;

	wyFile		m_plinklockfile;

	//original frame window procedure
	WNDPROC     m_wmnextproc;

	//flag to check if WM_MDINEXT message is custom or windows default
	wyBool		m_iscustomwmnext;
	
	wyWChar		*m_menurefreshobj;
	wyWChar		*m_menucurrentquery;
	wyWChar		*m_menuselectedquery;
	wyWChar		*m_menuallquery;

	wyBool		m_isdbsearchtab;
    HIMAGELIST  m_hiconlist;

    wyBool      m_istabledataunderquery;
    wyBool      m_isinfotabunderquery;
    wyBool      m_ishistoryunderquery;
	wyInt32		m_prefpersist;
	wyBool		m_conrestore;
	wyBool		m_isannouncementopen;
	wyString	m_announcementshtml;
	wyString	m_regname;
	MDIWindow	*m_pcquerywnd;
	Announcements	*m_announcements;
	List		*m_mdiwlist;
	wySQLite			*m_sqliteobj;
	wyBool		m_issessionsaveactive;
	HANDLE		m_sessionsavemutex;
	wyBool		m_sessionrestore;
	wyString	m_database;
	wyString	m_sessionbackup;
	List		*m_connectiontabnamelist;
	List		*m_mdilistfordropdown;

} GLOBALS, *PGLOBALS;


/*! \struct tagConnDlgParam
    \brief Contains connection dialog details
    \param wyChar      m_connname[128];			// Connection name
	\param wyBool      m_isnew;					// Is the connection new ?
    
*/
typedef struct tagConnDlgParam
{
	wyWChar     m_connname[128];
	wyBool      m_isnew;
}
CONNDLGPARAM, *PCONNDLGPARAM;


/*! \struct tagGridScrollInfo
    \brief Contains information about grid scrolling position
    \param LONG		m_initrow;					// Initial row
	\param LONG		m_initcol;					// Initial column
	\param LONG		m_selrow;					// Selected row
	\param LONG		m_selcol;					// Selected column

*/
typedef struct tagGridScrollInfo
{
	LONG		m_initrow;
	LONG		m_initcol;
	LONG		m_selrow;
	LONG		m_selcol;

} GRIDSCROLLINFO, *PGRIDSCROLLINFO;


/*! \struct st_mysql_rowsex
    \brief Contains extended information about rows
    \param struct st_mysql_rowsex  *m_next;			// list of rows 
	\param MYSQL_ROW				m_data;			// Mysql row data
	\param wyBool					m_ismysql;		// from mysql or user created 
	\param wyBool					m_newrow;		// its a new row user correct updating or new insert
	\param wyBool					m_ischecked;	// Is the row selected ?
	
*/
typedef struct st_mysql_rowsex 
{
  struct st_mysql_rowsex *m_next;		
  MYSQL_ROW     m_data;
  wyBool		m_ismysql;				
  wyBool		m_newrow;				
  wyBool		m_ischecked;
} MYSQL_ROWSEX;


/*! \struct st_mysql_dataex
    \brief Contains extended information about mysql data
    \param MYSQL_ROWSEX		*m_data;				// Extended rows information
	\param MYSQL_ROW		 m_origdata;			// Rows original information

*/
typedef struct st_mysql_dataex 
{
  MYSQL_ROWSEX	*m_data;
  MYSQL_ROW		m_origdata;
} MYSQL_DATAEX;


/*! \struct st_mysql_tabledataex
    \brief Contains extended information about table data
    \param MYSQL_DATAEX		*m_data;				// Mysql extended data information
	\param wyChar           **m_fields;				// Pointer to fields
	\param wyInt32			m_fieldcount;			// Number of fields

*/
typedef struct st_mysql_tabledataex 
{
	MYSQL_DATAEX	*m_data;
	wyChar          **m_fields;
	wyInt32         m_fieldcount;
} MYSQL_TABLEDATAEX;


/*! \struct tagFreeTableDataRes
    \brief Contains all the information to displayed on to the TABLE tab
    \param MYSQL_TABLEDATAEX	*m_table_data;		// Extended information about table data
	\param MYSQL_RES			*m_fieldres;		// Field info of the table
	\param MYSQL_RES			*m_keyres;			// Index info of the table
	\param HANDLE				 m_hevent;			// Thread HANDLE to free up the resources
	\param Tunnel*				 m_tunnel;			// Tunnel pointer

*/
typedef struct tagFreeTableDataRes
{
	MYSQL_TABLEDATAEX	*m_table_data;
	MYSQL_RES			*m_fieldres;
	MYSQL_RES			*m_keyres;
	HANDLE				m_hevent;
	Tunnel*				m_tunnel;
	
}TABLEDATARES;
 

/*! \struct tagTemplateValue
    \brief This structure is for the template key and its caption.
		   This is used only when a user wants the template dialog box.
    \param wyInt32	    m_idvalue;						// Template
	\param wyChar		m_caption[256];					// Template text
	\param wyInt32		m_type;							// Advance or normal editor

*/
typedef struct tagTemplateValue
{
	wyInt32	    m_idvalue;
	wyWChar     m_caption[256];
	wyInt32     m_type;
}
TEMPLATEVALUE, *PTEMPLATEVALUE;


/*! \struct tagPermissions
    \brief structure which holds various permission which has been granted.
    \param wyBool	m_isselect;							// Select permission ?
	\param wyBool	m_isinsert;							// Insert permission ?
	\param wyBool	m_isupdate;							// Update permission ?
	\param wyBool	m_isdelete;							// Delete permission ?
	\param wyBool	m_iscreate;							// Create permission ?
	\param wyBool	m_isdrop;							// Drop permission	?
	\param wyBool	m_isgrant;							// Grant permission ?
	\param wyBool	m_isreferences;						// References permission ?
	\param wyBool	m_isindex;							// Index permission	?
	\param wyBool	m_isalter;							// Alter permission	?
	\param wyChar  *m_db;								// Database name
	\param wyChar  *m_table;							// Table name
	\param wyChar  *m_column;							// Column name
*/

typedef struct tagPermissions
{
	wyBool	m_isselect;
	wyBool	m_isinsert;
	wyBool	m_isupdate;
	wyBool	m_isdelete;
	wyBool	m_iscreate;
	wyBool	m_isdrop;
	wyBool	m_isgrant;
	wyBool	m_isreferences;
	wyBool	m_isindex;
	wyBool	m_isalter;

    wyBool	m_iscreatetmptable;
    wyBool	m_islocktable;

    wyBool	m_iscreateview;
    wyBool	m_isshowview;
    wyBool	m_iscreateroutine;
    wyBool	m_isalterroutine;
	wyBool	m_isexecute;

    wyBool	m_istrigger;
    wyBool	m_isevent;

	wyBool	m_isproc;
	
	wyChar	*m_db;
	wyChar  *m_table;
	wyChar  *m_column;
	wyChar	*m_proc;
}
PERMISSIONS, *LPPERMISSIONS;



/*! \struct tagPermissionsList
    \brief Its is used to create permission link list
    \param PERMISSIONS				 m_perm;				// Different permissions
	\param tagPermissionsList		*m_next;				// Pointer to tagPermissionsList

*/
typedef struct tagPermissionsList
{
	PERMISSIONS			m_perm;
	tagPermissionsList	*m_next;
}
PERMISSIONSNODE, *LPPERMISSIONSNODE;


/*! \struct tagDiffComboItem
    \brief 
    \param wyWChar			szDB[128];						// Database name					
	\param Tunnel		   *tunnel;							// Tunnel pointer
	\param CHAR				szDBs[128];	
	\param PMYSQL			mysql;
	\param PMYSQL			m_mysql;
	\param ConnectionInfo  *info;
*/
typedef struct tagDiffComboItem
{
	MDIWindow           *wnd;
	wyWChar				szDB[128];
	Tunnel				*tunnel;
	PMYSQL				mysql;
	PMYSQL				m_mysql;
	ConnectionInfo		*info;
}
DIFFCOMBOITEM, *LPDIFFCOMBOITEM;


/**
   * An enum.
   * EXECUTEOPTION
	
    SINGLE		0,
	ALL			1,
	SELECTED	2,
	INVALID		3.

*/
enum EXECUTEOPTION
{
	SINGLE,
	ALL,
	SELECTED,
	INVALID
};

//enum RESULTVIEWTYPE
//{
//	GRID_VIEW,
//	FORM_VIEW,
//	TEXT_VIEW
//};

#endif
