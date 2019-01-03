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

#define     NO_EXPRESSION_SPECIFIED_FOR_CHECK    _(L"No Expression specified ")

class FieldStructWrapper;

class CheckColumn : public wyElem
{
public:
	FieldStructWrapper *m_pcwrapobj;
	wyInt32             m_lenth;
	CheckColumn(FieldStructWrapper *value);
};

struct CheckConstarintInfo
{
	wyString    m_name;
	wyString    m_checkexpression;
	List        *m_listcolumns;
};

class CheckConstraintStructWrapper : public wyElem
{
public:
	CheckConstarintInfo    *m_newval;
	CheckConstarintInfo    *m_oldval;
	wyString    *m_errmsg;

	CheckConstraintStructWrapper(CheckConstarintInfo *value, wyBool isnew);
	~CheckConstraintStructWrapper();
};


class TabCheck
{
public:
		/// Window handle
		HWND                            m_hwnd;

		//HWND							m_checktext;
	
		/// Grid Window Handle
		HWND                            m_hgridtblcheckconst;
	
		HWND                            m_hdlggrid;
	
		//.. sub tabs' tabmgmt ptr
		TableTabInterfaceTabMgmt*       m_ptabmgmt;
	
		List                            m_listwrapperstruct;
	
		CRITICAL_SECTION                m_cs;       //..Doubt.. Ask vishal
	
		MDIWindow*                      m_mdiwnd;
	
		wyBool                          m_ismysql41;
		wyBool                          m_ismariadb52;
		wyBool							m_ismysql553;
	
		// backtick string from preferences, either empty or quote
		wyChar*							m_backtick;
	
		wyString                        m_celldataprevval;

		// used for Shift+Click functionality
		wyInt32                         m_lastclickindgrid;
		wyInt32                         m_lastclickdlggrid;

	/// Constructor
		TabCheck(HWND hwndparent, TableTabInterfaceTabMgmt* ptabmgmt);
		TabCheck();

	/// Destructor
	~TabCheck();

	/// Grid Window Procedure
	/**
	@param hwnd             : IN hwnd
	@param message          : IN message
	@param wparam           : IN wparam
	@param lparam           : IN lparam
	@returns
	*/
	static LRESULT CALLBACK         GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	//wyBool                  OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam);


	/// Creates the structure of the grid
	/**
	@returns void
	*/
	void                            InitGrid();
	//void                            InitDlgGrid();

	/// intializes m_mysql, m_tunnel and calls CreateGrid()
	/**
	@returns void
	*/
	wyBool                          Create();
	wyInt32                     InsertRow();

	/// Saves the last focused row and column in the grid
	/**
	@returns void
	*/
	void                    OnTabSelChanging();
	void                    ApplyCancelGridChanges();
	wyBool          ValidateChecks(wyBool showmsg = wyFalse);

	//..makes Windows Visible/Invisible
	/*
	@returns    void
	*/
	void                    OnTabSelChange();

	void Resize();

	/// Drops the selected check constraint
	/**
	@returns wyBool, wyTrue if success, otherwise wyFalse
	*/
	wyBool          DropSelectedChecks();

	/// Inserts a new row
	/**
	@returns wyTrue
	*/
	wyBool          ProcessInsert();

	wyBool                  OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam);

	/// Shows the Dialog-box if the grid cell on which mouse is clicked is INDEXCOLUMNS
	/**
	@returns void
	*/
	void                            OnGVNButtonClick();

	wyBool                          OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);

	void                    HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex);

	LRESULT                 OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

	//wyBool          GetAlteredIndexes(wyString &query, wyUInt32 &querybuflen, wyBool execute=wyFalse);

	wyBool          GenerateQuery(wyString &query);

	wyBool                          GenerateCreateQuery(wyString &query);

	wyBool                          GenerateAlterQuery(wyString &query);
	wyBool                          GetDroppedChecks(wyString &query);

	wyUInt32        GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata);

	wyBool                          OnEndEditExpression(WPARAM wParam, LPARAM lParam);

	wyBool                          OnEndEditName(WPARAM wParam, LPARAM lParam);

	CheckConstarintInfo                *GetDuplicateCheckStruct(CheckConstarintInfo* duplicateof);

	/// Generates query for the newly added indexes
	/**
	@param query            : OUT   query string
	@param querybuflen      : OUT   length of query generated
	@param execute          : IN    whether execute or not ? (wyTrue : Alter table || wyFalse : Create table)
	@returns wyTrue if query is valid else wyFalse
	*/
	wyBool                  GetNewAndModifiedChecks(wyString &query, wyBool  execute = wyFalse);

	//..Scans entire row for the changed values (Alter table)
	//.....called on EndLabelEdit and close of the Dialog
	/*
	@param currentrow       : IN    currently selected row
	@param currentcol       : IN    currently selected column
	@param currentdata      : IN    current data
	@returns    wyTrue if row is scanned successfully, else wyFalse
	*/
	wyBool                  ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata);

	//.. Clears all memory allocated during process
	/*
	@param iscallfromdestructor  : IN wyTrue/wyFalse
	@returns
	*/
	void                    ClearAllMemory(wyBool iscallfromdestructor = wyTrue);

	/// Function to initialize the grid with the relevant data i.e about the indexes
	/**
	@returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
	*/
	wyBool          FetchCheckConstraintsIntoWrapper();


	//.. Fill the initial values from the structure "m_backupcopy"  
	/*
	@param iscancelchanges  : IN
	@returns    pointer of duplicate structure object
	*/
	void                    FillInitValues();

	wyBool					 GetAllCheckConstraint(wyChar * createstring, wyString *allcheckconstraint);

	/// After Save, Reinitializes the grid (because, in case, if user doesn't provide index name, we need to show names assigned by mysql)
	/**
	@returns    void
	*/
	void                    ReInitializeGrid();


	void					HandleChecksOnFieldRename(CheckConstraintStructWrapper* checkwrap);

	void				 CheckForQuotesAndReplace(wyString *colname);

	void					CancelChanges(wyBool isaltertable);
	wyBool					ProcessDelete();
	wyBool				DropCheck(wyUInt32 row);
	void ChangeListOnDelete(CheckConstraintStructWrapper* cwrapobj);

	wyBool ExecuteQuery(wyString &query);

};