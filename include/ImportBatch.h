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
#include "ImportFromSQL.h"

#ifndef _IMPORTBATCH_H_
#define _IMPORTBATCH_H_

class ImportFromSQL;

/*! \struct __import_longparam
    \brief Used to store import information
    \param HWND hwndMsg MessageBox handle
    \param HWND hwndProgress Progress bar handle
	\param HWND	hwndSize 
	\param wyUInt32	totalfilesize Total file size 
*/
struct __import_longparam
{
	HWND		m_hwndMsg;
	HWND		m_hwndProgress;
	HWND		m_hwndSize;
    wyBool      m_isTunnel;
	wyInt64		m_totalfilesize;
	wyUInt32	m_count;
};

/*! Creates a type import information */ 
typedef struct	__import_longparam IMPORTPARAM;


/*! \struct __import_longparam
    \brief Structure for thread sync in importbatch. 
    This structure pointer is sent as a long parameter in the thread for importbatch
    \param HANDLE impevent Handle to the import event
    \param ImportFromSQL *import ImportFromSQL object pointer
	\param wyUInt32 num_bytes Number of total bytes
	\param wyUInt32 num_queries Number of queries
    \param wyBool *stopquery Stop flag
    \param IMPORTERROR retcode IMPORTERROR enum value
    \param IMPORTPARAM *lpParam __import_longparam pointer
*/
struct __stimportbatch
{
	HANDLE          m_impevent;
    ImportFromSQL      *m_import;
	wyUInt32        m_num_bytes;
	wyUInt32        m_num_queries;
	wyBool			*m_stopquery;
	IMPORTERROR     m_retcode;
	IMPORTPARAM     *m_lpParam;
	MDIWindow		*wnd;
};

/*! Creates a type import batch */ 
typedef struct	__stimportbatch IMPORTBATCH;


class ImportBatch
{
public:
	
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
    ImportBatch ();

     /// Destructor
    /**
    Free up all the allocated resources
    */
	~ImportBatch();

    /// Creates the import process
    /** This procedure is used to invoke the import procedure.
    @param hwnd Window handle.
    @param tunnel   Current tunnel pointer
    @param mysql Current mysql pointer.
    @returns wyBool
    */
    wyBool	Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql);

private:

    /// Parent window handle
    HWND		m_hwndparent;

    /// Window handle
    HWND        m_hwnd;
    
	/// Edit control handle
    HWND        m_hwndedit;
    
	/// Font handle
	HFONT		m_hfont;
    
	/// MySQL pointer
	PMYSQL		m_umysql;
    
	/// Tunnel pointer
	Tunnel		*m_tunnel;
    
	/// Current Database name
	wyChar		m_curdb[128];
    
	/// value to stop the thread
	wyBool		m_stopimport;
    
	/// values to keep the state of threads
	wyBool		m_importing;
    
	/// persistence pointer
	Persist		*m_p;


    /// The dialog procedure import GUI.
    /** The procedure that is used by the import thread.
    */
	static INT_PTR CALLBACK DlgProc(HWND hwnd,wyUInt32 message, WPARAM wParam, LPARAM lParam);

    /// Initialize the Import
    /** 
    @param wparam		: IN handle to the window
    */
    void InitilizeExportData(HWND hwnd);

    /// Handle WM_COMMAND message in DlgProc.
    /** 
    @param wparam		: IN 
    */
    void OnWMCommand(HWND hwnd, WPARAM wparam);

	///Import process
	/**
	@return void
	*/
	void ImportDump();

    /// Handle WM_MOUSEMOVE message in DlgProc.
    /** 
    @param hwnd			: IN handle to the window
    @param lparam		: IN lparam
    */
    void OnWMMouseMove(HWND hwnd, LPARAM lparam);

    /// invokes the importing process
    /** 
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
    wyBool  ExecuteBatch();

    /// Shows zero file size error
    /** 
    @param param		: IN IMPORTPARAM structure
    @returns void
    */
    void ShowEmptyFileError(IMPORTPARAM * param);

    /// concludes the Import process
    /** 
    @param evt			: IN IMPORTBATCH structure
    @returns void
    */
    void ImportConclude(IMPORTBATCH * evt, IMPORTPARAM * param);

    /// Selects the the exported file to import 
    /** This opens the openfile dialog to select the export file to import.
    @returns void
    */
	void	SetExpFileName();

    /// Thread procedure for import
    /** The procedure that is used by the import thread.
    @param lpParam		: IN/OUT structure to communicate with the thread procedure.
    @returns unsigned int always zero.
    */
	static unsigned __stdcall ImportThread(LPVOID lpParam);

    /// Procedure that is used to update the GUI messages while importing.
    /** This is used to show the messages like how many queries executed and how many bytes read.
    @param lpparam		: IN IMPORTPARAM pointer.
    @param num_bytes	: IN Number of bytes read. 
    @param num_queries	: IN Number of queries executed.
    @returns void
    */
	static void UpdateGui(void * lpparam, wyInt64 num_bytes, wyInt64 num_queries);
    
    /// checks whether the wquery is empty(contains spaces only) or not.
    /** Parses the query to check whether it contains non space chars
    @param query		: IN query to parse.
    @returns wyBool wyTrue if it contains spaces only, otherwise wyFalse.
    */
	wyBool	IsQueryEmpty(const wyChar * query);

    /// Shows the current database to user
    /** Selects the current database in the message box.
     @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool	GetCurDatabase();


    /// Changes the font to BOLD
    /** Changes the font to BOLD of the current database
     @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool	SetCurDbFont();

    /// Used to show the error message.
    /** 
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool	HandleError();	

    /// Initializes the import values
    /** It initializes the ImportFromSQL object with all the values like wconnection info...
    @param import		: IN ImportFromSQL object pointer
    @returns void
    */
	void    SetImportValues(ImportFromSQL * import);

    /// Initializes the import handle values
    /** It initializes the handle values for import
    @param param		: IN IMPORTPARAM structure
    @returns void
    */
	void    SetParamValues(IMPORTPARAM * param);

    /// Enable/Disable dialog windows
    /** Enable/Disable dialog windows like buttons...
    @param enable: IN wyTrue/wyFalse
    @returns void
    */
	void    EnableDlgWindows(wyBool enable);

    /// Changes the OK button text.
    /** Used to change the OK button text when required.
    @returns void
    */
	void    ChangeOKButtonText(wyWChar * text);

    /// make visible or not the progressbar accordingly
    /** 
    @param show: IN wyTrue/wyFalse
    @returns void
    */
	void    ShowProgressBarOrStatic(wyBool show);

    /// Update the finish message
    /** 
    @param param: IN IMPORTPARAM structure
    @returns void
    */
	void    UpdateFinishValues(IMPORTPARAM * param);

    /// initializes the progress bar.
    /** It calculates the progressbar by calculating the import file size
    @returns wyUInt32 the size of the file.
    */
	wyInt64    SetInitProgressValues();
};


class CImportError
{

public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	CImportError();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~CImportError();

    /// Creates the Error Message dialogbox
    /** It calculates the progressbar by calculating the import file size
    @param hwndparent		: IN Parent window handle
    @param message			: IN Message to be displayed
    @param errfile			: IN Error file name where the error description is to be stored.
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool Create(HWND hwndparent, const wyChar * message, const wyWChar *errfile);

private:

    /// Tunnel pointer
	Tunnel      *m_tunnel;
    
	/// MySQL pointer
	MYSQL       *m_mysql;
    
	/// Error message
	const wyChar *m_message;
    
	/// Error file name
    const wyWChar *m_errfile;

    /// Window procedure for the Error message dialog box
    /** 
    */
    static INT_PTR CALLBACK WndProc(HWND phwnd, UINT pmessage, WPARAM pwparam, LPARAM plparam);

    /// Handle WM_COMMAND message in DlgProc.
    /** 
    @param wparam: IN 
    */
    void OnWMCommand(HWND hwnd, WPARAM wparam);
};

#endif
