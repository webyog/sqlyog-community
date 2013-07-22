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

#include "EditorFont.h"
#include "wyString.h" 
#include "Global.h"
#include "Tunnel.h"
#include "MySQLVersionHelper.h"

class MySQLError
{

public:

/// Default constructor.
/**
* Initializes the member variables with the default values.
*/
MySQLError(wyString* pkeywords = NULL, wyString* pfunctions = NULL);

/// Shows the current MYSQL error
/**
@param hdlg          : IN Dialog window handle
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Mysql pointer
@param query		 : IN SQL Query
@param val			 : Dialog Box or Message Box
@returns void
*/
void Show(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query = NULL, wyBool val = wyFalse, MYSQL_RES *reswarning = NULL);

void Show(HWND hdlg, const wyChar* query, wyInt32 mysqlerrorno, const wyChar* mysqlerror);

///Handles MySQL error
/**
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Mysql pointer
@param err           : OUT gets the error to display
*/
wyInt32 HandleMySQLError(Tunnel * tunnel, PMYSQL mysql, wyString *err);	

///Handles MySQL warnings
/**
@param tunnel        : IN Tunnel pointer
@param mysql         : IN Mysql pointer
@param reswarning    : IN mysql warnings
@return void
*/
void HandleMySQLWarnings(Tunnel * tunnel, PMYSQL mysql, MYSQL_RES *reswarning);	

/// Shows the current error with query
/**
@param hdlg          : IN Dialog window handle
@param query		 : IN SQL Query
@param errmsg		 : error message
@returns void
*/
void	ShowDirectErrMsg(HWND hdlg, const wyChar * query , const wyChar * errmsg);

private:

// Tunnel pointer
Tunnel				*m_tunnel;

// Mysql pointer
PMYSQL				m_mysql;

// SQL Querry
const wyChar		*m_query;

// MySQL Error Number
wyInt32				m_mysqlerrno;

//MySQL Error Message
wyString			m_mysqlerrmsg;

//Handle to the focused window
HWND				m_hwnd;

//dialog handle
HWND				m_hwnddlg;

/// Original query window procedure
WNDPROC             m_wporigquerytextproc ;

/// Original err window procedure
WNDPROC             m_wporigerrtextproc ;

//Flag sets wyTrue if its warning, sets wyFalse for error
wyBool				m_iswarning;

wyString*           m_pkeywords;

wyString*           m_pfunctions;

/// Creates MySQL Error Dialog Box
/**
@param hwndparent			: IN Parent Window Handler
@return Void
*/
void CreateErrorDialog(HWND hwndparent);

///Window Procedure for Error Dialog 
/**
@param hwnd                 : IN Window Handler.
@param message              : IN Messages.
@param wparam               : IN Unsigned message parameter.
@param lparam               : IN LPARAM parameter.
*/
static INT_PTR CALLBACK	WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

///Creates a Scintilla Window
/**
@param hDlg					: IN DialogBox handler
*/
void CreateScintillaWindow(HWND hDlg);

/// Writes the MySQL Error into the Text File
/** 
@param hwnd					: IN Window Handler
@returns void
*/
wyBool WriteToTextfile(HWND hwnd);

///Sets the error message editor modes
/**
@param hwnderror		: IN handle to error message
@return void
*/
void	SetErrMsgEditorModes(HWND hwnderror);

///window procedure for the query text editor(scintilla)
/**
@param hwnd				: Handle to the dialog box. 
@param message			: Specifies the message. 
@param wparam			: Specifies additional message-specific information. 
@param lparam			: Specifies additional message-specific information. 
@returns 1 or 0
*/
static	LRESULT	CALLBACK    QueryTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

///window procedure for the error string editor(scintilla)
/**
@param hwnd				: Handle to the dialog box. 
@param message			: Specifies the message. 
@param wparam			: Specifies additional message-specific information. 
@param lparam			: Specifies additional message-specific information. 
@returns 1 or 0
*/
static	LRESULT	CALLBACK    ErrTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

};
