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


#include "DataType.h"
#include "Tunnel.h"
#include "Global.h"
#include "wyString.h"

#ifndef _TABLEADVPROPERTIES_H_
#define _TABLEADVPROPERTIES_H_

/*! \struct TableAdvPropValues
	\brief Structure used by CTableAdvprop to get/set table advance properties
    wyString    m_type              Container for table type   
    wyString    m_checksum          Container for table checksum   
    wyString    m_auto_incr         Container for table auto increment value 
    wyString    m_avg_row           Container for table type 
    wyString    m_comment           Container for table comment  
    wyString    m_max_rows          Container for max_rows in table 
    wyString    m_min_rows          Container for min rows in table   
    wyString    m_pwd               Container for table password       
    wyString    m_delay             Container for delay key write       
    wyString    m_rowformat         Container for row format       
    wyString    m_raidtype          Container for raid type      
    wyString    m_chunks            Container for table chunks       
    wyString    m_chunksize         Container for table chunk size       
    wyBool      m_changed 
*/

struct TableAdvPropValues 
{
    wyString    m_type;            
    wyString    m_checksum;         
    wyString    m_auto_incr;        
    wyString    m_avg_row;          
    wyString    m_comment;          
    wyString    m_max_rows;         
    wyString    m_min_rows;         
    wyString    m_delay;            
    wyString    m_rowformat;        
    wyString    m_chunks;           
    wyString    m_chunksize;        
    wyBool      m_changed;
    wyString    m_charset;
    wyString    m_collation;
};


class TableMakerAdvProperties
{
public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
    TableMakerAdvProperties       ();
	
    /// Destructor
    /**
    Free up all the allocated resources if there are any.
       */
    ~TableMakerAdvProperties      ();

    /// Common window procedure for the dialog.
    /**
    @param      hwnd:       IN Handle to the dialog box. 
    @param      message:    IN Specifies the message. 
    @param      wParam:     IN Specifies additional message-specific information. 
    @param      lParam:     IN Specifies additional message-specific information. 
    @returns    Always returns 0
    */
	static	BOOL	CALLBACK	DlgProc(HWND hwnd,UINT message,WPARAM wParam, LPARAM lParam);

    /// Method to start and display the dialog.
    /**
    @param      hwnd:       IN Handle to the parent of the dialog box 
    @param      db:         IN DB for which table is created
    @param      table:      IN Table for which advance property is created
    @param      lParam:     IN/OUT Default values for the dialog. On exit it will contain the new values
    @param      isalter:    IN wyTrue if its for Alter window or wyFalse if its for Create table window
    @returns    1 on success and 0 on false
    */
	wyInt32     Create(HWND hwnd, const wyChar* db, const wyChar * table, Tunnel *tunnel, 
                         PMYSQL mysql, TableAdvPropValues *ptadvp, wyBool isalter);

    /// Common method to generate the correct string for adv values which can be used in CREATE/ALTER table statement
    /**
    @param      tunnel:     IN Tunnel used for mysql operation
    @param      mysql:      IN MySQL pointer used for mysql operation
    @param      patv:       IN Input from which string has to be generated
    @param      str:        IN/OUT Advance property values from which the string will be created
    @returns    wyTrue on success and wyFalse on success
    */
    wyBool      GetAdvPropString(Tunnel *tunnel, PMYSQL mysql, TableAdvPropValues* ptav, wyString& str);

private:

    /// Initialize the dialog
    /**
	@returns void
    */
    void				InitDlgValues();

    /// Fill the grid with initial data
    /**
	@returns void
    */
    void				FillInitialData();

    /// Fill the CTAdvPropValues structure
    /**
	@returns void
    */
    void				FillStructure();	

    /// Helper functions to initialize the dialog
    /// Fill table type options
    /**
	@returns void
    */
    void                InitTableTypeValues();

    /// Fill checksum value options
    /**
	@returns void
    */
    void                InitCheckSumValues();

    /// Fill delay key write values
    /**
	@returns void
    */   
	void                InitDelayKeyWriteValues();

    /// Fill rowformat values
    /**
	@returns void
    */    
	void                InitRowFormatValues();

	/// Copies properties from one table to another
	/**
	@param advprop		: Destination table properties
	@param ptav			: Source table properties
	@returns void
	*/
    void                CopyProperties(TableAdvPropValues *advprop, TableAdvPropValues *ptav);

    /// Initializes the Charset Combobox with all the MySQL supported charsets
    /**
    @returns void
    */
    void                InitCharacterSetCombo();

    /// Handles the Charset Combobox Change 
    /**
    @returns void
    */
    void                HandleTabCharset(HWND hwnd);

    /// Initializes the Collation Combobox with all the MySQL supported collations
    /**
    @returns void
    */
    void                InitCollationCombo();

    void                ReInitRelatedCollations(HWND hwnd, wyWChar *charsetname);

    /// Tunnel Pointer against which queries will be executed 
    Tunnel              *m_tunnel;

	/// MySQL pointer against which queries will be executed 
	PMYSQL				m_mysql;        
    
	/// Container for the db name
	wyString            m_db;    

	/// Container for the table name
    wyString            m_table;        
	
	/// Current row
	wyInt32				m_currow;

	/// To keep track whether its alter table or create table window 
	wyBool				m_isalter;      

	/// Handle to the dialog window
	HWND				m_hwnd;         

	/// Handle to the parent window of the dialog
    HWND                m_hwndparent;     

	/// Pointer to active query window
	MDIWindow*			m_querywnd;    

	/// Pointer to the structure that needs to be used for default value and filled
	TableAdvPropValues	*m_advprop;     

    ///character set name
    wyString            m_charset;
};

#endif
