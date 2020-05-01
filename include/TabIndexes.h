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
class TableTabInterfaceTabMgmt;

#define     NO_COLUMNS_DEFINED_FOR_INDEX    _(L"No columns selected")




class FieldStructWrapper;

class IndexColumn : public wyElem
{
public:
    FieldStructWrapper *m_pcwrapobj;
    wyInt32             m_lenth;
	wyString			m_order;
    IndexColumn(FieldStructWrapper *value);
};

struct IndexInfo
{
    wyString    m_name;
    wyString    m_colsstr;
    List        *m_listcolumns;
    wyString    m_indextype;
	wyString	m_indexcomment;
	wyString	m_visible;
	
};

class IndexesStructWrapper : public wyElem
{
public:
    IndexInfo    *m_newval;
    IndexInfo    *m_oldval;
    wyString    *m_errmsg;

    IndexesStructWrapper(IndexInfo *value, wyBool isnew);
    ~IndexesStructWrapper();
};

class TabIndexes
{
public:
    /// Window handle
    HWND                            m_hwnd;

    /// Grid Window Handle
    HWND                            m_hgridindexes;

    HWND                            m_hdlggrid;

    //.. sub tabs' tabmgmt ptr
    TableTabInterfaceTabMgmt*       m_ptabmgmt;

    List                            m_listwrapperstruct;

    CRITICAL_SECTION                m_cs;       //..Doubt.. Ask vishal
    
    MDIWindow*                      m_mdiwnd;
    
	/// Automated PK Index row
	wyInt32							m_automatedindexrow;

    /// used to store previous cell value
    wyString                        m_celldataprevval;

    wyBool                          m_ismysql41;
	wyBool                          m_ismariadb52;
	wyBool							m_ismysql553;
	wyBool							m_supportsordering;
	wyBool							m_supportsvisibility;


    // used for Shift+Click functionality
    wyInt32                         m_lastclickindgrid;
    wyInt32                         m_lastclickdlggrid;

	RECT							m_wndrect;

	RECT							m_dlgrect;
	
	List							m_controllist;

	// backtick string from preferences, either empty or quote
	wyChar*							m_backtick;

	
    /// Constructor
    TabIndexes(HWND hwndparent, TableTabInterfaceTabMgmt* ptabmgmt);

    /// Destructor
    ~TabIndexes();

    /// Get the wrapper of the existing PK that is dropped
    /*
    @returns the wrapper of the dropped existing index that was PK (for Alter table only)
    */
    IndexesStructWrapper*           GetDroppedPKWrapper();


    /// Handles Primary Key columns in Tab Fields/Columns.
    /**
    @returns void
    */
    void							HandlePrimaryKeyIndex();

    /// Clears/Deletes all list items
    /**
    @param  listindcols     :   IN  list of index columns to be cleared
    @param  indwrap         :   IN  index-wrapper pointer to remove entry of index-column from field-wrappers 
                                    (NULL indicates, only to delete index-columns without removing its entries from fieldstruct-wrappers)
    @returns void
    */
    void                            ClearListIndexCols(List *listindcols, IndexesStructWrapper  *indwrap = NULL);

    /// intializes m_mysql, m_tunnel and calls CreateGrid()
    /**
    @returns void
    */
    wyBool                          Create();

    /// Grid Window Procedure
    /**
    @param hwnd             : IN hwnd
    @param message          : IN message
    @param wparam           : IN wparam
    @param lparam           : IN lparam
    @returns 
    */
    static LRESULT CALLBACK         GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    static LRESULT CALLBACK         DlgGVWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    
    /// Creates the structure of the grid
    /**
    @returns void
    */
    void                            InitGrid();
    void                            InitDlgGrid();

    /// Resize function
    /**
    @returns void
    */
    void                            Resize();

    void                            ResizeColumnsDialog(HWND hwnd, LPARAM lParam);

	// All dervied data should be refreshed based on preferences and also display
	void							Refresh();
	void							Refresh(IndexInfo *indexInfo);

    /// Shows the Dialog-box if the grid cell on which mouse is clicked is INDEXCOLUMNS
    /**
    @returns void
    */
    void                            OnGVNButtonClick();

    /// checks for Index type
    /**
    @param wParam           : IN wparam
    @param lParam           : IN lparam
    @returns wyFalse if more than 1 Primary Index is selected, else wyTrue
    */
    wyBool                          OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);
    wyBool                          OnDlgGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);

    wyBool                          OnEndEditIndexName(WPARAM wParam, LPARAM lParam);
    wyBool                          OnEndEditIndexColumns(WPARAM wParam, LPARAM lParam);
    wyBool                          OnEndEditIndexType(WPARAM wParam, LPARAM lParam);
    wyBool                          OnEndEditIndexComment(WPARAM wParam, LPARAM lParam);
	wyBool                          OnEndEditIndexVisibility(WPARAM wParam, LPARAM lParam);

    /// Shows the Columns Dialog
    /**
    returns void
    */
    void                            ShowColumnsDialog();

    /// Window Procedure of Columns Dialog
    /**
    @param hwnd             : window handle
    @param msg              : message
    @param wParam           : wparam
    @param lParam           : lparam
    returns bool
    */
    static INT_PTR CALLBACK            ColDlgWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    //static BOOL CALLBACK            ColDlgWndProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Intializes the Dialog
    /**
    @param hwnd             : window handle
    returns void
    */
    void                            OnWMInitDialog(HWND hwnd);

    /// Sets the structure with modified values (checked/unchecked) to the selected cell
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            OnIDOK(HWND hwnd);

    /// Reads the attached Linked-list and Fills the List box with column names
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            FillColumnsGrid(HWND hwnd);

    /// Sets the position of the Dialog-box
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            OnDlgWMSize(HWND hwnd);

    /// Handles the WM_COMMAND message of the Dialog-box
    /**
    @param hwnd             : window handle of the Dialog
    @param wParam           : wparam
    @param lparam           : lparam
    @returns void
    */
    void                            OnColDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Enables/Disables the buttons MoveUp/Down of the Dialog-box
    /**
    @param hwnd             : window handle of the Dialog
    @param lparam           : lparam
    @returns void
    */
    void                            OnListViewItemChanged(HWND hwnd, LPARAM lparam);

    /// Handles WM_NOTIFY message
    /**
    @param hwnd             : window handle of the Dialog
    @param wParam           : wparam
    @param lparam           : lparam
    @returns bool
    */
    wyBool                          OnWMNotify(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Moves List items up/down in the dialogbox
    /**
    @param hwnd             : IN window handle of the Dialog
    @param up               : IN whether command is "Up" or "Down"
    @returns void
    */
    void                            OnButtonUpDown(HWND hwnd, wyBool up);

    /// Generates the query for the "Indexes" tab (Create Table)
    /**
    @param hwnd             : IN window handle of the Dialog
    @param up               : IN whether command is "Up" or "Down"
    @returns void
    */
    //wyBool                          GetIndexes(wyString &query);
    wyBool                          GenerateCreateQuery(wyString &query);
    wyBool                          GenerateAlterQuery(wyString &query);

    /// Function to initialize the grid with the relevant data i.e about the indexes
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool          FetchIndexValuesIntoWrapper();


    wyUInt32        GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata);

    /// Process to delete selected rows
    /**
    @returns wyTrue
    */
    wyBool	        ProcessDelete();

    /// Drops the selected indexes
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool          DropSelectedIndexes();

    /// Drops the currently selected index
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool			DropIndex(wyUInt32 row);

    /// Inserts a new row
    /**
    @returns wyTrue
    */
    wyBool          ProcessInsert();

    /// Generates query for the Altered(modified) indexes
    /**
    @param query            : OUT   query string
    @param querybuflen      : OUT   length of query generated
    @param execute          : IN    whether execute or not ? (wyTrue : Alter table || wyFalse : Create table)
    @returns wyTrue if query is valid else wyFalse
    */
    //wyBool          GetAlteredIndexes(wyString &query, wyUInt32 &querybuflen, wyBool execute=wyFalse);

    wyBool          GenerateQuery(wyString &query);

    wyBool          ValidateIndexes(wyBool showmsg = wyFalse);

    /// Generates query for the dropped indexes
    /**
    @param query            : OUT   query string
    @param querybuflen      : OUT   length of query generated
    @param execute          : IN    whether execute or not ? (wyTrue : Alter table || wyFalse : Create table)
    @returns wyTrue if query is valid else wyFalse
    */
    wyBool          GetDroppedIndexes(wyString& query);

    /// Generates query for the newly added indexes
	/**
	@param query            : OUT   query string
    @param querybuflen      : OUT   length of query generated
    @param execute          : IN    whether execute or not ? (wyTrue : Alter table || wyFalse : Create table)
    @returns wyTrue if query is valid else wyFalse
	*/
    wyBool                  GetNewAndModifiedIndexes(wyString &query, wyBool  execute=wyFalse);

    //..Scans entire row for the changed values (Alter table)
    //.....called on EndLabelEdit and close of the Dialog
    /*
    @param currentrow       : IN    currently selected row
    @param currentcol       : IN    currently selected column
    @param currentdata      : IN    current data
    @returns    wyTrue if row is scanned successfully, else wyFalse
    */
    wyBool                  ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata);

    /// Saves the last focused row and column in the grid
    /**
    @returns void
    */
    void                    OnTabSelChanging();

    void                    ApplyCancelGridChanges();

    //..makes Windows Visible/Invisible
    /*
    @returns    void
    */
    void                    OnTabSelChange();

    //.. Cancels the modification and ReInitializes all the window values
    /*
    @param isaltertable     : IN    
    @returns    void
    */
    void                    CancelChanges(wyBool    isaltertable=wyFalse);

    //.. Creates and returns the duplicate structure
    /*
    @param originalstruct   : IN    original structure
    @returns    pointer of duplicate structure object
    */
    //STRUCTINDEXCOLUMNS*     GetDupIndexColsStruct(STRUCTINDEXCOLUMNS* originalstruct);

    //.. Fill the initial values from the structure "m_backupcopy"  
    /*
    @param iscancelchanges  : IN    
    @returns    pointer of duplicate structure object
    */
    void                    FillInitValues();

    //.. Clears all memory allocated during process
    /*
    @param iscallfromdestructor  : IN wyTrue/wyFalse
    @returns    
    */
    void                    ClearAllMemory(wyBool iscallfromdestructor = wyTrue);

    wyBool                  OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam);

    /// After Save, Reinitializes the grid (because, in case, if user doesn't provide index name, we need to show names assigned by mysql)
    /**
    @returns    void
    */
    void                    ReInitializeGrid();

    void                    SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data);

    IndexInfo                *GetDuplicateIndexesStruct(IndexInfo* duplicateof);

    //void                    HandleIndexesOnFieldDelete(IndexesStructWrapper* iindexwrap);
    void                    HandleIndexesOnFieldDelete(IndexesStructWrapper* indexwrap, FieldStructWrapper *deletedfieldwrap);

    void                    HandleIndexesOnFieldRename(IndexesStructWrapper* indexwrap, FieldStructWrapper *modifiedwrap);

    void                    HandleIndexesOnDatatypeChange(IndexesStructWrapper* indexwrap, FieldStructWrapper *modifiedwrap);

    void                    ChangeListOnDelete(IndexesStructWrapper* cwrapobj);

    void                    RemoveIndexWrappersFromFieldsWrappers(IndexColumn *indcol, IndexesStructWrapper* cwrapobj);

    void                    HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex);

    /// performs a length validation depends on type
    /**
    Function to check whether the length fields should be edited or not coz if 
    it is char, varchar, blob or text then only it should be edited.
    @param row			: IN row
    @returns LRESULT  1 if length validation is required, otherwise 0
    */
	LRESULT		            DoLengthValidation(wyInt32 row);

    /// Enable disable the the UP/DOWN buttons
    /**
    @param row			: IN row 
    @returns LRESULT, always 1
    */
	LRESULT		            EnableDisableUpDownButtons(HWND hgrid, wyInt32 row);

    wyBool                  StructFKContainsOtherValues(IndexColumn *value, wyInt32 col);

    LRESULT                 OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

		///Function gets and stores the control rectangles in a linked list
        /**
        @returns void
        */
        void                        GetCtrlRects(HWND hwnd);

        ///Function positions the controls appropriately
        /**
        @returns void
        */
        void                        PositionCtrls(HWND hwnd);

        ///Function is called when Windows ask fo size information
        /**
        @params lparam              : IN message parameter
        @returns void
        */
        void                        OnWMSizeInfo(LPARAM lparam, HWND hwnd);

		void						OnWMPaint(HWND hwnd);

		void						PositionWindow(RECT* prect, HWND hwnd);

        wyInt32                     InsertRow();
		
		
		/**
		Maps A/D to ASC / DESC
		@params val              : IN message parameter A or D 
		@returns ASC or DESC in accordance
		*/
		wyString DecodeIndexOrder(wyString val);

		/**
		Maps yes/no to Visible / Unvisible
		@params val              : IN message parameter : Yes or False
		@returns Visible or Unvisible in accordance
		*/
		wyString DecodeIndexVisibility(wyString val);
		
		
};