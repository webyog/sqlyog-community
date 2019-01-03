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


#ifndef _TABFIELDS_H_
#define _TABFIELDS_H_

//#include "FrameWindowHelper.h"
#include "OtherDialogs.h"

#define	    ZERO                    0
#define		CNAME	                0
#define		DATATYPE		        1
#define		LENGTH                  2
#define		DEFVALUE                3
#define		PRIMARY                 4
#define		BINARY                  5
#define		NOTNULL                 5
#define		UNSIGNED                6
#define		DEFAULTNUMROWS	        6
#define		AUTOINCR                7
#define		ZEROFILL                8
#define     CHARSET                 9
#define     COLLATION               10
#define		ONUPDATECT              11
#define		COMMENT_                12
#define     CHARSETCOL              9
#define     COLLATIONCOL            10
#define     VIRTUALITY              13
#define     EXPRESSION              14
#define		CHECKCONSTRAINT			15
#define     MAXLENWITHBACKTICKS                         4
#define		DEFAULTNUMROWS								6


#define		DUPLICATED_FIELDNAME_EX						_(L"Duplicate column name")
#define		EMPTY_ROWS_EX								_(L"There are no columns in this table. Define at least one column")
#define		ALTERED_SUCESSFULLY_EX  					_(L"Table altered successfully")
#define		CREATED_SUCESSFULLY							_(L"Table created successfully")
#define     FIELDNAME_NOTSPECIFIED_EX					_("Column name not specified")
#define     NO_COLUMNS_DEFINED                          _("No columns defined")

struct FIELDATTRIBS
{
    wyString                m_name;                     /// Column name
    wyString                m_datatype;                 /// Column datatype
    wyString                m_len;                      /// length attrib
    wyString                m_default;                  /// default value
    wyBool                  m_pk;                       /// flag - pk
    wyBool                  m_binary;                   /// flag - binary
    wyBool                  m_notnull;                  /// flag - nutnull
    wyBool                  m_unsigned;                 /// flag - unsigned
    wyBool                  m_autoincr;                 /// flag - auto increment
    wyBool                  m_zerofill;                 /// flag - zero fill
    wyBool                  m_onupdate;                 /// flag - (for timestamp)
    wyString                m_charset;                  /// column charset
    wyString                m_collation;                /// column collation
    wyString                m_comment;                  /// column comment
	wyString                m_virtuality;               //for mariadb virtual/persistent columns
	wyString                m_expression;               //expression for virtual coloumn.
	wyString                m_mysqlvirtuality;               //for mysql virtual/stored columns
	wyString                m_mysqlexpression;               //expression for virtual coloumn.
	wyString                m_mycheckexpression;               //expression for check constraint.
    struct FIELDATTRIBS*    m_next;                     /// pointer to the next FIELDATTRIBS struct elem
};

class IndexesStructWrapper;

/// This class stores the Index-wrapper pointer
class IndexedBy : public wyElem
{
public:
    IndexesStructWrapper*  m_pindexwrap;                /// pointer to IndexesStructWrapper
    IndexedBy(IndexesStructWrapper* value);
};

class FKStructWrapper;

/// This class stores the FK-wrapper pointer
class ReferencedBy : public wyElem
{
public:
    FKStructWrapper*        m_pfkwrap;                  /// pointer to FKStructWrapper
    ReferencedBy(FKStructWrapper*   value);
};

/// Wrapper of FIELDATTRIBS structure
class FieldStructWrapper : public wyElem
{
public:
    FIELDATTRIBS    *m_oldval;                          /// stores old values (For alter Table only)
    FIELDATTRIBS    *m_newval;                          /// stores new/modified values for the column
    wyString        m_errmsg;                           /// stores any error (to show in the SQLPreview)
    List            m_listindexesworkingcopy;           /// list of "IndexedBy" class object-pointers
    List            m_listindexesbackupcopy;            /// list of "IndexedBy" class object-pointers (used for Alter Table only)
    List            m_listfkworkingcopy;                /// list of "ReferencedBy" class object-pointers
    List            m_listfkbackupcopy;                 /// list of "ReferencedBy" class object-pointers (used for Alter Table only)

	wyBool			m_ischanged;
    FieldStructWrapper(FIELDATTRIBS *value, wyBool isnew);
    ~FieldStructWrapper();
};

class TableTabInterfaceTabMgmt;

class TabFields
{
public:
    TabFields(HWND hwndparent, TableTabInterfaceTabMgmt* ptabmgmt);

    ~TabFields();

    HWND                        m_hwnd;

    HWND                        m_hbtnnew;

    HWND                        m_hbtndelete;

    MDIWindow*                  m_mdiwnd;
    
    /// Structure pointer to store fields values
    List                        m_listwrapperstruct;

	List                        m_listwrapperstruct_2;

    HWND                        m_hwndparent;

    TableTabInterfaceTabMgmt*   m_ptabmgmt;

    HWND                        m_hgridfields;
    
    //  Is mysql41
    wyBool                      m_ismysql41;
	//is mariadb version>=5.2
	wyBool                     m_ismariadb52;

	//is mariadb version>=10.3.9
	wyBool                     m_ismariadb10309;

	//is mysql version>=5.7
	wyBool                     m_ismysql57;

	wyBool						m_ismysql578;

    /// Flag used to check one auto increment field selected
	wyBool			            m_autoincrpresent;

    ///old value of a column While altering the column
	wyString                    m_oldvalue;

    wyString                    m_fieldprevval;

    ///charset resultset
	MYSQL_RES*                  m_charsetres;

    //CRITICAL_SECTION            m_cs;

    /// index of the auto increment row
	wyInt32                     m_autoincrowid;

	
	/// Flag used to check whether auto increment field is present in the table
	wyBool			            m_autoinccheck;


    /// Keeps the name of the row having auto_increment key
	wyString			        m_autorowname;

    wyBool                      m_pkchanged;

    HWND                        m_hchkboxhidelanguageoptions;

    WNDPROC                     m_wporighidelangopt;

    // used for Shift+Click functionality
    wyInt32                     m_lastclick;

    wyString                    m_autoinccol;

    /// Persistence class object pointer
	Persist	                    *m_p;

	// backtick string from preferences, either empty or quote
	wyChar*						m_backtick;
    
	//to check whether control from save table or alter table
	wyBool m_isalter;

    /// intializes m_mysql, m_tunnel and calls CreateGrid()
    /**
    @returns void
    */
    wyBool                        Create();

    /// Creates Grid Window
    /**
    @param hwnd             : IN parent window of the grid
    @returns void
    */
    void                        CreateGrid(HWND hwnd);

    /// Resize function
    /**
    @returns void
    */
    VOID						Resize();

    wyInt32                     GetToolButtonsSize();

    /// validates the grid
    /**
    @returns wyFalse if validation fails else wyTrue
    */
    wyInt32                     OnTabSelChanging();

    /// makes Windows Visible/Invisible
    /*
    @returns    void
    */
    wyInt32                     OnTabSelChange();
    
    /// Validate when a user has selected a datatype
    /**
    @param row		: Row of grid for which datatype is provided
    @param col		: Datatype that should be compared for validation
    */    
	wyBool		                SetValidation(wyUInt32 row, wyChar* datatype);

    /// Initializes the Charset Listbox in the Grid to all MySQL supported values
    /** 
	@param gridhwnd			: Grid HANDLE
	@returns void
    */
	void		                InitCharsetCol();


    ///Handles different types of validation in create table like field or datatype or length not specified, duplicate field name 
	/**
	returns wyBool wyFalse if any validation required else wyTrue
	*/
	wyInt32		                ValidateFields(wyBool    showmsg = wyTrue);

        //Handles if Datatype Not specified
	/**
	@param length		:lenth of the datatype field
	@return wyBool wyTrue if datatype not specified else return wyFalse
	*/
	//wyBool				        DatatypeNotSpecified(wyInt32 length, const wyChar * fieldname);

    //Handles if Length  Not specified for varchar datatype
	/**
	@param datatype		:specifies the datatype
	@param length		:lenth of the datatype field
	@return wyBool wyTrue if datatype not specified else return wyFalse
	*/
	//wyBool			            LengthNotSpecified(wyString *datatype, wyInt32	length);

    /// Initializes the Grid
    /**
	@returns wyBool
    */
    wyBool                      InitGrid();

    /// Sets the Init Values(used for Alter table/Manage (FK/Indexes))
    /**
    @returns wyBool
    */
    wyBool                      SetInitValues();
    
    /// Initialize the grid with correct number of rows
    /**
	@returns void
    */
	void                        CreateInitRows();

    /// Initializes the Collation Listbox in the Grid to all MySQL supported values
    /** 
	@param gridhwnd			: Grid HANDLE
	@returns void
    */
    void                        InitCollationCol();

    /// Filters The Collation Listbox in the Grid according to Character selected
    /** 
	@returns void
    */
    void                        FilterCollationColumn(wyInt32 row, wyInt32 charsetlen);


    /// Grid Window Procedure
    /**
    @param hwnd             : IN hwnd
    @param message          : IN message
    @param wparam           : IN wparam
    @param lparam           : IN lparam
    @returns 
    */
    static LRESULT CALLBACK     GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Grid wnd proc interfaces
    /// Handle GVN_BEGINLABELEDIT
    /**
    Handle operation when a column has just begun to edit.
    @param wparam	: IN Row that is being edited
    @param lparam	: IN Column that is being edited
    @returns whether to allow editing or not
    */
    LRESULT	                    OnGVNBeginLabelEdit(WPARAM wparam, LPARAM lparam);

    /// Validate before label editing
    /**
    @param row		: IN Row of column which is going to be edited
    @param col		: IN Column which is going to be edited
	@returns wyTrue on success else wyFalse
    */    
    wyBool		                ValidateOnBeginLabelEdit(wyUInt32 row, wyUInt32 col);

    /// Creating Wrapper or Modifying Wrapper values
    /*
    @param wParam   : IN wparam
    @param lParam   : IN lparam
    @returns 1 if EndLabel is validated successfully, else returns 0
    */
    LRESULT	                    OnGVNEndLabelEdit(WPARAM wparam, LPARAM lparam);

    /// Validate on end label editing
    /**
    @param wparam	: IN Specifies the row, column that has just finished editing.
							The low-order word contains the row while the high-order word contains the column. 
    @param lparam	: IN  End edited value
	@returns wyTrue
    */
    wyBool		                ValidateOnEndLabelEdit(WPARAM wparam, LPARAM lparam);
    
    /// Handle GVN_ENDADDNEWROW
    /**
    Handle operation when a new row has been added
    @param wparam	: IN Specifies the row index that has been added
    @param lparam	: IN Never used
    @returns whether to allow editing or not
    */
    //LRESULT	                    OnGVNEndAddNewRow(WPARAM wparam, LPARAM lparam);

    //Handle the Enum column
	/**
	@param hwndgrid : IN handle to grid
	@param row : IN current grid row
	@param col : IN current grid coulmn
	@return wyTrue if anything chnaged elese return wyFalse
	*/
    wyBool		                HandleEnumColumn(HWND hwndgrid, wyInt32 row, wyString& datatypestr);

    /// Handles tiny int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleTinyIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles bit datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleBitValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles bool or boolean datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleBoolAndBooleanValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles small int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleSmallIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles medium int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleMediumIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles integer datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleIntegerValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles big int datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleBigIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles float datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleFloatValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles double datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleDoubleValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles real datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleRealValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles decimal datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleDecimalValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles numeric datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleNumericValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles date validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleDateValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles date datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleDataTypeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles time stamp validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleTimeStampValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles time datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleTimeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles year datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleYearValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles char and binary datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
    @param isbinary         : whether the type is binary or not
	@returns wyTrue
	*/
    wyBool                      HandleCharAndBinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isbinary);

	/// Handles varchar datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleVarCharAndVarbinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isvarbinary);

    /// Handles all blob datatype text validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@param datatype			: Datatype that should be compared for validation
	@returns wyTrue
	*/
	wyBool                      HandleAllBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype);

    /// Handles the blob datatype text validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@param datatype			: Datatype that should be compared for validation
	@returns wyTrue
	*/
    wyBool                      HandleBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype);

    /// Handles the set datatype validation
	/**
	@param hwndgrid			: HANDLE to grid window
	@param row				: Row number
	@param index			: Index
	@returns wyTrue
	*/
    wyBool                      HandleSetEnumValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index);

    /// Handles if duplicate field name is present 
	/**
	@return void
	*/
	//void				DuplicateFieldName();

    /// Appends each Column definition to the query
    /**
    @param query            :   contains query string
    @returns    wyTrue, if any column definition is fetched, or wyFalse;
    */
    wyBool                      GenerateQuery(wyString& query);

    wyBool                      GenerateCreateQuery(wyString& str);

    /// Function will Append Altered fields in the "query"(wyString param)
    /**
    @param  query   :   OUT altertable_query
    @returns    Success if fields are changed/added/deleted else Failure
    */
    wyBool                      GenerateAlterQuery(wyString& str);

    wyBool                      GetColumnAndDataType(wyString &query, FIELDATTRIBS*   pfieldattribs);

    /// Helper function to get collation value of the columns
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void                        GetCharsetAndCollationValue(wyString& query, FIELDATTRIBS*   pfieldattribs);

    /// Helper function to get extra values of the column
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void                        GetExtraValues(wyString& query, FIELDATTRIBS*   pfieldattribs);

        /// Helper function to get the default value of the column
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */

    void                        GetDefaultValue(wyString& query, FieldStructWrapper* cwrapobj);
	void                        GetVirtualOrPersistentValue(wyString& query, FieldStructWrapper* cwrapobj);

    void				        HandleDefaults(MYSQL_ROW myfieldrow, MYSQL_RES *myres, wyString &datatype, wyInt32 &ret, wyString& defaultstr);

    /// Helper function to get the comment value of the columns
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void                        GetCommentValue(wyString& query, FIELDATTRIBS*   pfieldattribs);


	/// Helper function to get the check condition of the columns
	/**
	@param query		: OUT wyString in which the values have to be added
	@param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
	*/
	void                        GetCheckValue(wyString& query, FIELDATTRIBS*   pfieldattribs);

    /// Fetches the initial the values
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				        FetchInitData();

    /// Fills the fetched init data
    wyBool                      FillInitData();


    /// Gets information from each field to fill the initial grid
    /**
    @param myfieldres		: IN MYSQL field res
    @param mykeyres			: IN MYSQL key res
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool                      TraverseEachFieldRow(MYSQL_RES *myfieldres, wyString createtable);

    void                        InitFieldAttribs(FIELDATTRIBS *newrowlongvalue);
	//void                        setExpression(wyString *atrrinbute, wyString *createstring);

    /// Process to delete selected rows
    /**
    @returns wyTrue
    */
    wyBool	                    ProcessDelete();

    /// Drops the currently selected field
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				        DropColumn(wyUInt32 row);

    /// Process to inserting rows in between other rows
    /**
    @returns wyTrue
    */
	wyBool	                    ProcessInsert();

    /// Creates the string containing the new and modified columns
    /**
    @param newcolumns		: OUT Add columns string
    @returns wyInt32, 0 if success, -1 if error, otherwise 1
    */
    void                        GetNewAndModifiedColumns(wyString &columnsstr);

    /// Generates Query for Dropped Columns
    /**
    @param newcolumns		: OUT drop columns string
    @returns wyInt32, 0 if success, -1 if error, otherwise 1
    */
	wyInt32                     DropColumnQuery(wyString &dropcolumns);

    /// Cancels the modification and ReInitializes all the window values
    /*
    @param isaltertable     : IN    
    @returns    void
    */
    void                        CancelChanges(wyBool    isaltertable=wyFalse);

    /// Exchanges linkedlists and values associated with the current row with the upper row.
    /*
    @returns    void
    */
    void                        OnClickMoveUp();

    /// Exchanges linkedlists and values associated with the current row with the below row.
    /*
    @returns    void
    */
    void                        OnClickMoveDown();

    /// Exchanges linkedlists and values of row1 and row2.
    /*
    @param row1                 : IN    index of row1
    @param row2                 : IN    index of row2
    @returns    void
    */
    void                        ExchangeRowValues(wyInt32 row1, wyInt32 row2);

    wyUInt32                    GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata);

    //..Scans entire row for the changed values (Alter table)
    //.....called on EndLabelEdit and OnClose of the Dialog
    /*
    @param currentrow       : IN    currently selected row
    @param currentcol       : IN    currently selected column
    @param currentdata      : IN    current data
    @returns    wyTrue if row is scanned successfully, else wyFalse
    */
    wyBool                      ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata);

    /// Drops the selected column
    /*
    @returns wyFalse if no checkbox is checked or user responds "No" to the Confirmation Dialog, else wyTrue
    */
    wyBool                      DropSelectedColumns();

    /*
    @returns void
    */
    void                        OnTabClosing();

    ///This will Show/ Hide the Language options(Charset and Collation)
	/** 
	@returns void
    */
	void		                ShowHideCharsetAndCollation();

    /// Clears all memory
    /*
    @returns void
    */
    void                        ClearAllMemory();

    /// sets the PK columns values according to the changes in the Tab "Indexes" for PRIMARY type index
    /**
    @returns void
    */
	void				        OnPrimaryIndexChange();

    void                        HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam);

    /// Enables/disables the move up/down buttons
    /*
    @param wparam               :   row number of the new selection
    @returns void
    */
    void                        OnGVNEndRowChange(WPARAM wParam);

    LRESULT                     OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

    void                        SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data);

    FieldStructWrapper*         GetNextCWrapObjFromGrid(wyUInt32    row);

    FIELDATTRIBS*               GetDuplicateFieldAttribsStruct(FIELDATTRIBS* original);

    /// Fetches all column values and sets the values to the grid
    /*
    @returns void
    */
    void                        ReInitializeGrid();

    /// Modifies the m_listwrapperstruct member variable when the user deletes the column
    /*
    @param  cwrapobj            :   IN  pointer to FieldStructWrapper object which is deleted
    @returns void
    */
    void                        ChangeListOnDelete(FieldStructWrapper* cwrapobj);

    /// Gets the column definition
    /*
    @param cwrapobj             :   IN  pointer to FieldStructWrapper object which has stored all values for the column
    @param coldef               :   OUT will contain the column definition
    @param isnew                :   IN  flag indicates whether the column is newly added or the old-one.
    @returns void
    */
    void                        GetColumnDefinition(FieldStructWrapper* cwrapobj, wyString &coldef, wyBool isnew);
	//function to findout whethr to generate drop or recreate or not

	wyBool                    IsDropAndRecreateRequired(FieldStructWrapper* cwrapobj,wyBool isnew);

	//function to findout wehther to generate drop or recreate for check constraint

	wyBool                    IsDropAndRecreateCheckRequired(FieldStructWrapper* cwrapobj, wyBool isnew);

    /// rotates through all grid rows, finds the wrapper from the column name
    /*
    @param                      :   IN  column name
    @returns FieldStructWrapper object pointer
    */
    FieldStructWrapper*         GetWrapperObjectPointer(wyString &columnname); 

	/// rotates through all grid rows, finds the wrapper from the column name
	/*
	@param                      :   IN  column name
	@returns FieldStructWrapper object pointer
	*/
	FieldStructWrapper*         GetWrapperObjectPointerch();

    /// Drops columns from indexes when user deletes the field from the grid
    /*
    @param                      :   IN  pointer of the FieldStructWrapper object which is dropped
    @returns void
    */
    void                        HandleIndexesOnFieldDelete(FieldStructWrapper* fieldswrapobj);

    /// Drops columns from foreign keys when user deletes the field from the grid
    /*
    @param                      :   IN  pointer of the FieldStructWrapper object which is dropped
    @returns void
    */
    void                        HandleFKsOnFieldDelete(FieldStructWrapper* fieldswrapobj);

    /// modifies columns in indexes when user modifies the field name from the grid
    /*
    @param                      :   IN  pointer of the FieldStructWrapper object which is modified
    @returns void
    */
    void                        HandleIndexesOnFieldRename(FieldStructWrapper* fieldswrapobj);

    /// modifies columns in foreign keys when user modifies the field name from the grid
    /*
    @param                      :   IN  pointer of the FieldStructWrapper object which is modified
    @returns void
    */
    void                        HandleFKsOnFieldRename(FieldStructWrapper* fieldswrapobj);

    /// modifies length for index-column in indexes when user modifies the column datatype from the grid
    /*
    @param                      :   IN  pointer of the FieldStructWrapper object which is modified
    @returns void
    */
    void                        HandleIndexesOnDatatypeChange(FieldStructWrapper* fieldswrapobj);

    /// Will give the row number in the grid from the wrapper
    /*
    @param pwrapobj             :   pointer of the wrapper for which to get the row number
    @returns                    :   row number
    */
    wyInt32                     GetRowNoForWrapper(FieldStructWrapper* pwrapobj);

    /// Checks whether the length is valid for the supplied datatype
    /*
    @param lengthstr            :   IN  length value in the String format
    @param datatypestr          :   IN  datatype
    @returns true if the length is valid, else returns false
    */
    wyBool                      IsValidLength(wyString  &lengthstr, wyString &datatypestr);

    /// Resets the grid-row data when the user has manually deleted the datatype and column name
    /*
    @param  row                 :   IN  the row which should be reset
    @returns    void
    */
    void                        ResetRowValues(wyUInt32 row);

    /// Checks whether the datatype is numeric of not
    /*
    @param datatype             :   IN  the datatype
    */
    //wyBool                      IsDatatypeNumeric(wyString  &datatype);

    /// Sets the focus to the row where colname column lies (Used by SD)
    /*
    @param colname              :   IN  the column name
    */
    wyBool                      SelectColumn(wyString &colname);
};
#endif