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

#include "DialogPersistence.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "SQLMaker.h"
#include "TabQueryTypes.h"   
#include "DoubleBuffer.h"

#ifndef _EXPIMP_H
#define _EXPIMP_H

class MySQLDataEx;

struct __excelexportdata
{
    HANDLE          m_filename;
    MYSQL_RES       *m_result;
    MYSQL_FIELD     *m_fields;
	MySQLDataEx	    *m_tabrec;
    Tunnel          *m_tunnel;
    wyString        m_textlimit;
    wyString        m_decimal;
    HWND            m_hwndmessage;
    HWND            m_hwnd;
    wyBool          *m_stopped;
	wyBool			*m_selcol;
	wyBool			m_resultfromquery;

};
typedef struct __excelexportdata ExportExcelData;

class CExportResultSet;
class MySQLDataEx;

/*! \struct TableAdvPropValues
	\brief Structure used by CTableAdvprop to get/set table advance properties

	HANDLE              m_exportevent				Export event HANDLE
    CExportResultSet    *m_exportresultset			ExportResultSet Object
	wyInt32			    *m_exportstop				Flag for stop exporting

*/
struct __stopexport
{
    HANDLE              m_exportevent;
    CExportResultSet    *m_exportresultset;
	wyInt32			    *m_exportstop;
};
typedef struct __stopexport StopExport;


class ExportCsv
{
public:
	ExportCsv();
	~ExportCsv();

	/// Function to get the escaping characters.
	/**
	@returns wyTrue
	*/
	wyBool  GetEscapeCharacters	();

	/// Function to get independent characters outside the class.
	/**
	@param hwnd			: IN Window HANDLE
	@returns wyTrue
	*/
	wyBool  GetEscapeCharacters(HWND hwnd); 

	/// Function process the escaping character and formats it properly.
	/**
	@param buffer		: IN/OUT Text to escape
	@returns wyTrue
	*/
	wyBool  ProcessEscChar(wyChar *buffer);

	/// Writes data to the buffer when we have to write for fixed length data.
	/**
	@param filebuffer	: IN/OUT filebuffer
	@param text			: IN text to write
	@param field		: IN Field information
	@param length		: IN Length of the string
	@param isescaped	: IN Is escaped	?
	@param fterm		: IN FIELD TERMINATE 
	@param isfterm		: IN IS FIELD TERMINATE BY selected ?
	@param lterm		: IN Line terminate
	@param islterm		: IN IS LINE TERMINATE BY selected ?
	@param encl			: IN FIELDS ENCLOSED BY
	@param isencl		: IN FIELDS ENCLOSED BY selected ?
	@returns wyTrue on successful write to file else wyFalse
	*/
	wyBool  WriteFixed(wyString * filebuffer, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, wyBool isescaped, wyChar fterm, wyBool isfterm, wyChar lterm, wyBool isLterm, wyChar encl, wyBool isencl);

	/// Writes data of variable length to the buffer.
	/**
	@param filebuffer	: IN/OUT filebuffer
	@param text			: IN text to write
	@param field		: IN Field information
	@param length		: IN Length of the string
	@param isescaped	: IN Is escaped	?
	@param fterm		: IN FIELD TERMINATE 
	@param isfterm		: IN IS FIELD TERMINATE BY selected ?
	@param lterm		: IN Line terminate
	@param islterm		: IN IS LINE TERMINATE BY selected ?
	@param encl			: IN FIELDS ENCLOSED BY
	@param isencl		: IN FIELDS ENCLOSED BY selected ?
	@returns wyTrue on successful write to file else wyFalse
	*/
	wyBool  WriteVarible(wyString * filebuffer, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, wyBool isescaped, wyChar fterm, wyBool isfterm, wyChar lterm, wyBool isLterm, wyChar encl, wyBool isencl);

	// Function to get the default escaping characrers depending upon the various
	// options selected by the user.
	/**
	@param fterm		: IN/OUT FIELD TERMINATE 
	@param isfterm		: IN/OUT IS FIELD TERMINATE BY selected ?
	@param lterm		: IN/OUT Line terminate
	@param islterm		: IN/OUT IS LINE TERMINATE BY selected ?
	@param encl			: IN/OUT FIELDS ENCLOSED BY
	@param isencl		: IN/OUT FIELDS ENCLOSED BY selected ?
	@returns wyTrue
	*/ 
	wyBool  GetDefEscChar(wyChar *fterm, wyBool *isfterm, wyChar *lterm, wyBool *islterm, wyChar *encl, wyBool *isencl);

	/// Escape char details
	ESCAPECHAR	m_esch;
	
	/// Window HNADLE
         HWND    m_hwnd;

};

class ExportMultiFormat
{
public:
	ExportMultiFormat();
	~ExportMultiFormat();

	/// Callback function for export multi format dialog box
	/**
	@param hwnd			: IN Window handle
	@param messgae		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	/// Callback function for export multi format dialog box
	/**
	@param hwnd			: IN Window handle
	@param messgae		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProcXML(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Initializes the values for the export multi format dialog
	/**
	@param hwnd				: IN Window HANDLE
	@returns void
	*/
    void    OnWmInitDlgValues(HWND hwnd);

	/// Initializes the values for the export multi format dialog
	/**
	@param hwnd				: IN Window HANDLE
	@returns void
	*/
    void    OnWmInitDlgValuesXML(HWND hwnd);

	/// Command handler for export multi format 
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN Unsigned message parameter
	@returns wyTrue on success else wyFalse
	*/
    wyBool  OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Command handler for export multi format 
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN Unsigned message parameter
	@returns wyTrue on success else wyFalse
	*/
    wyBool  OnWmCommandXML(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Main dialog creator for export multi format
	/**
	@param hwndparent		: IN Parent window HANDLE
	@param db				: IN Database name
	@param table			: IN Table name
	@param tunnel			: Tunnel pointer
	@param mysql			: Pointer to mysql pointer
	@returns wyTrue on success else wyFalse
	*/
	wyBool	Create(HWND hwnd, CHAR * db, CHAR * table, Tunnel * tunnel, PMYSQL mysql);
	
	/// Main dialog creator for IMPORT from XML
	/**
	@param hwndparent		: IN Parent window HANDLE
	@param db				: IN Database name
	@param table			: IN Table name
	@param tunnel			: Tunnel pointer
	@param mysql			: Pointer to mysql pointer
	@returns wyTrue on success else wyFalse
	*/
	wyBool	CreateXML(HWND hwnd, CHAR * db, CHAR * table, Tunnel * tunnel, PMYSQL mysql);

	/// Gets the initial data  for the dialog
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	FillInitData();
	
	/// Setting the charset combo box
	/**
	@return VOID
	*/
	VOID	SetComboText();
	
	/// Fills the listbox with the fields. 
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	FillFields();

	/// Creates dialog for the user to select the file name for export
	/**
	@param iscsv  :whether import from csv or XML
	@returns wyTrue on success else wyFalse
	*/
	wyBool	SetExpFileName(wyBool iscsv=wyTrue);

	/// Function to get the escaping characters.
	/**
	@returns wyTrue
	*/
	wyBool	GetEscapeCharacters	();

	/// Function where the import data starts
	/**
	@param iscsv  :whether import from csv or XML
	@returns wyTrue on success else wyFalse
	*/
	wyBool	ImportData(wyBool iscsv=wyTrue);

	/// Gets back the file name from the dialog
	/**
	@param filename		: OUT the retrieved filename
	@returns wyTrue
	*/
	wyBool	GetFileName(wyString &filename);

	/// Function sets the values of various static edit box depending upon the value of the various flags.
	/**
	@returns wyTrue
	*/
	wyBool	SetStaticText();
	
	/// Function to prepare query depending upon various options selected by user.
	/**
	@param query		: IN/OUT Query string
	@returns the length of the query
	*/
	wyInt32 PrepareQuery(wyString &query);

	/// Function to prepare query depending upon various options selected by user.
	/**
	@param query		: IN/OUT Query string
	@returns the length of the query
	*/
	wyInt32 PrepareQueryXML(wyString &query);

	/// Escape char details
	ESCAPECHAR	m_esch;

	/// Parent window HANDLE
	HWND	    m_hwndparent;

	/// Window HANDLE
    HWND        m_hwnd;

	/// Edit window HANDLE
    HWND        m_hwndedit;
	
	/// HANDLE to table list
    HWND        m_hwndtlist;
	
	/// Handle to field list
    HWND        m_hwndfieldlist;
    
	/// Database name
	wyString    m_db;

	/// Table name
    wyString    m_table;	

	/// Charset selected for the selected file
	wyString m_importfilecharset;

	/// Pointer to mysql pointer
	PMYSQL	    m_mysql;

	/// Tunnel pointer
	Tunnel	    *m_tunnel;

	/// Persist type pointer
	Persist	    *m_p;

	/// Persist type pointer for xml import
	Persist	    *m_p_XML;
};

class EscapeChar
{
public:

	EscapeChar();
	~EscapeChar();

	/// Callback function for escape char dialog box
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Initializes the values for the escape char dialog
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
    void    OnWmInitDlgvalues(HWND hwnd);

	/// Command handler for escape char dialog
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN Unsigned message parameter
	@returns wyTrue on success else wyFalse
	*/
    void    OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

	// Enables/Disables options in the escape char dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param enable		: IN Enabled ?
	*/
    void    EnableOptions(HWND hwnd, wyInt32 enable);

	/// Creates the dialog for the escape char dialog box
	/**
	@param hwnd			: IN Window HANDLE
	@param pesc			: IN Pointer to escape char
	@param type			: IN Type fo escape char
	@param choice		: IN Fields in the escape char dialog
	@returns wyTrue on success else wyFalse
	*/
	wyBool      GetEscapeChar(HWND hwnd, PESCAPECHAR pesc, wyBool type, wyBool choice);

	/// The function is called when the ok button is pressed on the escape char dialog
	/**
	@returns wyTrue
	*/
	wyBool      ProcessOK();

	/// Function to set the initial values in the edit box when the user presses the button two times.
	/**
	@returns wyTrue
	*/
	wyBool      SetEscapeChar();

	/// Sets the excel type escape chars
	/**
	@param wyTrue		: IN Copy from clipboard or not
	*/
	void		SetExcelEscapeChar(wyBool);

	/// Parent window HANDLE
	HWND		 m_hwndparent;

	/// Window HANDLE
    HWND         m_hwnd;
	
	/// Type of escapechar
	wyBool       m_type;
	
	/// Field names selected ?
    wyBool       m_fieldnames;

	/// Pointer to escape chars details
	PESCAPECHAR	 m_pesc;
};


class BackUp
{
public:
	
	BackUp();
	~BackUp();

	/// Callback function for backup database dialog box
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Initializes the values for the backup database dialog
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
    void    OnWmInitDlgvalues(HWND hwnd);

	/// Command handler for backup dialog
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN Unsigned message parameter
	@returns wyTrue on success else wyFalse
	*/
    wyBool  OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Function is called when an error occurs in backup
	/**
	@returns wyFalse
	*/
    wyBool  OnError();

	/// Main dialog creator for backup database
	/**
	@param hwndparent		: IN Parent window HANDLE
	@param mysql			: IN Pointer to mysql pointer
	@param db				: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	Create(HWND hwndParent, Tunnel * tunnel, PMYSQL mysql, wyChar *db);

	/// Gets the directory name where the backup file is to be saved.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	GetDirectoryName();

	/// Main function for the backups table.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	BackUpTable();

	/// Function fills the listbox with the tables of the selected database.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	FillListBox();

	/// Function first does read lock for all the tables.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	LockTables();

	/// Function first does read lock for all the tables.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	UnlockTables();

	// Function prepares the query and returns the buffer, depending upon various tables selected.
	/**
	@param query		: IN/OUT Query string
	@returns length of the query
	*/
	wyInt32 PrepareQuery(wyString &query);

	/// Parent window HANDLE
	HWND	 m_hwndparent;

	/// Window HANDLE
    HWND     m_hwnd;

	/// Table list HANDLE
    HWND     m_hwndtablelist;

	/// Directory name dialog HANDLE
    HWND     m_hwnddirname;

	/// Database name
    wyString m_db;

	/// Directory name
    wyString m_dir;
	
	/// Pointer to mysql pointer
	PMYSQL	 m_mysql;

	/// Tunnel pointer
	Tunnel	 *m_tunnel;
	
	/// Persistence class object pointer
	Persist	*m_p;
};

class CRestore
{
public:
	
	CRestore();
	~CRestore();

	/// Callback function for restore database dialog box
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Initializes the values for the restore database dialog
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
    void    OnWmInitDlgValues(HWND hwnd);

	/// Command handler for restore dialog
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN Unsigned message parameter
	@returns wyTrue on success else wyFalse
	*/
    wyBool  OnWmCommand(HWND hwnd, WPARAM wparam);

	/// Main dialog creator for restore database
	/**
	@param hwndparent		: IN Parent window HANDLE
	@param mysql			: IN Pointer to mysql pointer
	@param db				: IN Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool	Create(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql, wyChar *db);

	/// Gets the directory name where the backup file is saved.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	GetDirectoryName();

	/// Main function for restoring the table.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	RestoreTable();

	/// Function fills the listbox with the tables of the selected database.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	FillListBox();

	/// Function finds and adds .MYD files from the current directory and adds them to the listbox.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool	FindAndAddFiles();

	// Function prepares the query and returns the buffer, depending upon various tables selected.
	/**
	@param query		: IN/OUT Query string
	@returns query length
	*/
	wyInt32 PrepareQuery(wyString &query);

	/// Parent window HANDLE
	HWND	    m_hwndparent;

	/// Window HANDLE
    HWND        m_hwnd;

	/// HNADLE to table list
    HWND        m_hwndtablelist;

	/// HANDLE to get directory name dialog
    HWND        m_hwnddirname;

	/// Database name
	wyString    m_db;

	/// Directory name
    wyString    m_dir;
	
	/// POinter to mysql pointer
	PMYSQL	    m_mysql;
	
	/// Tunnel pointer
	Tunnel	    *m_tunnel;
	
	/// Persistence pointer
	Persist	    *m_p;
};

class CShowInfo
{
public:

	CShowInfo();
	~CShowInfo();

	/// Callback function for show info dialog box
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static	INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	/// Initializes the values for the show info dialog
	/**
	@param hwnd				: IN Window HANDLE
	@returns void
	*/
    void    OnInitDlgvalues(HWND hwnd);
	
	/// Creates dialog for showing information
	/**
	@param hwndParent		: IN Parent window HANDLE
	@param tunnel			: IN Tunnel pointer
	@param myres			: IN Mysql result set
	@param title			: IN Caption for the dialog
	@returns non-zero on success else zero
	*/
	wyBool	ShowInfo(HWND hwndParent, Tunnel * tunnel, MYSQL_RES *  myres, wyChar *title, wyChar *summary = NULL);

	/// Handles WM_CTLCOLORSTATIC for the dialog
    /**
    @param hwnd			: IN dialog handle
    @param wParam		: IN window procedure WPARAM
    @param lParam		: IN window procedure LPARAM
    @returns LRESULT 1
    */
	wyInt32	OnCtlColorStatic(WPARAM wParam, LPARAM lparam);

	/// Sets the font for the window
	/**
	@returns wyTrue
	*/
	wyBool	SetFont();

	///Resizes the dialog
	/**
	@param hwnd			: IN dialog handle
	@returns void
	*/
	void	ShowInfoResize(HWND hwnd);
	
	/// Title string for the window
	wyString        m_title;

	/// Window HANDLE
	HWND	        m_hwnd;
	
	/// PArent window  HANDLE
	HWND            m_hwndparent;

	/// Editor window HANDLE
    HWND            m_hwndedit;

	/// Font HANDLE
	HFONT	        m_hfont;

	/// Tunnel pointer
	Tunnel			*m_tunnel;

	/// Mysql result set
	MYSQL_RES		*m_res;

	/// Mysql Fields
	MYSQL_FIELD		*m_field;

	//Summary
	wyString        m_summary;


};

class CExportResultSet
{
public:

	CExportResultSet();
	~CExportResultSet();

	/// Command handler for export result set dialog
	/**
    @param hwnd			: IN Handle to the dialog box. 
    @param message		: IN Specifies the message. 
    @param wparam		: IN Unsigned window parameter
    @param lparam		: IN Long message pointer.
	@returns wyTrue
	*/
	static		INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Initializes the values for export result set
	/**
	@param hwnd			: IN Window HNADLE
	@returns void
	*/
    void        OnWmInitDlgValues(HWND hwnd);

	/// Command handler for export result set dialog
	/**
    @param hwnd			: IN Handle to the dialog box. 
    @param wparam		: IN Unsigned window parameter
    @param lparam		: IN Long message pointer.
	@returns wyTrue
	*/
    wyBool      OnWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Creates the export result dialog box
	/**
    @param hwndparent       : IN Parent window handle
	@param ptr				: IN Record tab details
    @param data             : IN Exporting information
	*/
	wyBool		Export(HWND hwndparent, MySQLDataEx *ptr, wyBool fromquery = wyFalse, wyBool isenablesqlexport = wyTrue);

    /// Retrives the table rows
    /**
    */
    wyBool      GetTableData();
	
	/// Set the static text for the export result dialog
	/**
	@returns wyTrue
	*/
	wyBool		SetStaticText();

	void		FlashIfInactive(HWND hwnd);

	/// Sets the file in the edit box.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool		SetExpFile();

	/// Function calls the required function depending on the selection made and exports the data.
	/**
	@returns wyTrue on success else wyFalse
	*/
	static	unsigned __stdcall		ExportData(LPVOID lpparam);

	/// This function add the field names in the top of file to be used with ignore # lines option.
	/**
	@param buffer		: IN/OUT String
	@param fescape		: IN Fields escapes
	@param lescape		: IN Line escapes
	@returns wyTrue
	*/
	wyBool		AddColumnName(wyString * buffer, wyChar *fescape, wyChar *lescape);

	/// This function implements working of Save As CSV option in the import/export menu.
	/**
	@param hfile		: INFile HANDLE
	@returns wyTrue on success  else wyFalse
	*/
	wyBool		SaveDataAsCSV(HANDLE hfile);

	/// This function implements working of Save As XML option in the import/export menu.
	/**
	@param hfile		: IN File HANDLE
	@returns wyTrue on success  else wyFalse
	*/
	wyBool		SaveDataAsXML(HANDLE hfile);
	/// This function implements working of Save As JSON option in the import/export menu.
	/**
	@param hfile		: IN File HANDLE
	@returns wyTrue on success  else wyFalse
		*/
	wyBool     SaveDataAsJson(HANDLE hfile);

	/// This function implements working of Save As Excel option in the import/export menu.
	/**
	@param multisheet	: IN Multi resultset in multisheet or not
	@param textlimit	: IN Limits for all kind of text
	@param decimalplaces: IN Number of decimal places
	@returns wyTrue if success else wyFalse
	*/
    wyBool      SaveDataAsEXCEL(wyBool multisheet, wyWChar *textlimit, wyWChar *decimalplaces);

	/// This function implements working of Save As HTML option in the import/export menu.
	/**
	@param hfile		: IN File HANDLE
	@returns wyTrue on success  else wyFalse
	*/
	wyBool		SaveDataAsHTML(HANDLE hfile); 

	/// Retrieves all the column lengths in an array
	/**
	@param row			: IN Mysql row
	@param col			: IN Number of columns
	@param len			: IN length of array
	@returns void
	*/
    void        GetColLengthArray(MYSQL_ROW row, wyInt32 col, unsigned long *len);

	 /// Starts the export as SQL statement
	/**
	@returns wyTrue on successful export else wyFalse
	*/
	wyBool		SaveDataAsSQL();

	/// Enables the dialog options for excel export
	/**
	@returns wyTrue on success
	*/
    wyBool  EnableExcelOptions();

	/// Disables the dialog options for excel export
	/**
	@returns wyTrue on success
	*/
    wyBool  DisableExcelOptions();

	/// Enables the dialog options for CSV export
	/**
	@returns wyTrue on success
	*/
    wyBool  EnableCSVOptions();

	/// Enables the dialog options for SQL export
	/**
	@returns wyTrue on success
	*/
    wyBool  EnableSQLOptions();
	
	/// Disables the dialog options for SQL export
	/**
	@returns wyTrue on success
	*/
    wyBool  DisableSQLOptions();

	/// Disables the dialog options for CSV export
	/**
	@returns wyTrue on success
	*/
    wyBool  DisableCSVOptions();

	/// Disables all other things in the dialog
	/**
	@returns wyTrue on success
	*/
    wyBool  DisableOtherOptions();

	/// Enables all other things in the dialog
	/**
	@returns wyTrue on success
	*/
    wyBool  EnableOtherOptions();

	/// Enables all the options in the dialogs
	/**
	@returns wyTrue on success
	*/
    wyBool  EnableAll();

	/// Enables all the options in the dialogs
	/**
	@returns wyTrue on success
	*/
    wyBool  DisableAll();

	/// Main function to start exporting
	/**
	@returns wyTrue on success else wyFalse
	*/
    wyBool  StartExport();

	/// Checks for the user cancel
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool  IsStopExport();

	/// Stops the exporting process
	/**
	*/
    VOID    StopExporting();

	/// Changes the file extension according to the option selected
	/**
	*/
    VOID    ChangeFileExtension();

	/// Changes the file extension according to the option selected
	/**
	*/
	void	FillColumnList();
	void	InitExpCodePageCombo(HWND hdlg, wyInt32 ctrl_id);
	/// Adds all the selected column to the array
	wyBool	AddSelColumn();

	/// Starts the export as Excel
	/**
	@param resultset		: IN StopExport struct
	returns wyTrue on successful export
	*/
	wyBool	StartExcelExport(StopExport *resultset);

	/// Starts the export as SQL
	/**
	@param resultset		: IN StopExport struct
	returns wyTrue on successful export
	*/
	wyBool	StartSimpleSQLExport(StopExport *resultset);

	/// Starts the export as CSV
	/**
	@param resultset		: IN StopExport struct
	returns wyTrue on successful export
	*/
	wyBool	StartCSVExport(StopExport *resultset);

	/// Starts the export as HTML
	/**
	@param resultset		: IN StopExport struct
	@param file				: IN File name
	returns wyTrue on successful export
	*/
	wyBool	StartHTMLExport(StopExport *resultset, wyWChar *file);

	/// Starts the export as XML
	/**
	@param resultset		: IN StopExport struct
	@param file				: IN File name
	returns wyTrue on successful export
	*/
	wyBool	StartXMLExport(StopExport *resultset, wyWChar *file);
	
	/// Starts the export as JSON
	/**
	@param resultset		: IN StopExport struct
	@param file				: IN File name
	returns wyTrue on successful export
	*/
	wyBool  StartJSONExport(StopExport *resultset, wyWChar *file);

	
	/// Function to call when the SQL option is checked
	/**
	*/
	void	OnSQLCheck(HWND hwnd);

	/// Function to call when the Excel option is checked
	/**
	*/
	void	OnExcelCheck();

	/// Function to call when the XML option is checked
	/**
	*/
	void	OnXMLCheck();

	/// Function to call when the HTML option is checked
	/**
	*/
	void	OnHTMLCheck();

	/// Function to call when the CSV option is checked
	/**
	*/
	void	OnCSVCheck();

	//this function is caled when JSON option is selected.
	void   OnJSONCheck();

		/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void ExpDatResize(HWND hwnd);

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
			void OnPaint(HWND hwnd);



	/// ExportCsv	class object
	ExportCsv	m_cesv;

	/// Tunnel pointer
	Tunnel		*m_tunnel;

	/// Window HANDLE
	HWND		m_hwnd;

	/// Editor window HANDLE
    HWND        m_hwndedit;

	/// XML Window HANDLE in export dialog
	HWND		m_hwndxml;

    /// XML Window HANDLE in export dialog
	HWND		m_hwndexcel;

	/// HTML Window HANDLE in export dialog
    HWND        m_hwndhtml;

	/// CSV Window HANDLE in export dialog
    HWND        m_hwndcsv;

	/// SQL Window HANDLE in export dialog
	HWND        m_hwndsql;

	//Json window HANDLE in export dialog
	HWND        m_hwndjson;

	/// Font HANDLE
	HFONT		m_hfont;

	/// HANDLE for select all button
	HWND		m_hwndselall;

	/// HANDLE for deselect all button
	HWND		m_hwnddeselall;

	/// HANDLE for column selection list box
	HWND		m_hwndcolsel;

	/// HANDLE for export sql struct only
	HWND		m_hwndstructonly;

	/// HANDLE for export sql data only
	HWND		m_hwnddataonly;

	/// HANDLE for export sql struct and data both
	HWND		m_hwndstructdata;

	/// HANDLE to include version
	HWND		m_hwndversion;

	/// Pointer to keep track of selected fields
	wyBool		*m_selectedfields;

	/// Contains tab record details 
	
    MySQLDataEx *m_ptr;

	/// Mysql result set
	MYSQL_RES	    *m_res;

	/// Mysql fields
	MYSQL_FIELD	    *m_field;

	/// ESCAPECHAR object
	ESCAPECHAR	    m_esch;

	/// Persistence pointer
	Persist		    *m_p;

	/// Persistence pointer
	Persist		    *m_p2;

    /// File Handle
    HANDLE		    m_hfile;

    /// Filename
    wyString        m_filename;

	/// Flag to check for stop button
    wyBool          m_stopexporting;

	/// Flag set when exporting
    wyBool          m_exporting;

	/// The handle to export thread
    HANDLE          m_exportthread;

    HWND            m_hwndchange;

    /// HANDLE for the blob input
    HWND            m_hwndbloblimit; 

	/// HANDLE for the decimal input
    HWND            m_hwnddecimal;

	/// HNADLE for the status message
    HWND            m_hwndmessage;

	/// WPARAM parameter
    WPARAM          m_wparam;

	/// LPARAM parameter
    LPARAM          m_lparam;

	/// The return value from the thread
	wyBool			m_threadret;

	/// Flag for export as SQL struct only
	wyBool			m_structonly;

	/// Flag for export as SQL data only
	wyBool			m_dataonly;

	/// Flag for export as SQL struct and data both
	wyBool			m_structdata;

	///Flag for SQLyog including Version 
	wyBool			m_includeversion;

    /// Flag to check whether the export is initialised from query tab or not
    wyBool          m_fromquerytab;

    /// Flag to check whether the result set was from the data tab or from the executed query
    wyBool          m_resultfromquery;
	
    /// CRITICAL_SECTION
    CRITICAL_SECTION    m_cs;

	/// List to store the dialogbox component values
	List				m_controllist;

	/// To store the original size of the dialog box
	RECT				m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	RECT				m_wndrect;

    wyBool              m_isenablesqlexport;

	HWND				m_hwndcpcombo;

};

class CExportXMLHTML 
{
public:

	CExportXMLHTML();
	~CExportXMLHTML();
	
};


class ExportEXCEL
{

	/// Constructor
    ExportEXCEL();

	/// Destructor();
    ~ExportEXCEL();

private:

	/// Handle to the file
	HANDLE m_hfile;

	/// File name
    wyString filename;

	/// Handle to blob limit
    HWND hwndblob;

	/// Handle to multisheet limit
    HWND hwndmulsheet;

	/// Initializes the values before exporting
	/**
	@param hwnd			: IN HANDLE to the window
	@param resultset	: IN ExportResultSet object
	@returns wyTrue on success else wyFalse
	*/
    wyBool InitValues(HWND hwnd, CExportResultSet *resultset);

public:

	/// Callback window procedure for the dialogs
    static wyInt32 CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
};

#endif _EXPIMP_H
