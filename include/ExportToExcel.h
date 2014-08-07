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
#include "Global.h"
#include "ExportMultiFormat.h"
#include "CommonHelper.h"

/*! \class wyString
    \brief Class to export to Excel sheet directly
*/
class ExportToExcel
{
private:

	/// Gets the file name to set for the excel sheet
	/**
	@param exceldata :IN Struct containing various info passed to the func
	@return wyTrue on successful retrival of file name else wyFalse
	*/
	wyBool	GetFileName(ExportExcelData *exceldata);

	/// Opens the file for writing
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	OpenFile();

	/// Closes the file when the file writing is completed.
	/**
	@return wyTrue if successful else wyFalse.
	*/
	wyBool	CloseFile();

	/// writes the content of the file into the file in the formatted way.
	/**
	@param exceldata :IN Struct containing various info passed to the func
	@return wyTrue if successful else wyFalse.
	*/
	wyBool	PrintTableToFile(ExportExcelData *exceldata);

	/// Writes the excel header info to the file.
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteExcelHeaders();

	/// Writes the worksheet name
	/**
	@param sheetname :IN sheet name
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteWorkSheets(wyChar *sheetname);

	/// Writes the excel sheet closing info to the file.
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseWorkSheets();

	/// Writes the excel header closing info to the file.
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseHeader();

	/// Writes the given string to the file
	/**
	@param buffer		: IN String to print
	@param data			: IN Flag to decide whether we are writting data or tag
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteToFile(wyString &buffer, wyBool isdata = wyFalse);

	/// Writes the Table header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteTableHeader();

	/// Writes the Table closing header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseTableHeader();

	/// Writes the Row header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteRowHeader();

	/// Writes the Row closing header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseRowHeader();

	/// Writes the Cell header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteCellHeader(wyBool header);

	/// Writes the Cell closing header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseCellHeader();

	/// Writes the Data header to the file
	/**
	@param arg		:IN Data type to write
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteDataHeader(const wyChar *arg);

	/// Writes the Data closing header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseDataHeader();


	/// Writes the field names to the file
	/**
	@param exceldata	: IN ExportExcelData 
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool  WriteFieldNames(ExportExcelData *exceldata);

	/// Writes the field names to the file
	/**
	@param exceldata	: IN ExportExcelData 
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteData(ExportExcelData *exceldata);

	/// Writes the style header to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteStylesHeaders();

	/// Writes the different styles to the file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	WriteStyles();

	/// Writes the style for the field names
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	SetStyleHeader();

	/// Writes the default style to be used in excel
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	SetDefaultStyle();

	/// Writes the date style to be used in excel
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool  SetStyleDate();

	/// Writes the datetime style to be used in excel
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool  SetStyleDateTime();

	/// Writes the time style to be used in excel
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool  SetStyleTime();

	/// Writes the closing style header to file
	/**
	@return wyTrue on successful write in the file else wyFalse
	*/
	wyBool	CloseStylesHeaders();

	/// Gets the cell style for that perticular field value
	/**
	@return style to be used
	*/
	wyChar	*GetCellStyle();

	/// Modifies the data to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return void
	*/
	VOID	TuneData(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the datetime data type to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
	wyBool	TuneDateTime(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the float to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
	wyBool	TuneFloat(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the blob to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
	wyBool	TuneBlob(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the varchar to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
	wyBool	TuneVarChar(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the datatime to the format recognized by excel.
	/**
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
    wyBool  ValidateDateTime(MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the data to the format recognized by excel.
	/**
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
    wyBool  ValidateDate(MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the time to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
    wyBool  ValidateTime(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);

	/// Modifies the timestamp to the format recognized by excel.
	/**
	@param exceldata		: IN ExportExcelData struct
	@param tdata			: IN MysqlDataType struct
	@param data				: IN The actual data
	@param retval			: OUT the returning value after tuning
	@return wyTrue if success else wyFalse
	*/
    wyBool  ValidateTimeStamp(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval);
	
	/// Checks for the validity of the date
	/**
	@param date				: IN date
	@param dateonly			: IN check for only date and not time
	@returns wyTrue if the date is valid else wyFalse
	*/
	wyBool  CheckValidDate(wyString &date, wyBool dateonly = wyFalse);

	/// Checks for the validity of the time
	/**
	@param time				: IN date
	@returns wyTrue if the time is valid else wyFalse
	*/
	wyBool  CheckValidTime(wyString &time);

	/// Changes the timestamp of version 4.0 to the timestamp of version 4.1 or higher
	/**
	@param data				: IN/OUT data to be convert
	*/
	void  ChangeToTimeStamp(wyString &data);

	/// Converts the given string into utf8 string
	/**
	@param ansistr			: IN Text to convert
	@returns utf8 string
	*/
	wyChar*	ConvertToUtf8(wyString &ansistr);


	/// Write the data that are not null
	/**
	@param exceldata		: IN ExportExcelData struct
	@param myrow			: IN Mysql row data
	@param count			: IN Field count
	@param value			: IN/OUT Value of that field
	@returns void
	*/
	void	WriteNotEmptyData(ExportExcelData *exceldata, MYSQL_ROW myrow, wyInt32 count, wyString &value);

	/// Writes field data that are null
	/**
	@param value			: IN/OUT Value of that field
	@returns void
	*/
	void	WriteEmptyData(wyString &value);

	/// File pointer
	FILE		*m_file;

	/// Mysql result set
	MYSQL_RES	*m_resultset;

	/// File name
	HANDLE	    m_filename;

	/// Tunnel pointer
	Tunnel		*m_tunnel;

	/// Mysql fields set
	MYSQL_FIELD	*m_fieldset;

	/// Data type 
	MysqlDataType	m_datatype ;

	/// Hold the code page information
    wyString    m_codepage;

	/// Window HANDLE 
	HWND		m_hwnd;

	///Hold file contents
	wyString    m_buffer;

	wyBool		m_resultfromquery;

public:

	/// Constructor 
	ExportToExcel();

	/// Destructor
	~ExportToExcel();

	wyInt32 m_charset;
	/// Main function to start export
	/**
	@param exceldata	: IN ExportExcelData struct type
	@returns wyTrue if successful export else wyFalse
	*/
	wyBool PrintToExcel(ExportExcelData *exceldata);

};