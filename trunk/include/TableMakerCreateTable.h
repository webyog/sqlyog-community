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


#ifndef _TABLEMAKER_H_
#define _TABLEMAKER_H_

#include "FrameWindowHelper.h"
#include "TableMakerAlterTable.h"
#include "TableMakerBase.h"



class TableMakerCreateTable : public TableMakerBase
{

public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
    TableMakerCreateTable			();
	
    /// Destructor
    /**
    Free up all the allocated resources if there are any.
    */
    virtual				~TableMakerCreateTable();

    /// Creates the dialog
    /**
    Initializes the table management dialog and sets initial values.
    @param querywindow	: IN Pointer to the parent querywindow class
    @param tunnel		: IN Tunnel class used for MySQL queries
    @param mysql		: IN MySQL structure used for MySQL queries
    @param dbname		: IN Database in which table will be created
    @param tablename	: IN This parameter is not used
    */
    wyBool  Create(MDIWindow* querywindow, Tunnel* tunnel, 
                   PMYSQL mysql, const wyChar * dbname, const wyChar* tablename);
    
	///Linked list used to insert the name(s) of created table(s) if the flag m_isinsertlist is wyTrue
	List			m_newtableslist;

	///Flag tells whether to insert created table name into the linked list
	wyBool			m_isinsertlist;

private:

	///Handles different types of validation in create table like field or datatype or length not specified, duplicate field name 
	/**
	returns wyBool wyFalse if any validation required else wyTrue
	*/
	wyBool		CreateTableValidation(); 

	/// Create and execute the CREATE TABLE statement
    /** Uses various helper functions as listed below to generate the correct statements
    @returns    Creation of table was successful or not.
    */
    wyBool		CreateTable();

	/// Generate the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: IN Column definitions
    @param primary		: IN Primary key value definitions
    @returns    Creation of query was successful or not
    */
    wyBool		CreateCompleteQuery(wyString& query, wyString& primary, wyString& autoincr);

	/// Handles if duplicate field name is present 
	/**
	@return void
	*/
	void				DuplicateFieldName();

    /// Helper function to get the column and datatype of the column
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
	wyInt32        GetColumnAndDataType(wyString& query, PROWDATA prowdata);

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

    /// Helper function to get the default value of the column
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void        GetDefaultValue(wyString& query, PROWDATA prowdata);

    /// Helper function to get extra values of the column
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void        GetExtraValues(wyString& query, PROWDATA prowdata);

    /// Helper function to get collation value of the columns
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void        GetCharsetAndCollationValue(wyString& query, PROWDATA prowdata);

    /// Helper function to get the comment value of the columns
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void        GetCommentValue(wyString& query, PROWDATA prowdata);

    /// Helper function to get the primary key value of the row
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */
    void        GetPrimaryKeyValue(wyString& query, PROWDATA prowdata);
    
	/// Helper function to key value of the row (Case when a column is AUTO_INCREMENT as well as a part of PRIMARY KEY)
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns wyTrue if column is AUTO_INCREMENT and PK both, else returns wyFalse
    */
    wyBool        GetKeyValue(wyString&   keydef, PROWDATA prowdata);
    
    /// Helper function to get the auto increment value of the row
    /** 
    @param query		: OUT wyString in which the values have to be added
    @param prowdata		: IN  Array of column values from which data needs to be extracted
	@returns void
    */

    void        GetAutoIncrementKeyValue(wyString& query, PROWDATA prowdata);


	/// Funtion to Initialize TableAdvPropValues structure variables
    /** 
    @param advprop		: OUT structure whose members are initialized
	@returns void
    */
	void		InitAdvProperties(TableAdvPropValues	*advprop);

	/// Grid procedure interface
    /// Handles the starting of grid view label edit
    /**
    @param wparam		: Unsigned message parameter
	@param lparam		: Long message pointer
	@returns wyTrue on success else wyFalse
    */
	virtual LRESULT	OnGVNBeginLabelEdit(WPARAM wparam, LPARAM lparam);

	/// Grid procedure interface
    /// Handles the ending of grid view label edit
    /**
    @param wparam		: Unsigned message parameter
	@param lparam		: Long message pointer
	@returns wyTrue
    */
    virtual LRESULT	OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam);

	// Process that saves the current table and ask whether user wants to create a new table
    /**
	@returns wyTrue on success
    */
	virtual wyBool	ProcessOk();
	
	/// Confirms from users for changes
	/**
    @returns wyTrue
    */
    virtual wyBool	ProcessCancel();

    /// Initiates the deletion of rows process
	/**
    @returns wyTrue
    */
    virtual wyBool	ProcessDelete();

    /// Initiates the process of inserting rows 
	/**
    @returns wyTrue
    */
	virtual wyBool	ProcessInsert();

    /// Creates advance properties dialog
	/**
    @returns wyTrue
    */
	virtual wyBool	ProcessAdvanceProperties();

    /// Process is called just before window is destroyed
	/**
    @returns wyTrue
    */
    virtual wyBool  ProcessWMNCDestroy();

    /// Retrieves the title string
    /** 
    @param str		: Title string
    */
    virtual void	GetTitle(wyString& str);

    /// Handles help file
    /** 
    @returns void
    */
    virtual void	HandleHelp();

    /// Creates initial rows
    /** 
    @returns void
    */
	virtual void	CreateInitRows();

    /// Reinitializes the grid when the user has created a table and wants to create more
    /** 
	@returns void
    */
    void		ReInitialize();

    void HandleDefaults(wyString &query, wyString &dbdefaultvalstr);

    /// Whether there was any change in the grid
    /** 
	@returns wyTrue when changed else wyFalse
    */
    wyBool          IsChanged();

	/// create the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @returns    Creation of query was successful or not
    */	
	wyBool			CreateQuery(wyString &query);

	/// This will show the Create table Query Preview
	/**
	@returns wyTrue if success else wyFalse
	*/
	wyBool		ShowPreview() ;

	// Whether there was any change in the grid
    wyBool			m_changed;              
};

#endif
