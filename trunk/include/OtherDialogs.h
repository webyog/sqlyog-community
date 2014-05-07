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


#include "FrameWindowHelper.h"
#include "SQLMaker.h"
#include "DoubleBuffer.h"

#ifndef _CREORDERCOL_
#define _CREORDERCOL_

class CShowWarning
{

public:

	/// Constructor
						CShowWarning();
	/// Destructor
						~CShowWarning();


	/// Shows warning message
	/**
	@param hwnd		: Window HANDLE
	*/
			void		 ShowWarnings(HWND hwnd);

	/// This function handles the window procedure
	/**
	@param phwnd		: Window HANDLE
	@param pmessage		: Message
	@param pwparam		: Unsigned message parameter
	@param plparam		: Long message parameter
	*/
	static	INT_PTR		 CALLBACK WndProc(HWND phwnd, wyUInt32 pmessage, WPARAM pwparam, LPARAM plparam);

	/// This function creates the actual dialog box and also initializes some values of the class
	/**
	@param hwndparent	: Parent window 
	@param tunnel		: Tunnel pointer
	@param umysql		: Mysql pointer to pointer
	@param title		: Title
	@param text			: Text
	*/
			wyBool		 Create(HWND hwndparent, Tunnel *tunnel, PMYSQL umysql,
								const wyWChar *title, const wyWChar *text);

    /// This function handles commands for WndProc
	/**
	@param hwnd			: Window HANDLE
	@param warn 		: Show warning pointer
	@param wparam		: Unsigned message parameter
	*/
	static	wyBool		WndProcCommands(HWND hwnd, CShowWarning *warn, WPARAM wparam);

    /// Initializes values for show warning class
	/**
	@param hwnd			: Window HANDLE
	@param warn 		: Show warning pointer
	@param wparam		: Unsigned message parameter
	*/
	static	wyBool		WndProcInitValues(HWND hwnd, CShowWarning *warn, WPARAM wparam);
	
	/// Tunnel pointer
			Tunnel		*m_tunnel;

	/// MySQL Pointer
			PMYSQL		 m_mysql;

	/// Title
	const	wyWChar		*m_title; 

	/// Text 
	const	wyWChar		*m_text;

};


class EmptyDB
{
public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	EmptyDB();

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~EmptyDB();

	/// Callback dialog procedure for the window.
	/**
	@param hwnd			: Window Handler.
	@param message		: Window message.
	@param wparam		: WPARAM pointer.
	@param lparam		: LPARAM pointer.
	*/
	static	INT_PTR	CALLBACK EmptyDBDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam); 

	/// Function creates the actual dialog box and also initializes some values of the class.
	/**
	@param hwndparent	: Window Handler.
	@param tunnel		: Tunnel pointer.
	@param umysql		: Pointer to mysql.
	@param db			: Database name.
	@returns 1 if success else 0
    */
	wyInt32		Create(HWND hwndparent, Tunnel *tunnel, PMYSQL umysql, wyChar *db);

	/// Initialize the various start thing for the dialog box .
	/**
	*/
	void  EmptyDbDlgInit();

	/// Setting the properties  of Listview Control
	/**
    */
    void InitList();

	/// Fill the listview with all the objects of a database.
	/**
    */
    void FillList();
   
	/// Command handler for EmptyDatabase dialog
	/**
	@param wparam		: IN Unsigned message parameter
	@returns 1 if success else 0
    */
    
    wyInt32 OnWmCommand(WPARAM wParam);

	/// Sets checkboxes in the listview if selectall checkbox is selected 
	/**
	@param state		: selectall checkbox selected or not 
    */
	void	SetCheckBoxes(wyBool state);

	/// Empty the database i.e. drop all the objects in the database.
    /**
    @returns 1 if success else 0
    */
	wyInt32	EmptyDatabase();

	/// Checks whether any item is selected in the listview.
	/**
	@returns wyTrue if selected else wyFalse
    */
	wyBool	IsAnyItemSelected();

	/* Function to handle the event on ListBox
	*/
	void EmptyDbListViewEvent();

	/// Dialog window HANDLE.
    HWND m_hwnddlg;

	//ListView Handle
    HWND m_hwndlist;

	//Select/Deselect All Handle
    HWND m_hwndselectall;

    /// MySQL pointer.
	PMYSQL			 m_mysql;

	/// Tunnel pointer.
	Tunnel			*m_tunnel;
    
	/// DataBase name.
	wyString		m_db; 

	//Tables in Listview is checked or not
	wyBool m_isalltables;

	//Trigger is checked by checking Tables or not.
	wyBool m_istriggerchecked;

    //Handle to the tool tip control which is used to display the db name
    //HWND m_hwndtooltip;


};

class QueryPreview
{
public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	QueryPreview(wyString* pkeywords = NULL, wyString* pfunctions = NULL);

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~QueryPreview();

	/// Callback dialog procedure for the window.
	/**
	@param hwnd			: Window Handler.
	@param message		: Window message.
	@param wparam		: WPARAM pointer.
	@param lparam		: LPARAM pointer.
	*/
	static	INT_PTR	CALLBACK QueryPreviewDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam); 

	/// Function creates the actual dialog box and also initializes some values of the class.
	/**
	@param hwndparent	: Window Handler.
	@param tunnel		: Tunnel pointer.
	@param umysql		: Pointer to mysql.
	@param db			: Database name.
	@returns 1 if success else 0
    */
	wyInt32		Create(HWND hwndparent, wyChar *query);

	/// Initialize the various start thing for the dialog box .
	/**
	*/
	void  InitDlg();

	 ///window procedure for the query preview editor(scintilla)
    /**
    @param hwnd				: Handle to the dialog box. 
    @param message			: Specifies the message. 
    @param wparam			: Specifies additional message-specific information. 
    @param lparam			: Specifies additional message-specific information. 
    @returns 1 or 0
    */
	static	LRESULT	CALLBACK    TextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	///Resizes the dialog
	/**
	@param hwnd			: IN dialog handle
	@returns void
	*/
	void	Resize(HWND hwnd);

	//dialog handle
    HWND m_hwnddlg;

	/// Original text window procedure
	WNDPROC             m_wporigtextproc ;
	
	//query
	wyString m_query;

    wyString*           m_pkeywords;
    wyString*           m_pfunctions;
};

class ValueList
{
public:

    /// Callback dialog procedure for the window.
	/**
    Constructor.
    */
	ValueList(PMYSQL m_mysql);

    /// Callback dialog procedure for the window.
	/**
    Destructor.
    */
	~ValueList();

	/// Callback dialog procedure for the window.
	/**
	@param hwnd			: Window Handler.
	@param message		: Window message.
	@param wparam		: WPARAM pointer.
	@param lparam		: LPARAM pointer.
	*/
	static	INT_PTR	CALLBACK ValueListDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam); 

	/// Function creates the actual dialog box and also initializes some values of the class.
	/**
	@param hwnd			: Window Handler.
	@param values		: IN/OUT Set or Enum values.
	&param isenum		: IN enum or set
	@returns 1 if success else 0
    */
	wyInt32		Create(HWND hwnd, wyString *values, wyBool isenum);

	/// Initialize the various start thing for the dialog box .
	/**
	@param hwnd			: Window Handler.
	@returns void
	*/
	void	InitDlg(HWND hwnd);

	/// Initialize the ListBox .
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool	InitListBox();

	/// The command handler for create relationship dialog.
	/**
	@param hwnd			: IN Windows HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: In Long message parameter
	@returns void
	*/
	void    OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);

	/// Gets Called when user clicks on Ok control of the Dlg box
	/**
	@returns void
    */
	void	ProcessOK();

	///Adds a value to the listbox
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool	AddValue();

	///Replaces a value from the listbox
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool	ReplaceValue();
	
	//Moves a value into Up/Down to the listbox
	/**
	@param isup		: IN Up/Down 
	@returns wyTrue if success else wyFalse
	*/
	wyBool	MoveUpOrDown(wyBool isup);

	///Sets a selected value from the listbox to the editbox
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool	SetValue();

	//Enables/Disables the Up and Down button
	/**
	*/
	void	EnableOrDisableUpAndDown();
	
	///Removes the selected value from the listbox
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool	RemoveValue();

	//Enables/Disables the Add and Replace button
	/**
	*/
	void	EnableOrDisableValueButtons();
	
	//dialog handle
    HWND m_hwnddlg;

	//dialog handle
    HWND m_hwndlist;

	//values
	wyString m_values;

	//enum or set
	wyBool	m_isenum;

	PMYSQL			m_mysql;
};


//Object Browser Color dialog box
class ConnColorDlg
{
public:
	/// Default constructor.
    /**
    Initializes the member variables with the default values.
    */
	ConnColorDlg();

	 /// Default destructor
    /**
    Free up all the allocated resources
    */
	~ConnColorDlg();

	/// The main dialog proc for the invalid registration dialog box.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static INT_PTR		CALLBACK ConnColorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Displayes the dialog box
	/**
	@param hwnd			: Window Handler
	*/
	wyInt32		    ShowConnColorDlg(HWND hwndparent);

	/// Function handles the WM_COMMAND on the main dialog window
    /** 
    @param hwnd			: IN window handle
    @param wparam		: IN WPARAM value returned by the window procedure
    @returns void
    */
    void            OnWmColorCommand(HWND hwnd, WPARAM wparam, LPARAM lParam);

	wyBool			m_changecolor;

};


//Object Browser Color dialog box
class RenameTabDlg
{
public:
	/// Default constructor.
    /**
    Initializes the member variables with the default values.
    */
	RenameTabDlg();

	 /// Default destructor
    /**
    Free up all the allocated resources
    */
	~RenameTabDlg();

	/// The main dialog proc for the invalid registration dialog box.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static INT_PTR		CALLBACK RenameTabDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Displayes the dialog box
	/**
	@param hwnd			: Window Handler
	*/
	wyInt32		    ShowRenameTabDlg(HWND hwndparent);

	/// Function handles the WM_COMMAND on the main dialog window
    /** 
    @param hwnd			: IN window handle
    @param wparam		: IN WPARAM value returned by the window procedure
    @returns void
    */
    void            OnWmColorCommand(HWND hwnd, WPARAM wparam, LPARAM lParam);

	//wyBool			m_changecolor;

};

#endif
