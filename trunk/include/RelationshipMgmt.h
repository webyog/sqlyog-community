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

#include "FrameWindowHelper.h"

#ifndef _RELATIONMAN_H_
#define _RELATIONMAN_H_

class RelationshipMgmt
{
public:
	RelationshipMgmt();
	~RelationshipMgmt();

	/// The main dialog proc for the index manager dialog box.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static  wyInt32	CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Function to create grid window
	/**
	@param hwnd			: IN Window HANDLE
	@returns wyTrue on success else return wyFalse
	*/
	wyBool    OnCreateGrid(HWND hwnd);

	/// Function is called when the use presses the new button on relation window
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
	void    OnIDCNew(HWND hwnd);

	/// Function is called when the use presses the Edit button on relation window
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
	void	OnIDCEdit(HWND hwnd);

	/// Function is called when the use presses the delete button on relation window to delete a relation
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
	void    OnDelete(HWND hwnd);
    	
	/// Callback procedure for the grid control.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static  LRESULT	CALLBACK GVWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	///The relationship dialog gets initialized from here.
	/**
	@param hwnd			: IN Windows HANDLE
	@param tunnel		: IN Tunnel pointer
	@param mysql		: IN Pointer to mysql pointer
	@param db			: IN Database name
	@param table		: IN Table name
	@returns non zero on successful creation of dialog else wyFalse
	*/
	wyInt32 Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyChar * db, wyChar * table, const wyChar *constraint = NULL);

	/// Function to handle relationship dialog commands
	/**
	@param hwnd			: IN Windows HANDLE
	@param wparam		: IN Unsigned message parameter
	*/
    void        OnWmCommand(HWND hwnd, WPARAM wparam);

	/// Resizes main dialog
	/**
	@param hwnd			: IN handle to dialog
	@return void
	*/
	void		MoveDialog(HWND hwnd);

	///Resizing the dialog
	/**
	@param lparam		: IN Long message parameter
	@return void
	*/
	void		Resize(LPARAM Lparam);

	/// Function to move the windows in proper positions in the dialog box.
	/**
	@returns wyTrue
	*/
    wyBool		MoveWindows();

	/// Function to initialize the grid with the relevant data i.e about the indexes.
	/**
	@returns wyTrue
	*/
	wyBool		InitGridData();

	/// Functions fills the initial grid with the data about the indexes.
	/**
	@returns wyTrue
	*/
	wyBool		FillGridWithData();

	/// Determines whether a table is using InnoDB engine or not
	/**
	@param hwnd			: IN Window HANDLE
	@returns wyTrue
	*/
	wyBool		IsTableInnoDB(HWND hwnd);

	/// Adds remaining items to complete the query when the user deletes a relationship
	/**
	@returns wyTrue
	*/
	wyBool		AddRemaining();

	/// Fills the grid dialog with the new entry when user add a new relationship
	/**
	@returns wyTrue if inserted else wyFalse
	*/
	wyBool		FillGridWithDataNew();

	///Fills the grid with the Default constraint passed through 'Create' function
	/**
	@return VOID
	*/
	VOID		FillGridWithDefaultData();

	/// Deletes FK relationship
	/**
	@returns wyTrue if inserted else wyFalse
	*/
	wyBool		DeleteRelationNew();

	/// Function deletes the selected index.
	/**
	@returns wyTrue if inserted else wyFalse
	*/
	//wyBool		DeleteIndex();

	/// Function to reseek the index when the user exits after doing new/edit index.`
	/**
	@returns wyTrue if inserted else wyFalse
	*/
	wyBool		ReseekIndex(wyWChar * index);

	/// Database name
	wyString    m_db;

	/// Table name
    wyString    m_table;

	/// Index name
    wyString    m_indexname;

	/// Window HANDLE
	HWND		m_hwnd;

	/// Grid HANDLE
    HWND        m_hwndgrid;

	/// Parent window HANDLE
    HWND        m_hwndparent;
	
	/// Index number
	wyInt32		m_index;

	/// Pointer to mysql pointer
	PMYSQL		m_mysql;

	/// Tunnel pointer
	Tunnel		*m_tunnel;

	/// Mysql result set
	MYSQL_RES	*m_mystatusres;

	/// Mysql row 
	MYSQL_ROW	m_mystatusrow;

///Flag tells if any new relationship is created
	wyBool		m_isrelcreated;

	///Flag sets wyTrue if the relationship edition is successful
	wyBool		m_isreledited;

	///Flag tells if any relationship is deleted
	wyBool		m_isreldeleted;

	///keeps the single Fkey details need to display in the Manage Relationship dialog box
	///it has passed as 6th parameter of 'Create'
	wyString	m_deffkey;

	///flag tells whether table supports relationship or not
	wyBool		m_isfksupport;
};

//Used to store the unique index name and inserted in to linked list
class CUniqueIndexElem : public wyElem
{
public:

	///Constructor
	/**
	@param indexname : IN field name of unique index
	*/
	CUniqueIndexElem(wyString &indexname);

	///Destructor
	~CUniqueIndexElem();
	
	/// Unique index field name
	wyString	m_indexname;
};

//Struct holds the values to create F-key , & it is passed as a parameter of 'Create' function(It is used for schema designer)
struct RelFieldsParam
{
	wyString			*parenttable;    // IN referenced table name
	List				*srcfldinfolist; // IN linked list holds the values used to set the source column(s) field
	List				*tgtfldinfolist; // IN linked list holds the values used to set the target column(s) field
	wyBool				iseditfkey;		 //	IN flag tells whether the dlg box is "Edit Relationhip" or "Create Relationship"
    wyString			*firstsrcfld;    // OUT sets the first row Source Column field if F-key creation is successful
	wyString			*firsttgtfld;    // OUT sets the first row Target Column field if F-key creation is successful
	wyString			*fkeyinfos;		 // IN foreign key info.
	wyString			*constraintname; // OUT sets the created F-key name 	
	fkeyOption			ondelete;		 // gets if option ON DELETE is defined
	fkeyOption			onupdate;		 // gets if option ON UPDATE is defined
};

class CCreateRel
{
public:

	CCreateRel();
	~CCreateRel();

	/// The main dialog proc for the create relationship dialog box.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static  wyInt32	CALLBACK	DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// The command handler for create relationship dialog.
	/**
	@param hwnd			: IN Windows HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: In Long message parameter
	@returns void
	*/
	void    OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);


	/// Creates the grid for the relationship manager dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@returns void
	*/
	void    OnCreateGrid(HWND hwnd);

	/// The main dialog proc for the grid window.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
	static  LRESULT	CALLBACK GVWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Creates dialogs for new relation window
	/**
	@param hwnd					: IN Windows HANDLE
	@param tunnel				: IN/OUT Tunnel pointer
	@param mysql				: IN/OUT Pointer to mysql pointer
	@param db					: IN/OUT Database name
	@param table				: IN/OUT Table name
	@param neweditindexname		: IN/OUT index name 
	@param defreldata			: IN structure keeps the default values needed to fill the dialog box
	@returns non zero on successful creation of dialog else wyFalse
	*/
	wyInt32		Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyChar * db, wyChar * table, wyChar * neweditindexname, RelFieldsParam *relfldparam = NULL);


	/// Function takes care of the delete option in create relationship dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnDelete(HWND hwnd, LPARAM lparam);

	/// Function takes care of the update option in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnUpdate(HWND hwnd, LPARAM lparam);

	/// Function is called when CASCADE is ticked within UPDATE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnUpdateCascade(HWND hwnd, LPARAM lparam);

	/// Function is called when NULL is ticked within UPDATE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnUpdateNull(HWND hwnd, LPARAM lparam);
    
	/// Function is called when NO ACTION is ticked within UPDATE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
	void        OnUpdateAction(HWND hwnd, LPARAM lparam);

	/// Function is called when RESTRICT is ticked within UPDATE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnUpdateRestrict(HWND hwnd, LPARAM lparam);

	/// Function is called when CASCADE is ticked within DELETE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnDelCascade(HWND hwnd, LPARAM lparam);

	/// Function is called when NULL is ticked within DELETE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnDelNull(HWND hwnd, LPARAM lparam);

	/// Function is called when NO ACTION is ticked within DELETE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnDelNoAction(HWND hwnd, LPARAM lparam);

	/// Function is called when RESTRICT is ticked within DELETE group in CREATE RELATIONSHIP dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OnDelRestrict(HWND hwnd, LPARAM lparam);

	/// Add query extensions if on deleted is enabled 
	/**
	@param query		: IN/OUT Query string
	@returns wyTrue if addition is successful else wyFalse
	*/
    wyBool      HandleOnDelete(wyString &query);

	/// Add query extensions if on deleted is enabled 
	/**
	@param query		: IN/OUT Query string
	@returns wyTrue if addition is successful else wyFalse
	*/
    wyBool      HandleOnUpdate(wyString &query);

	/// Uncheck all the checked options
	/**
	@param hwnd			: Window HNADLE
	@param nid			: Array of objects to uncheck
	@param size			: Size of the array
	@returns void
	*/
    void        UnCheckAll(HWND hwnd, LPARAM lparam, wyInt32 *nid, wyInt32 size);

	/// Function to create and add indexes
	/**
	@param hwnd			: Window handler
	@returns void
	*/
    void        CreateIndex(HWND hwnd);

	//Sets main dialog cordinates and size
	/**
	@return void
	*/
	void		MoveDialog();

	///Resising the dialog controls
	/**
	@param lParam : IN message parameter
	@return void
	*/
	void		Resize(LPARAM lParam);

	/// Function to create 100 rows in the grid and the columns.
	/**
	@returns wyTrue
	*/
	wyBool		InitGridData();

	/// Retrieves fields and display into the grid
	/**
	@returns wyTrue on successful retrieval else wyFalse
	*/
	wyBool		FillGridData();

	/// Fills the combo box with names of the table
	/**
	@returns wyTrue
	*/
	wyBool		FillCombo();

	/// Fills the 'Create Relationship' dialog box with values passed and disable the combo box(Referenced table).
	/**
	@returns wyTrue for success else wyFalse
	*/
	wyBool		FillDefaultVaues();

	/// For 'Edit Relationship' set the corresponding F-key options in dialog box
	/**
	@return void
	*/
	VOID		SetDialogFkeyOption();

	/// The function is called when the user changes the dest address in the relationship creation dialog
	/**
	@param hwndcombo		: Combo box HANDLE
	@returns wyTrue on successful retrieval else wyFalse
	*/
	wyBool		ChangeDestColumns(HWND hwndcombo);


	/// Handle for setting the Target column list box
	/**
	@param reftabletable : IN referenced table name
	@return wyTrue on success else wyFalse
	*/
	wyBool		FillGridDestColumns(const wyChar *reftabletable);

	/// Handle to fill the Target columns with primary key(s) or unique index(s) of referenced table
	/**
	@param reftabletable : IN reference table name
	@returns void
	*/
	void		FillDestColumnField(const wyChar *reftabletable);


	//Fills the grid Taget columns with Primary keys / Uniqe keys if present to create new Relationship
	/**
	@param reftabletable: IN referenced table name
	@return wyTrue for success ,else return wyFalse
	*/
	wyBool		DestColumnsFillKeys(const wyChar *reftabletable);

	//Fills the grid Taget columns with valus in the list "m_tgetfieldslist" to Edit a relationship
	/**
	@param reftabletable: IN referenced table name
	@return wyTrue for success ,else return wyFalse
	*/
	wyBool		DestColumnsFillDefaultValues();	

	///Checks wheteher the F-key info is edited in "Edit Relationship" dialog box if yes delete it, and create a new one
	/**
	@retun wyTrue if the Foreign key is edited , else return wyFalse
	*/
	wyBool	IsForeignKeyEdited();

	/// For Edit Relationship : Handle deletes the original one and create the new one with edited infos.
	/**
	@param query : IN query for creating new F-key
	@param fknamestr : IN costarint name gets from the dialog box
	@return wytrue if create F_key is successful, else return wyFalse
	*/
	wyBool	DropAndCreateNewForeignKey(wyString *query, wyString *fknamestr);
	    
	/// Creates new indexes
	/**
	@returns wyTrue on successful creation else wyFalse
	*/
	wyBool		CreateNewIndex();

	/// Gets the names for indexes
	/**
	@returns wyTrue 
	*/
	wyBool		GetIndexName();


	/// Checks for columns to be equal
	/**
	@returns wyTrue if they are equal else wyFalse
	*/
	wyBool		IsSelColTotalEqual();

	/// Checks if any column is selected  or not 
	/**
	@returns wyTrue if they are equal else wyFalse
	*/
	wyBool		IsAnyColSel();

	/// Function to start the two create index dialog.
	/**
	@returns wyTrue
	*/
	wyBool		ShowKeyIndex();

	/// Shows the reference index table 
	/**
	@returns wyTrue on successful display else wyFalse
	*/
	wyBool		ShowRefIndex();

	/// Database name
	wyString    m_db;

	/// Table name
    wyString    m_table;
	wyString    m_neweditindexname;

	/// Window HANDLE
	HWND		m_hwnd;

	/// Grid Window HANDLE
    HWND        m_hwndgrid;

	/// Parent window HANDLE
    HWND        m_hwndparent;

	/// Pointer to mysql pointer
	PMYSQL		m_mysql;

	/// Tunnel pointer
	Tunnel		*m_tunnel;
	
	///List stores all fields in the Source columns of 'Create Relationship" dlg box, if default vaules are passed to dialogbox.
	List		*m_srcfieldslist;	

	///List stores all fields in the Target columns of 'Edit Relationship" dlg box, if default vaules are passed to dialogbox.
	List		*m_tgetfieldslist;	

	///The referenced table that passed to the dialogbox
	wyString	m_referencedtable;

	///Flag sets wyTrue if the relationship creation is successful
	wyBool		m_isindexcreated;

	///Foreign key constraint name
	wyString	m_fknamestr;

	//variables keeps the 1st source and 1st target fields appears in the grid
	wyString	m_firstsrcfld;
	wyString	m_firsttgtfld;

	///Flag tells the dialog box for Create Relationship or Edit Relationship
	wyBool		m_iseditrelationship;

	///Flag tells whether the F-Key is changed or not for "Edit Relationship" dialog box
	wyBool		m_isfkeychanged;

	///Flag tells whether the F-key lost while Editing F-key ( used to set the grid in Manage Relationship dlg box)
	wyBool		m_isfkeylost;

	///Keeps pointer to last parameter passed in'Create' function ( Relationship infos)
	RelFieldsParam *m_relfldparam;
};

#endif  _RELATIONMAN_H_
