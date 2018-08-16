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
#include "Datatype.h"
class TableTabInterfaceTabMgmt;

#define CONSTRAINT          0
#define CHILDCOLUMNS        1
#define PARENTDATABASE      2
#define PARENTTABLE         3
#define PARENTCOLUMNS       4
#define ONUPDATE            5
#define ONDELETE            6

#define NO_SRCCOLS   _(L"No referencing columns selected for the foreign key")
#define NO_TGTDB     _(L"Referenced database not selected")
#define NO_TGTTBL    _(L"Referenced table not selected")
#define NO_TGTCOLS   _(L"No referenced columns selected for the foreign key")
#define UNEQUAL_SRC_TGT_COLS    _(L"The number of referencing and referenced columns do not match")

class FieldStructWrapper;

//..Used by Schema Designer
class FKSrcColName_SD : public wyElem
{
public:
    wyString        m_colname;
};


class FKSourceColumn : public wyElem
{
public:
    FieldStructWrapper *m_pcwrapobj;

    FKSourceColumn(FieldStructWrapper *value);
};


class FKTargetColumn : public wyElem
{
public:
    wyString        m_name;
    wyString        m_datatype;
    wyBool          m_selected;
};

struct StructFK
{
    wyString        m_name;
    List            *m_listsrccols;
    wyString        m_srccols;
    wyString        m_tgtdb;
    wyString        m_tgttbl;
    List            *m_listtgtcols;
    wyString        m_tgtcols;
    wyString        m_onupdate;
    wyString        m_ondelete;

    ~StructFK();
};

class FKStructWrapper : public wyElem
{
public:
    StructFK        *m_newval;
    StructFK        *m_oldval;
    wyString        *m_errmsg;

    FKStructWrapper(StructFK  *value, wyBool  isnew);
    ~FKStructWrapper();
};

struct KeysColDetail
{
    wyUInt32                m_sequenceinindex;
    wyString                m_colname;
    wyString                m_datatype;
    wyUInt32                m_size;
    struct KeysColDetail*   m_next;
};

typedef struct KeysColDetail KEYCOLSINFO, *PKEYCOLSINFO;

typedef struct
{
    wyString                m_keyname;
    PKEYCOLSINFO            m_colinfo;
}KEYSINFO, *PKEYSINFO;

class FKUnsavedWrappers : public wyElem
{
public:
    FKStructWrapper         *m_pfkwrap;
    wyInt32                 m_rowind;
};

struct FKSourceColumns
{
    struct FKSourceColumns  *m_prev;
    wyString                m_colname;
    wyString                m_datatype;
    wyUInt32                m_size;
    wyBool                  m_selected;
    struct FKSourceColumns  *m_next;
};

typedef struct FKSourceColumns FKSOURCECOLUMNS;

class TabForeignKeys
{
public:
    
    HWND                            m_hwnd;

    HWND                            m_hgridfk;

    HWND                            m_hdlggrid;

    /// Pointer to mysql pointer
	//PMYSQL		                    m_mysql;

    wyBool                          m_ismysql41;
	wyBool                          m_ismariadb52;

	/// Tunnel pointer
	//Tunnel		                    *m_tunnel;

	/// Mysql result set
	MYSQL_RES	                    *m_mystatusres;

	/// Mysql row 
	MYSQL_ROW	                    m_mystatusrow;

    /// Database name
	wyString                        m_db;

	wyString                        m_prevtgtdb;
    
    /// Table name
    wyString                        m_table;

    wyBool                          m_isfksupport;

    TableTabInterfaceTabMgmt*       m_ptabmgmt;

    wyUInt32                        m_totkeys;      //..Used for Auto Selection of Tgt Columns

    wyString                        m_celldataprevval;

    /// Structure pointer to store fields values
    List                            m_listfkwrappers;

    FKStructWrapper                 *m_failedwrap;
    wyInt32                         m_failedwraprow;
    wyChar                          **m_failedwrapsrccols;
    wyInt32                         m_nfailedwrapsrccols;

    MDIWindow                       *m_mdiwnd;

    // used for Shift+Click functionality
    wyInt32                         m_lastclickfkgrid;
    wyInt32                         m_lastclickdlggrid;

	RECT							m_wndrect;

	RECT							m_dlgrect;

	List							m_controllist;

	//member stores the subclassing procedure for the static control showing the gripper
    WNDPROC							m_gripproc;

	// backtick string from preferences, either empty or quote
	wyChar*							m_backtick;

    void                            InitStructFK(StructFK *value);

    TabForeignKeys(HWND hwndparent, TableTabInterfaceTabMgmt *ptabmgmt);

    ~TabForeignKeys();

    /// Grid Window Procedure
    /**
    @param hwnd             : IN hwnd
    @param message          : IN message
    @param wparam           : IN wparam
    @param lparam           : IN lparam
    @returns 
    */
    static LRESULT CALLBACK         GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    static LRESULT CALLBACK         DlgGridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// intializes m_mysql, m_tunnel and calls CreateGrid()
    /**
    @returns void
    */
    wyBool                          Create();

    /// Resize function
    /**
    @returns void
    */
    void                            Resize();

    /// Creates Grid Window
    /**
    @param hwnd             : IN parent window of the grid
    @returns void
    */
    wyBool                          CreateGrid();

    /// Initializes the Grid
    /**
	@returns wyBool
    */
    wyBool                          InitGrid();
    void                            InitDlgGrid();

    /// Initialize the grid with correct number of rows
    /**
	@returns void
    */
	void                            CreateInitRows();

    /// Handles the wm_command
    /**
    @param hwnd: IN Window handle
    @param wParam: IN Window wparam
    @returns void
    */
    void                            OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);

    /// Shows the Dialog-box if the grid cell on which mouse is clicked is CHILDCOLUMNS or PARENTCOLUMNS
    /**
    @returns void
    */
    void                            OnGVNButtonClick();

    /// Compares the datatypes of sources columns with that of parent table index columns and finds the matching pair of possible target columns
    /**
    @param  row                     : IN row number
    @param  tblname                 : IN table name
    @returns void
    */
    void                            OnTgtTblEndLabelEdit(wyInt32 row, wyString& tblname);

    /// clears memory on parent database selection
    /**
    @param  lparam                  : IN database name
    @returns void
    */
    void                            OnTgtDBSelection(LPARAM lparam);

    wyBool                          OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam);

    /// Window Procedure of Source Columns Dialog
    /**
    @param hwnd             : window handle
    @param msg              : message
    @param wParam           : wparam
    @param lParam           : lparam
    returns bool
    */
    static INT_PTR CALLBACK            SrcColsDlgWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Window Procedure of Target Columns Dialog
    /**
    @param hwnd             : window handle
    @param msg              : message
    @param wParam           : wparam
    @param lParam           : lparam
    returns bool
    */
    static INT_PTR CALLBACK            TgtColsDlgWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Intializes the Target Columns Dialog
    /**
    @param hwnd             : window handle
    returns void
    */
    void                            OnWMInitTgtColsDlg(HWND hwnd);

    /// Fills the Target Columns in the List view
    /**
    @param hwnd             : window handle
    returns void
    */
    void                            FillTgtColsDlgGrid(HWND hwnd);

    /// Handles WM_COMMAND message in Source columns' Dialog
    /**
    @param hwnd             : dialog window handle
    @param wParam           : wparam
    @param lParam           : lparam
    returns bool
    */
    void                            OnSrcColsDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Fetches Parent table columns and attaches the LL to the GridCell in the "row"th row.
    /**
    @param  row             : IN    row index
    @param  tablename       : IN    table name
    returns bool
    */
    wyBool                          FetchTgtTblCols(wyInt32 row, wyString& tablename);
    
    /// Reads the attached Linked-list and Fills the List box with column names
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            FillSrcColsDlgGrid(HWND hwnd);

    /// Sets the structure with modified values (checked/unchecked) to the selected cell
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            OnSrcColsDlgIDOK(HWND hwnd);
    
    /// Handles WM_NOTIFY message
    /**
    @param hwnd             : window handle of the Dialog
    @param wParam           : wparam
    @param lparam           : lparam
    @returns bool
    */
    wyBool                          OnWMNotify(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Intializes the Source Columns Dialog
    /**
    @param hwnd             : window handle
    returns void
    */
    wyBool                          OnWMInitSrcColsDlg(HWND hwnd);

    /// Moves List items up/down in the dialogbox
    /**
    @param hwnd             : IN window handle of the Dialog
    @param up               : IN whether command is "Up" or "Down"
    @returns void
    */
    void                            OnButtonUpDown(wyBool   up);


    /// validates Foreign Keys entry in the grid (Create table)
    /**
    @param  row             : IN    row index
    @param  query           : OUT   query generated
    @param  showmsg         : IN    whether to show message on error or not..
    @returns wyFalse if the FK validation fails, else wyTrue
    */
    wyBool                          ValidateFKs(wyBool showmsg = wyFalse);

    /// Handles WM_COMMAND message in Target columns' Dialog
    /**
    @param hwnd             : dialog window handle
    @param wParam           : wparam
    @param lParam           : lparam
    returns bool
    */
    void                            OnTargetColDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Sets the structure with modified values (checked/unchecked) to the selected cell
    /**
    @param hwnd             : window handle of the Dialog
    returns void
    */
    void                            OnTgtColsDlgIDOK(HWND hwnd);

    /// Gets all tables from the selected Database
    /**
    returns wyFalse, if fails, else wyTrue
    */
    wyBool                          GetAllTables();

    /// Manages check box click in the grid
    /**
    @param row: IN row index
    @returns LRESULT 1 if it is checked, otherwise 0
    */  
    void                            HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex);


    wyBool                          FetchInitValues();
    void                            FillInitValues(List* unsavedfkwrappers = NULL);

    wyBool                          FetchInitValuesHelper(wyUInt32 row, wyString &str);

    /// Fetches and returns the Fields of Parent table
	/**
    @param  dbname              : IN   database name
    @param  tblname             : IN   table name
	@returns STRUCTPARENTCOLUMNS structure object pointer
	*/
    List*                           GetListOfFields(wyString &dbname, wyString &tblname);

    /// Inserts a new row
    /**
    @returns wyTrue
    */
    wyBool                          ProcessInsert();

    /// Process to delete selected rows
    /**
    @returns wyTrue
    */
    wyBool	                        ProcessDelete();


    /// Drops the selected Fks
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool                          DropSelectedForeignKeys();

    /// Drops the currently selected FKs
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool			                DropForeignKey(wyUInt32 row);

    //This function will generate query only for dropped and recreated fk
    wyBool                          GenerateFKDropRecreateQuery(wyString **query, wyUInt32 gridrow);
    
    wyBool                          GenerateAlterQuery(wyString &query);
    
    /// Generates query for the dropped fks
    /**
    @param query            : OUT   query string
    @param execute          : IN    whether execute or not ? (wyTrue : Alter table || wyFalse : Create table)
    @returns wyTrue if query is valid else wyFalse
    */
    wyBool                          GetDroppedFKs(wyString& query);
    
    /// Gets all databases
    /**
    @returns wyTrue if fetched successfully else wyFalse
    */
    wyBool                          GetAllDatabases();
    wyBool                          InsertDBNamesIntoGridList();


    wyUInt32                        GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata);

    /// Shows Target Columns Dialog-box
    /**
    @returns wyTrue if fetched successfully else wyFalse
    */
    void                            ShowTargetColumns();

    /// Deallocates memory of the linked-list
    /**
    @param  head                : IN    pointer to memory to be deallocated
    @param  technique           : IN    method of allocation
    @returns    wyTrue
    */
    void                            ClearListTgtCols(List   *list);
    
    //..Passing 2 args so that, same function can be used to clear SrcCols of m_newval as well as m_oldval(pass NULL as 1st arg, when m_oldval->m_listsrccols is to be cleared)
    void                            ClearListSrcCols(FKStructWrapper *cwrapobj, List *list);

    //..Used for Alter Table 
    /*
    @param  hwnd        :   IN  Grid Handler
    @returns    wyTrue if row is scanned successfully, else wyFalse
    */
    wyBool                          ScanEntireRow(wyInt32  currentrow, wyInt32 currentcol, wyString& currentdata);

    
    /// Helper Function in Auto Selection of Target Columns - Fetches Keys info. (datatype, size)
    /**
    @param  dbnamestr           : IN    database name
    @param  tblnamestr          : IN    table name
    @returns    KEYSINFO pointer
    */
    PKEYSINFO                       FetchKeysInfo(wyString& dbnamestr, wyString& tblnamestr, wyBool &error);

    /// Fetches Keys info. (datatype, size)
    /**
    @param  dbnamestr           : IN        database name
    @param  tblnamestr          : IN        table name
    @param  PKEYSINFO           : IN/OUT    KEYSINFO pointer
    @returns    wyTrue if success, else wyFalse
    */
    wyBool                          GetKeyColumnsDatatypes(wyString& dbnamestr, wyString& tblnamestr, PKEYSINFO keyinfo);

    /// Compares parent column datatypes with source datatypes 
    /**
    @param  PKEYSINFO           : IN    KEYSINFO pointer
    @returns    STRUCTPARENTCOLUMNS pointer with modification.. (If any pair matches, then set m_isselected to wyTrue for that pair)
    */
    List*                           CompareWithSourceColumns(PKEYSINFO pkeyinfo);
    

    wyBool                          AreInternalDatatypesSame(wyString &datatype1, wyString &datatype2);

    /// Appends keycolsinfo to the head
    /**
    @param  head                : IN/OUT    pointer to the STRUCTPARENTCOLUMNS object
    @param  keycolsinfo         : IN        keycolsinfo structure object
    @param  select              : IN        set m_isselect to select
    @returns    pointer to STRUCTPARENTCOLUMNS object (head)
    */
    void                            AppendParentColumnsStruct(List* listsrccols, List* listkeycolumns, PKEYCOLSINFO keycolsinfo, wyBool select=wyFalse);


    /// Saves the last focused row and column in the grid
    /**
    @returns void
    */
    void                            OnTabSelChanging();

    //..makes Windows Visible/Invisible
    /*
    @returns    void
    */
    void                            OnTabSelChange();
    
    //.. Cancels the modification and ReInitializes all the window values
    /*
    @param isaltertable         : IN    
    @returns    void
    */
    void                            CancelChanges();

    
    //.. Creates and returns the duplicate structure
    /*
    @param originalstruct   : IN    original structure
    @returns    pointer of duplicate structure object
    */
    StructFK*                       GetDupStructFK(StructFK *value);
    List*                           GetDupListTgtCols(List *listtgtcols);
    List*                           GetDupListSrcCols(List *listsrccols);

    //..Used to remove all memory.. (Called by Destructor)
    void                            ClearAllMemory();

    /// Determines whether a table is using InnoDB engine or not
	/**
	@param hwnd			: IN Window HANDLE
	@returns wyTrue
	*/
    //..Remove this function and use the one defined in TableTabInerface
	wyBool		                    IsTableInnoDB(HWND hwnd);

    /// Sets the m_isfksupport to fksupported
    /**
    @param  fksupported         : IN    wyTrue if FK is supported by server (InnoDB)
    */
    //void                            SetFKSupport(wyBool fksupported);

    /// After Save, Reinitializes the grid (because, in case, if user doesn't provide name, we need to show names assigned by mysql)
    /**
    @returns    void
    */
    void                            ReInitializeGrid(List *unsavedfkwrappers = NULL);

	// All dervied data should be refreshed based on preferences and also display
	void							Refresh();
	void							Refresh(StructFK* fkInfo);

    void                            SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data);

    wyBool                          StructFKContainsOtherValues(StructFK *value, wyInt32 col);

    void                            ChangeListOnDelete(FKStructWrapper* cwrapobj);

    LRESULT		                    EnableDisableUpDownButtons(HWND hgrid, wyInt32 row);

    void                            ResizeColumnsDialog(HWND hwnd, LPARAM lParam);

    void                            OnSrcColEndLabelEdit(WPARAM wParam, LPARAM lParam);

    void                            OnTgtColEndLabelEdit(WPARAM wParam, LPARAM lParam);

    FKStructWrapper*                ManageWrapperForNewAndOldVal(FKStructWrapper* cwrapobj);

    void                            HandleFKsOnFieldRename(FKStructWrapper* fkwrapobj, FieldStructWrapper *modifiedwrap);

    void                            HandleFKsOnFieldDelete(FKStructWrapper *fkwrapobj, FieldStructWrapper *modifiedwrap);

    wyBool                          GenerateNewAndDroppedFKQuery(wyString   &query);

    wyInt32                         InsertRow();

    void                            InsertWrapperIntoRow(wyUInt32 row, FKStructWrapper *cwrapobj);

    List                            *ParseAndGetSrcCols(wyString& columnsstr);

    void                            ProcessFailedWrapper(wyUInt32 row);

    wyBool                          AddFKOnDrag(FKStructWrapper *cwrap, List *listsrccols);

    wyBool                          SelectForeignKey(wyString &fkname);

    LRESULT                         OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

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
};