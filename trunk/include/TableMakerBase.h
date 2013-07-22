/* Copyright (C) 2012 Webyog Inc.

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

#ifndef _TABLEMGMT_H_
#define _TABLEMGMT_H_

#include "DataType.h"
#include "Tunnel.h"
#include "Global.h"
#include "wyString.h"
#include "list.h"
#include "TableMakerAdvProperties.h"
#include "DialogPersistence.h"
#include "OtherDialogs.h"



class MDIWindow;

// column indexes used in table maker and table editor
#define		CNAME										0
#define		DATATYPE									1
#define		LENGTH										2
#define		DEFVALUE									3
#define		PRIMARY										4
#define		BINARY										5
#define		NOTNULL										5
#define		UNSIGNED									6
#define		AUTOINCR									7
#define		ZEROFILL									8
#define     CHARSET                                     9
#define		COLLATION									10
#define		COMMENT										11		

#define     CHARSETCOL                                  9
#define     COLLATIONCOL                                10

// WIDTH used between buttons
#define     MAXLENWITHBACKTICKS                         4

#define		DEFAULTNUMROWS								6

#define		DUPLICATED_FIELDNAME						_(L"Duplicate field name")
#define		VARCHAR_LENGTH_NOTSPECIFIED					_(L"Length must be specified for varchar datatype")
#define		VARBINARY_LENGTH_NOTSPECIFIED				_(L"Length must be specified for varbinary datatype")
#define		EMPTY_ROWS									_(L"There are no fields in this table, define at least one field")
#define		DROP_ALLCOLUMNS								_(L"You cannot delete all columns with ALTER TABLE. Use DROP TABLE instead")
#define		ALTERED_SUCESSFULLY							_(L"Table altered successfully")
#define     FIELDNAME_NOTSPECIFEID						_(L"Field name not specified")
#define		DATATYPE_NOTSPECIFIED						_("Datatype not specified for field name (%s)")
			

//Class used to create linked list that stores table name(s) created by CreateTable option
class NewTableElem :  public wyElem
{
public:
  
	/// Parameter Constructer that intialize m_tablename with tablename that is newly created	
	NewTableElem(const wyChar* tablename);
		
    /// Destructor
	~NewTableElem();

	///Table create
	wyString	m_tablename;
		
};

class TableMakerBase
{
public:

	TableMakerBase();

	~TableMakerBase();

    /// Common function to create and initialize the dialog window
    /**
    @returns        Returns the value returned by DialogBoxParam.
    */
    wyInt32         CreateDialogWindow();

    /// Pure virtual function which will be implemented by the derived classes.
    /**
    this will create the "Create table" or "Alter table" window.
    @param querywindow	: IN QueryWindow pointer
    @param tunnel		: IN Tunnel pointer
    @param mysql		: IN PMYSQL value
    @param hItem		: IN HTREEITEM value
    @param dbname		: IN current database name
    @param tablename	: IN current table name, valid only for alter table.
    @return wyBool	Dialog creation and processing was successful or not	
    */
    virtual wyBool  Create(MDIWindow* querywindow, 
                           Tunnel* tunnel, 
                           PMYSQL mysql,
                           const wyChar * dbname, const wyChar* tablename)=0;

    /// Common window procedure for the dialog.
    /**
    @param hwnd			: IN Handle to the dialog box. 
    @param message		: IN Specifies the message. 
    @param wparam		: IN Specifies additional message-specific information. 
    @param lparam		: IN Specifies additional message-specific information. 
    @returns    Always returns 0
    */
	static BOOL CALLBACK DlgWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Common window procedure for the grid
    /**
    @param hwnd			: IN HANDLE to the dialog box. 
    @param message		: IN Specifies the message. 
    @param wparam		: IN Specifies additional message-specific information. 
    @param lparam		: IN Specifies additional message-specific information. 
    @returns    TRUE or FALSE
    */
    static LRESULT CALLBACK GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Pointer to the query window class
	MDIWindow*		m_querywnd;   


protected:

    void    SetInitialValues(MDIWindow * querywindow, Tunnel * tunnel, PMYSQL mysql, 
                             const wyChar *dbname, const wyChar *tablename);

    /// Initializes the grid with the initial values.
    /**
    If it is in Create table, it will fill the controls with default values, 
    If it is in Alter table, it will fill the grid with table description.
    It takes help from the helper methods listed below
    @return wyTrue if successful or wyFalse otherwise
    */
    wyBool      InitDialog();
    wyBool      InitDialogStep2 ();

    /// Set the correct title for the dialog window.
    /**
	@returns void
    */
	void        SetTitle();

    /// Reposition and resize the dialog window
    /**
	@returns void
    */
	void        MoveDialog();
	
    /// Move the command buttons correctly
    /**
	@param lparam		: IN Specifies additional message-specific information.
	@returns void
    */
    void        MoveButtons(LPARAM lParam);

	///Resizing the grid
	/**
	@param lparam		: IN Specifies additional message-specific information.
	@return void
	*/
	void		MoveGrid(LPARAM lParam);

	///Resizing the dialog
	/**
	@param lparam		: IN Specifies additional message-specific information.
	@return void
	*/
	void		Resize(LPARAM lParam);

    /// Common function to create the grid
    /**
    @returns wyTrue if successful and wyFalse otherwise.
    */
    wyBool          CreateGrid();

    /// Initialize grid with common data for both CREATE/ALTER table
    /**
	@returns void
    */
	void            InitializeGrid();

    /// Initialize with correct data
    /**
	@returns void
    */
	virtual wyBool    Initialize(){return wyTrue;};

	// Interfaces to get correct values from the derived class
    /// Get correct title for the dialog
    /**
    @param str		: IN/OUT  To get the title
	@returns void
    */
    virtual void GetTitle(wyString& str){};

    /// Open the correct page in the help file
    /**
	@returns void
    */
	virtual void HandleHelp(){};

    /// Initialize the grid with correct number of rows
    /**
	@returns void
    */
	virtual void CreateInitRows(){};
	
	/// Grid wnd proc interfaces
    /// Handle GVN_BEGINLABELEDIT
    /**
    Handle operation when a column has just begun to edit.
    @param wparam	: IN Row that is being edited
    @param lparam	: IN Column that is being edited
    @returns whether to allow editing or not
    */
	virtual LRESULT	OnGVNBeginLabelEdit(WPARAM wparam, LPARAM lparam) {return TRUE;};

    /// Handle GVN_ENDLABELEDIT
    /**
    Event when a column has completed editing.
    @param wparam	: IN Specifies the row, column that has just finished editing.
                              The low-order word contains the row while the high-order word contains the column. 
    @param lparam	: IN Text that will be used if editing is successful.
    @returns    Whether to allow editing or not
    */
    virtual LRESULT	OnGVNEndLabelEdit(WPARAM wparam, LPARAM lparam){return TRUE;};

    /// Handle GVN_ENDADDNEWROW
    /**
    Handle operation when a new row has been added
    @param wparam	: IN Specifies the row index that has been added
    @param lparam	: IN Never used
    @returns whether to allow editing or not
    */
    virtual LRESULT	OnGVNEndAddNewRow(WPARAM wparam, LPARAM lparam){return TRUE;};

	/// Validate before label editing
    /**
    @param row		: IN Row of column which is going to be edited
    @param col		: IN Column which is going to be edited
	@returns wyTrue on success else wyFalse
    */    
    wyBool		 ValidateOnBeginLabelEdit(wyUInt32 row, wyUInt32 col);

    /// Validate on end label editing
    /**
    @param wparam	: IN Specifies the row, column that has just finished editing.
							The low-order word contains the row while the high-order word contains the column. 
    @param lparam	: IN  End edited value
	@returns wyTrue
    */    
    wyBool		 ValidateOnEndLabelEdit(WPARAM wparam, LPARAM lparam);

	/// Validate when a user has selected a datatype
    /**
    @param row		: Row of grid for which datatype is provided
    @param col		: Datatype that should be compared for validation
    */    
	wyBool		 SetValidation(wyUInt32 row, wyChar* datatype);

	// Various button click interfaces from the dialog

    /// Process CREATE/ALTER Button Press
    /**
	Handle operation user presses CREATE/ALTER button. The two derived interfaces then
	implement code to create/alter table
    @returns wyTrue if operation was successful otherwise wyFalse
    */
	virtual wyBool	ProcessOk() = 0;

    /// Process cancel button press
    /**
    Handle operation user presses cancels editing. It will ask for confirmation for exiting
    and do accordingly
    @returns wyTrue if operation was successful otherwise wyFalse
    */
	virtual wyBool	ProcessCancel() = 0;

	/// Process delete button press
    /**
    Deletes the selected row from the grid
    @returns wyTrue if operation was successful otherwise wyFalse
    */
	virtual wyBool	ProcessDelete() = 0;

    /// Process insert button press
    /**
    Inserts a new row above the selected row. If the first row is selected then no new row is added
    @returns wyTrue if operation was successful otherwise wyFalse
    */
	virtual wyBool	ProcessInsert() = 0;

    /// Initialize the advanced property dialog for the table
    /**
    @returns wyTrue if operation was successful otherwise wyFalse
    */
	virtual wyBool	ProcessAdvanceProperties() = 0;

    /// Process WM_NCDESTROY
    /**
    Allows the interface to free up any memory on dialog destroy
    @returns Always returns wyTrue
    */
    virtual wyBool  ProcessWMNCDestroy() = 0;

    /// Common function to insert a new row
    /**
    @returns Index of the row position inserted
    */
    wyUInt32        InsertRowInBetween();

    /// Gets the grid window HANDLE
	/**
	@returns HANDLE to grid
	*/    
	HWND		GetGrid() { return m_hwndgrid; }

	/// Gets the dialog window HANDLE
	/**
	@returns dialog HANDLE
	*/
    HWND        GetDlgHwnd() { return m_hwnd; }

	/// Gets the tunnel
	/**
	@returns tunnel pointer
	*/
	Tunnel*		GetTunnel() { return m_tunnel; }

	/// Gets the mysql window
	/**
	@returns pointer to mysql
	*/
	PMYSQL		GetMySQL() { return m_mysql; }

	/// Gets the query window
	/**
	@returns querywindow pointer
	*/
    MDIWindow*  GetQueryWindow() { return m_querywnd; }

	/// Sets the dialog HANDLE
	/**
	@param hwnd			: Window HANDLE
	@returns void
	*/
	void        SetDlgHwnd(HWND hwnd) { m_hwnd = hwnd; }

	/// Sets Font for the table maker grid
	/**
	@returns void
	*/
	void		SetFont();

    /// Filters The Collation Listbox in the Grid according to Character selected
    /** 
	@returns void
    */
    void        FilterCollationColumn(HWND gridhwnd, wyInt32 row, wyInt32 charsetlen);
    
    /// Initializes the Collation Listbox in the Grid to all MySQL supported values
    /** 
	@param gridhwnd			: Grid HANDLE
	@returns void
    */
    void        InitCollationCol(HWND gridhwnd);

	/// This will handle the dialog persistence
    /** 
	@param hwnd		: IN dialog handle
	@returns void
    */
	void		HandleDlgPersistance(HWND hwnd);

	/// This will Show Create table query preview
    /** 
	@returns wyBool
    */
	 virtual wyBool		ShowPreview() = 0;

	/// create the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Create query
    @returns    Creation of query was successful or not
    */	
	virtual  wyBool			CreateQuery(wyString &query) = 0;

	///This will Show/ Hide the Language options(Charset and Collation)
	/** 
	@returns void
    */
	void		HideCharsetAndCollation();

	/// Initializes the Charset Listbox in the Grid to all MySQL supported values
    /** 
	@param gridhwnd			: Grid HANDLE
	@returns void
    */
	void		InitCharsetCol(HWND gridhwnd);

	//Handle the Enum column
	/**
	@param hwndgrid : IN handle to grid
	@param row : IN current grid row
	@param col : IN current grid coulmn
	@return wyTrue if anything chnaged elese return wyFalse
	*/
	wyBool		HandleEnumColumn(HWND hwndgrid, wyInt32 row, wyInt32 col);

	/// Tunnel that will be used for sending MySQL queries
    Tunnel          *m_tunnel;      

	/// Tunnel that will be used for sending MySQL queries
    PMYSQL			m_mysql;  

	/// HANDLE to the grid
    HWND            m_hwndgrid;  

	/// Database in which table will be created/altered
    wyString        m_dbname;    

	/// Table altered or created
    wyString        m_tablename; 


	/// HANDLE to the dialog window
	HWND			m_hwnd;                

	/// Advance property management object
	TableAdvPropValues	m_advprop;        

	/// Checks for primary key defination
	wyBool			m_isprimarydefined;

	/// Flag used to check one auto increment field selected
	wyBool			m_autoincpresent;
	
	/// Flag used to check whether auto increment field is present in the table
	wyBool			m_autoinccheck;

	/// Flag tells atleast one table create for the case of Create Table dialog box
	wyBool			m_ret;

	/// String to keep the name of the auto_increment field
	wyString		m_autoincname;

	/// Persistence class object pointer
	Persist	    *m_p;

	///datatype is enum or set
	wyBool		m_isenumorset;

	///charset resultset
	MYSQL_RES	*m_charsetres;

	///old value of a column While altering the column
	wyString			m_oldvalue;

private:

    /// Process WM_COMMAND
    /**
    Handles WM_COMMAND message for table maker
    @param wparam			: Specifies additional message-specific information. 
    @param lparam			: Specifies additional message-specific information. 
	@returns void
    */
    void            HandleWMCommand(WPARAM wParam, LPARAM lParam);

	/// Handles tiny int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleTinyIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles bit datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleBitValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles bool or boolean datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleBoolAndBooleanValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles small int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleSmallIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles medium int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleMediumIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles integer datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleIntegerValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles big int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleBigIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles float datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleFloatValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles double datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleDoubleValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles real datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleRealValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles decimal datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleDecimalValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles numeric datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleNumericValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles date validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleDateValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles date datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleDataTypeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles time stamp validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleTimeStampValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);
	
	/// Handles time datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleTimeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles year datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleYearValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

	/// Handles char and binary datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
    @param isbinary         : whether the type is binary or not
	@returns wyTrue
	*/
    wyBool          HandleCharAndBinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isbinary);

	/// Handles varchar datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleVarCharAndVarbinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isvarbinary);

	/// Handles all blob datatype text validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@param datatype			: Datatype that should be compared for validation
	@returns wyTrue
	*/
	wyBool          HandleAllBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype);

	/// Handles the blob datatype text validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@param datatype			: Datatype that should be compared for validation
	@returns wyTrue
	*/
    wyBool          HandleBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype);

	/// Handles the set datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool          HandleSetEnumValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles the set default value validation if an empty default is given for invalid datatypes, then it throws a warning
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param data			    : Data typed by user 
	@returns wyTrue
	*/
    void HandleColDefaultValidation(HWND &hwndgrid, wyUInt32 &row, wyChar *data);
};
#endif
