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


#include "wyString.h"
#include "ExportMultiFormat.h"
#include "ClientMySQLWrapper.h"


/*! \struct tagUserHost
    \brief Structure That hold all user information of dlgbox items
	\param HANDLE               m_filename;             /// Handle to file
    \param MYSQL_RES            *m_result;              /// Mysql resultset
    \param MYSQL_FIELD          *m_fields;              /// Mysql fields
    \param TabQueryTypes        *m_tabrec;                  /// Class containing data
    \param Tunnel               *m_tunnel;              /// Tunnel pointer
    \param HWND                 m_hwndmessage;          /// Handle to the message bar
    \param HWND                 m_hwnd;                 /// Window Handle
    \param wyBool               *m_stopped;             /// Flag for export stopping
	\param wyBool			    *m_selcol;              /// Selected column
	\param wyBool			    m_structonly;           /// Structure only flag
	\param wyBool			    m_dataonly;             /// Dataonly flag
	\param wyBool			    m_structdata;           /// Both struct and data enabled
	\param wyString		        m_dbname;               /// Current database name
	\param wyString		        m_tablename;            /// Current table name
*/
struct __sqlexportdata
{
    HANDLE          m_filename;
    MYSQL_RES       *m_result;
    MYSQL_FIELD     *m_fields;
    MySQLDataEx	    *m_tabrec;
    Tunnel          *m_tunnel;
    HWND            m_hwndmessage;
    HWND            m_hwnd;
    wyBool          *m_stopped;
	wyBool			*m_selcol;
	wyBool			m_structonly;
	wyBool			m_dataonly;
	wyBool			m_structdata;
	wyString		m_dbname;
	wyString		m_tablename;

};
typedef struct __sqlexportdata ExportSQLData;


class ExportAsSimpleSQL
{
	/// Writes the 'drop table if exists' statement
	/**
	@param data		: IN ExportSQLData structure
	return wyTrue on successful write else wyFalse
	*/
	wyBool		WriteHeaders(ExportSQLData *data);

    /// Writes the 'Set names' to the file
    /**
    @returns wyTrue on success else wyFalse
    */
    wyBool      WriteSetNames();

	/// Writes the create statement in the file
	/**
	@param data		: IN ExportSQLData structure
	*/
	wyBool		WriteCreateStatement(ExportSQLData *data);

	/// Writes the insert statement in the file
	/**
	@param data		: IN ExportSQLData structure
	*/
	wyBool		WriteInsertStatement(ExportSQLData *data);

	/// Writes the buffer contents to file
	/**
	returns wyTrue on successful write else wyFalse
	*/
	wyBool		WriteToFile(wyString &buffer);

	/// Initializes the class variables
	/**
	@param data		: IN ExportSQLData structure
	*/
	void		Init(ExportSQLData *data);

    /// Adds the column names to the 'create' statement
    /**
    @param data     : IN ExportSQLData structure
    @param value    : IN string to append
    */
	void		AddColumnNames(ExportSQLData *data, wyString &value);

    /// Adds the column values to the 'insert' statement
    /**
    @param data     : IN ExportSQLData structure
    @param value    : IN string to append
    @returns wyTrue on successful addition.
    */
	wyBool		AddValues(ExportSQLData *data, wyString &value);

    /// Determines if length field is required or not
    /**
    @returns wyTrue if length field is required else wyFalse
    */
    wyBool      SkipLength(MysqlDataType &retdatatype);
	
	/// Tunnel pointer
	Tunnel		*m_tunnel;

	/// File name
	HANDLE	    m_filename;

	/// Mysql data type;
	MysqlDataType	m_mysqldatatype;

	/// Mysql resultset
	MYSQL_RES		*m_myres;

	/// Mysql rows
	MYSQL_ROW		m_myrow;

	wyULong			*m_rowlength;

	/// Extended struct for MYSQL_ROW
    PMYSQL			m_mysql;

	/// Window HANDLE
	HWND			m_hwnd;

	/// The escaped data
	wyChar			*m_escapeddata;

	///Hold file contents
	wyString    m_buffer;
    
public:

	 ExportAsSimpleSQL();
	~ExportAsSimpleSQL();

	/// Start function for the SQL export
	/**
	@param data		: IN ExportSQLData structure
	*/
	wyBool		StartSQLExport(ExportSQLData *data);

	//IT sets wyTrue when does the Export table from object-browser
	wyBool		m_isdatafromquery;
	
};
