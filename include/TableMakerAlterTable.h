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


#include "Global.h"

#include "TableMakerBase.h"

#ifndef _TABLEEDITOR_H_
#define _TABLEEDITOR_H_

class CGridControl;
class MDIWindow;

/*! \struct tagTeditColValue
    \brief used to store the column details once it is started editing
    \param wyString		m_oldname	old column name
    \param wyBool		m_isnew		field is new or not
    \param wyBool		m_alter		altered or not
    \param wyBool		m_onupdate	on update stmt is present or not
*/
struct tagTeditColValue
{
	wyString		m_oldname;
	wyBool          m_isnew;
	wyBool          m_alter;
	wyBool          m_onupdate;
};

/*! Creates a type tagTeditColValue*/ 
typedef tagTeditColValue TEDITCOLVALUE, *PTEDITCOLVALUE;



class TableMakerAlterTable:public TableMakerBase
{

public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	TableMakerAlterTable();

    /// Destructor
    /**
    Free up all the allocated resources if there are any.
    */
	virtual ~TableMakerAlterTable();

    /// Creates the dialog
    /**
    Initializes the table management dialog and sets initial values.

    @param querywindow	: IN Pointer to the parent querywindow class
    @param tunnel		: IN Tunnel class used for MySQL queries
    @param mysql		: IN MySQL class used for MySQL queries
    @param dbname		: IN Database base in which table will be created
    @param tablename	: IN This parameter is not used
    */
    wyBool              Create(MDIWindow *pcquerywindow, Tunnel * tunnel, 
                            PMYSQL mysql, const wyChar *dbname, const wyChar *tablename);

private:

    /// Table Editor Window procedure
	/**
    @param hwnd			: IN HANDLE to the dialog box. 
    @param message		: IN Specifies the message. 
    @param wparam		: IN Specifies additional message-specific information. 
    @param lparam		: IN Specifies additional message-specific information. 
    @returns TRUE or FALSE
    */
	static LRESULT CALLBACK TEditorWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Edit table grid window procedure
	/**
    @param hwnd			: IN HANDLE to the dialog box. 
    @param message		: IN Specifies the message. 
    @param wparam		: IN Specifies additional message-specific information. 
    @param lparam		: IN Specifies additional message-specific information. 
    @returns TRUE or FALSE
    */
	static LRESULT CALLBACK GVWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	
    /// Drops the currently selected field
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				DropColumn();

	///Handles different types of validation in Altertable like field or datatype or length not specified, duplicate field name 
	/**
	returns wyBool wyFalse if any validation required else wyTrue
	*/
	wyBool				AlterTableValidation(); 

    /// Alter the table schema
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				AlterTable();

    /// Checks any one of the field is primary or not
    /**
    @param rowcount			: IN Totalnomer of rows (Number of fields)
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				AnyColumnPrimary(wyInt32 rowcount);

	/// Checks any one of the field is auto increment or not
    /**
    @param rowcount			: IN Totalnomer of rows (Number of fields)
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				AnyColumnAutoIncrement(wyInt32 rowcount);

	

    /// Checks any one of the field is unique or not
    /**
    @param rowcount			: IN Totalnomer of rows (Number of fields)
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				AnyColumnUnique(wyInt32 rowcount);

    /// Creates the primary key string
    /**
    @param primary			: OUT primary key string
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool              AddPrimary(wyString &primary);

	/// Creates the Auto increment string
    /**
    @param primary			: OUT primary key string
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool              AddAutoIncrement(wyString &primary);

    /// Creates the string containing the new columns
    /**
    @param newcolumns		: OUT Add columns string
    @returns wyInt32, 0 if success, -1 if error, otherwise 1
    */
	wyInt32             AddNewColumn(wyString &newcolumns);

	/// Handles if duplicate field name is present 
	/**
	@return void
	*/
	void				DuplicateFieldName();

	//Handles if Datatype Not specified
	/**
	@param length		:lenth of the datatype field
	@return wyBool wyTrue if datatype not specified else return wyFalse
	*/
	wyBool				DatatypeNotSpecified(wyInt32 length, const wyChar * fieldname);
	
	//Handles if Length  Not specified for varchar datatype
	/**
	@param datatype		:specifies the datatype
	@param length		:lenth of the datatype field
	@return wyBool wyTrue if datatype not specified else return wyFalse
    */
	wyBool			LengthNotSpecified(wyString *datatype, wyInt32	length);

    /// Creates the string containing the Changed columns
    /**
    @param newcolumns		: OUT changes columns string
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool              ModifyColumns(wyString &modifycolumns);

    /// Creates the string containing dropped columns
    /**
    @param newcolumns		: OUT drop columns string
    @returns wyInt32, 0 if success, -1 if error, otherwise 1
    */
	wyInt32              DropColumnQuery(wyString &dropcolumns);
	
	/// Handles if all the rows are deleted and alter button is clicked  or if only one row present and delete button is clicked
	/**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				Dropallcolumns();

    /// Gets information from each field to fill the initial grid
    /**
    @param myfieldres		: IN MYSQL field res
    @param mykeyres			: IN MYSQL key res
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool              TraverseEachFieldRow(MYSQL_RES *myfieldres, MYSQL_RES *mykeyres);

	/// Get the title string for alter table
    /**
    @returns void
    */
	virtual void		GetTitle(wyString& str);

	/// Function is called when user presses help button in table creation dialog
    /**
    @returns void
    */
	virtual void		HandleHelp();

	/// Initializes alter table
    /**
    @returns void
    */
	virtual wyBool		Initialize();

	/// Procedure is called at the end of editing columns for table creation
	/// It checks for the various details selected
    /**
    @returns wyTrue
    */
	virtual LRESULT	OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);

	/// Procedure is called after a row is inserted to keep extra info with all rows
    /**
    @returns wyTrue
    */
	virtual LRESULT	OnGVNEndAddNewRow(WPARAM wParam, LPARAM lParam);

	/// Function handles wrong entering of data in the grid, if the user has not entered the first
	/// column then I don't allow him to edit other columns. Also it is the first column and the previous row column has not been set then I don't
	/// allow to change the content either.
    /**
    @returns wyTrue on success else wyFalse
    */
	virtual LRESULT	OnGVNBeginLabelEdit(WPARAM wParam, LPARAM lParam);

	/// Procedure is called when ok button is pressed
	/// This function checks for changes in the dialog and if changed , executes alter table
    /**
    @returns TableMakerBase::ProcessOk
    */
	virtual wyBool	ProcessOk();

	/// Confirms form user before exiting
    /**
    @returns wyTrue
    */
    virtual wyBool	ProcessCancel();

	/// Process to delete selected rows
    /**
    @returns wyTrue
    */
    virtual wyBool	ProcessDelete();

	/// Process to inserting rows in between other rows
    /**
    @returns wyTrue
    */
	virtual wyBool	ProcessInsert();

    /// Initializes the TEDITCOLVALUE objects
    /**
    @param newrowlongvalue      : IN/OUT TEDITCOLVALUE pointer
    */
    void    InitTeditColValue(TEDITCOLVALUE *newrowlongvalue);

    /// Function for the advance properties in the menu
	/**
    @returns wyTrue
    */
	virtual wyBool	ProcessAdvanceProperties();

	/// Function is called just before the window is destroyed , and it clears all the resources
    /**
    @returns wyTrue
    */
    virtual wyBool  ProcessWMNCDestroy();
	
    /// Initializes the objects TABLEADVPROPVALUES with the advanced values of the table
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				InitAdvProp();

    /// Initializes the values in the grid
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				FillInitData();

    /// Helps to free the resources allocated
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				FreeResource();

    /// Function to free the del columns buffer
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				ClearDelRowBuf();

    //Gets the maximum rows from myrow
    /**
    @param  textrow         : IN mysql row value
    @param  tofind          : IN Text to find
    @parasm retval          : OUT returning values
    @returns the integer form of the value
    */
    wyInt32             GetValues(wyChar *textrow, wyChar *tofind, wyString &retval);

	void				HandleDefaults(MYSQL_ROW myfieldrow, MYSQL_RES *myres, wyString &datatype, wyInt32 &ret, wyBool ismysql41);

	/// create the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @returns    Creation of query was successful or not
    */	
	wyBool				CreateQuery(wyString &query);

	/// This will show the Alter table Query Preview
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool		ShowPreview();

	/// Last column index
    wyUInt32            m_lastcol;

    // Totalcol index
    wyUInt32            m_totcol;

    /// Current row index
	wyInt32             m_currow;

    /// Whether the table structure changed or not
	wyBool				m_changed;

    /// Primary key changed or not
	wyBool              m_flgprimarykeychange;

    /// primary key present or not
    wyBool              m_flgprimarykeypresent; 

    /// Editor window handle
	HWND				m_hwnd;

    /// parent window handle
    HWND                m_hwndparent;

    /// Deleted col array
	wyString            *m_delcols;

    /// Current connection window pointer
	MDIWindow*			m_qcquerywnd;

	/// Keeps the name of the row having auto_increment key
	wyString			m_autorowname;

	/// Keeps the name of the row having primary key
	wyString			m_prirowname;

	/////old value of a column While altering the column
	//wyString			m_oldvalue;

};

#endif
