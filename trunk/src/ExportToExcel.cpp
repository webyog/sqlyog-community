/* Copyright (C) 2013 Webyog Inc

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


#include "ExportToExcel.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"

#define UTF8    "utf8"
#define SHEET1	"Sheet1"

#define  SIZE_8K		(8*1024)

ExportToExcel::ExportToExcel()
{
    m_datatype.m_exceltype.SetAs("");
	m_datatype.m_mysqltype.SetAs("");
	m_buffer.SetAs("");
	m_charset = 0;
	m_resultfromquery = wyFalse;
}

ExportToExcel::~ExportToExcel()
{
}

wyBool
ExportToExcel::PrintToExcel(ExportExcelData *exceldata)
{
	m_resultset = exceldata->m_result;
	m_tunnel	= exceldata->m_tunnel;
	m_hwnd		= exceldata->m_hwnd;
	m_resultfromquery = exceldata->m_resultfromquery;

	if(IsMySQL41(exceldata->m_tabrec->m_pmdi->m_tunnel, &exceldata->m_tabrec->m_pmdi->m_mysql) == wyTrue)
		m_codepage.SetAs("utf8");
	else
		m_codepage.Clear();

	if(GetFileName(exceldata) == wyFalse)
		return wyFalse;

	if(PrintTableToFile(exceldata) == wyFalse)
		return wyFalse;

	return wyTrue;
}

wyBool
ExportToExcel::GetFileName(ExportExcelData *exceldata)
{
	m_filename = exceldata->m_filename;

	if(m_filename)
		return wyTrue;

	return wyFalse;
}

wyBool
ExportToExcel::PrintTableToFile(ExportExcelData *exceldata)
{
	DWORD		dwbyteswritten;
	wyInt32		ret	= 0;
	wyString	error;
	wyChar*		encbuffer;
	wyInt32		lenptr = 0;
	//wyWChar		    *wencbuffer;
	//wyUInt32		widelen = 0;
	//first add in a buffer and then write to file
	if(WriteExcelHeaders() == wyFalse)
        return wyFalse;

    if(WriteStylesHeaders()== wyFalse)
        return wyFalse;

    if(WriteStyles() == wyFalse)
        return wyFalse;

    if(CloseStylesHeaders() == wyFalse)
        return wyFalse;

    if(WriteWorkSheets(SHEET1) == wyFalse)
        return wyFalse;

    if(WriteTableHeader() == wyFalse)
        return wyFalse;

    if(WriteFieldNames(exceldata) == wyFalse)
        return wyFalse;

    if(WriteData(exceldata) == wyFalse)
        return wyFalse;
	
    if(CloseTableHeader() == wyFalse)
        return wyFalse;

    if(CloseWorkSheets() == wyFalse)
        return wyFalse;

    if(CloseHeader() == wyFalse)
        return wyFalse;

	//if buffer is not empty then write the data to the file
	if(m_buffer.GetLength() != 0)
	{
		if(m_charset != CPI_UTF8)
		{
			encbuffer =  m_buffer.GetAsEncoding(m_charset, &lenptr);
			ret = WriteFile(m_filename, encbuffer, lenptr, &dwbyteswritten, NULL);
		}
		else
		{
			ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
		}
		//ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
		m_buffer.Clear();
		
		if(!ret)
		{
			error.Sprintf(_("Error no : %d"), GetLastError());
			MessageBox(m_hwnd, error.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
			return wyFalse;
		}
	}

	return wyTrue;
}

wyBool
ExportToExcel::WriteStylesHeaders()
{
	wyString style;

	style.SetAs("\r\n<Styles>");

	return WriteToFile(style);

}

wyBool
ExportToExcel::CloseStylesHeaders()
{
	wyString style;
	style.SetAs("\r\n</Styles>\r\n");

	return WriteToFile(style);

}

wyBool
ExportToExcel::SetDefaultStyle()
{
	wyString defaultstyle;

	defaultstyle.Add("\r\n <Style ss:ID=\"Default\" ss:Name=\"Normal\">");
	defaultstyle.Add("\r\n  <Alignment ss:Vertical=\"Bottom\"/>");
	defaultstyle.Add("\r\n  <Borders/>");
	defaultstyle.Add("\r\n  <Font/>");
	defaultstyle.Add("\r\n  <Interior/>");
	defaultstyle.Add("\r\n  <NumberFormat/>");
	defaultstyle.Add("\r\n  <Protection/>");
	defaultstyle.Add("\r\n </Style>");
	
	return WriteToFile(defaultstyle);
}

wyBool
ExportToExcel::SetStyleHeader()
{
	wyString style;

	style.Add("\r\n <Style ss:ID=\"s27\">");
	style.Add("\r\n  <Font x:Family=\"Swiss\" ss:Color=\"#0000FF\" ss:Bold=\"1\"/>");
	style.Add("\r\n </Style>");

	return WriteToFile(style);
}


wyBool
ExportToExcel::SetStyleDate()
{
	wyString style;

	style.Add("\r\n <Style ss:ID=\"s21\">");
	style.Add("\r\n  <NumberFormat ss:Format=\"yyyy\\-mm\\-dd\"/>");
	style.Add("\r\n </Style>");

	return WriteToFile(style);
}

wyBool
ExportToExcel::SetStyleDateTime()
{
	wyString style;

	style.Add("\r\n <Style ss:ID=\"s22\">");
	style.Add("\r\n  <NumberFormat ss:Format=\"yyyy\\-mm\\-dd\\ hh:mm:ss\"/>");
	style.Add("\r\n </Style>");

	return WriteToFile(style);
}

wyBool
ExportToExcel::SetStyleTime()
{
	wyString style;

	style.Add("\r\n <Style ss:ID=\"s23\">");
	style.Add("\r\n  <NumberFormat ss:Format=\"hh:mm:ss\"/>");
	style.Add("\r\n </Style>");

	return WriteToFile(style);
}

wyBool
ExportToExcel::WriteStyles()
{
	if(SetDefaultStyle() == wyFalse)
		return wyFalse;
	
	if(SetStyleHeader() == wyFalse)
		return wyFalse;

	if(SetStyleDate() == wyFalse)
		return wyFalse;

	if(SetStyleDateTime() == wyFalse)
		return wyFalse;

	if(SetStyleTime() == wyFalse)
		return wyFalse;
	
	return wyTrue;
}

wyBool
ExportToExcel::WriteData(ExportExcelData *exceldata)
{
	wyInt32		    count, messagecount = 0, rowcount = 0, rowptr = 0;
	wyString	    mysqlfieldtype, value, messbuff;
    MYSQL_ROW		myrow;
    MYSQL_RES		*myres;
	MYSQL_ROWS		*rowswalker = NULL;
	wyULong			*rowlength = NULL;
   
    myres = exceldata->m_result;
    m_tunnel->mysql_data_seek(myres,(my_ulonglong)0);

	//this condition true if resullt tab in edit mode or result set is edited or for table tab
    /*if(exceldata->m_tabrec->m_rowarray)
		tmp = exceldata->m_tabrec->m_data->m_data; */

	while(1)
    {
		if(*exceldata->m_stopped == wyTrue)
        {
            SetWindowText(GetDlgItem(exceldata->m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }

		// this condition is ture if export from table tab or from result tab in read only mode with the result set is not edited
        if(!exceldata->m_tabrec->m_rowarray || !exceldata->m_tabrec->m_rowarray->GetRowArray())
        {
			if(m_resultfromquery == wyTrue)
			{
				m_tunnel->mysql_data_seek(myres, rowcount);      
				myrow = m_tunnel->mysql_fetch_row(myres);
			}
			else
			{
				SeekCurrentRowFromMySQLResult(myres, &rowswalker, m_tunnel, &myrow, &rowlength);
			}

			if(!myrow)
				break;
		}

		//If the result tab in 'edit mode' or result set is edited or from table tab
		else 
        {
			//bug fixed.http://code.google.com/p/sqlyog/issues/detail?id=494
			//no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse           
            if(rowptr == exceldata->m_tabrec->m_rowarray->GetLength())
                break;

            //no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            
            
            if(exceldata->m_tabrec->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
			{
                rowptr++;
                break;
            }
            if(exceldata->m_tabrec->m_modifiedrow >=0 && exceldata->m_tabrec->m_modifiedrow == rowcount && 
                exceldata->m_tabrec->m_oldrow->IsNewRow() == wyFalse)
            {
                myrow = exceldata->m_tabrec->m_oldrow->m_row;
            }
            else
            {
                myrow = exceldata->m_tabrec->m_rowarray->GetRowExAt(rowptr)->m_row;
            }
            rowptr++;
        }

		VERIFY(WriteRowHeader() == wyTrue);

		///Gets all field(s) length. This condition true only if retrives from 'readonly' result set & data is not edited at all
        if(!exceldata->m_tabrec->m_rowarray && (m_resultfromquery == wyFalse))
		{
			m_datatype.length = rowlength;
		}
        else if(!exceldata->m_tabrec->m_rowarray)
		{
			//If export from obje-browser menu
            m_datatype.length = m_tunnel->mysql_fetch_lengths(myres);
		}
		else
		{
			m_datatype.length = NULL;
		}

		for(count = 0; count < exceldata->m_result->field_count; count++)
		{
			if(exceldata->m_selcol[count] == wyFalse)
				continue;

			GetMySqlDataType(&m_datatype, myres->fields, count);
            
			//if myrow[count] is NULL or if myrow[count] is "(NULL)" then we will write empty data
			if((myrow[count] && stricmp(myrow[count], STRING_NULL)) && (m_datatype.m_exceltype.GetString() != 0))
				WriteNotEmptyData(exceldata, myrow, count, value);
			else
				WriteEmptyData(value);
		}
		VERIFY(CloseRowHeader() == wyTrue);

        messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(exceldata->m_hwndmessage, WM_SETTEXT, 0,(LPARAM) messbuff.GetAsWideChar());

		//Freeing the buffer that keeps the row-lengths
		if(m_resultfromquery == wyFalse && rowlength)
		{
			free(rowlength);
			rowlength = NULL;
		}
        
        rowcount++;
	}

	//Freeing the buffer that keeps the row-lengths
	if(m_resultfromquery == wyFalse && rowlength)
	{
		free(rowlength);
		rowlength = NULL;
	}

	return wyTrue;
}

void
ExportToExcel::WriteEmptyData(wyString &value)
{
	wyString header;

	header.SetAs("\r\n    <ss:Cell>");

	VERIFY(WriteToFile(header) == wyTrue);

	header.SetAs("<Data ss:Type=\"String\">");
	VERIFY(WriteToFile(header) == wyTrue);

	value.SetAs("");

	VERIFY(WriteToFile(value, wyTrue) == wyTrue);

	VERIFY(CloseDataHeader() == wyTrue);
	VERIFY(CloseCellHeader() == wyTrue);
}

void
ExportToExcel::WriteNotEmptyData(ExportExcelData *exceldata, MYSQL_ROW myrow, wyInt32 count, wyString &value)
{
	wyString excelfieldtype;

	TuneData(exceldata, &m_datatype, myrow[count], value);
	
	VERIFY(WriteCellHeader(wyFalse) == wyTrue);
	
	excelfieldtype.SetAs(m_datatype.m_exceltype.GetString());

	if(excelfieldtype.GetLength() == 0)
		VERIFY(WriteDataHeader("String") == wyTrue);
	else
		VERIFY(WriteDataHeader(excelfieldtype.GetString()) == wyTrue);

	VERIFY(WriteToFile(value, wyTrue) == wyTrue);

	VERIFY(CloseDataHeader() == wyTrue);
	VERIFY(CloseCellHeader() == wyTrue);

}

VOID
ExportToExcel::TuneData(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString    temp, temp1, temp2;
	wyChar      *utfdata;

    temp.SetAs(data);
	
	if(!temp.GetLength())
	{
		retval.SetAs("");
		return;
	}

	if(TuneDateTime(exceldata, tdata, data, retval) == wyTrue)
		return;
	if(TuneBlob(exceldata, tdata, data, retval) == wyTrue)
		return;
	if(TuneVarChar(exceldata, tdata, data, retval) == wyTrue)
		return;
	if(TuneFloat(exceldata, tdata, data, retval))
		return;
	else
	{
		utfdata = (wyChar *)temp.GetString();
		if(!utfdata)
		{
			retval.SetAs("");
			return;
		}

		temp.SetAs(utfdata);
        retval.SetAs(temp);
		return;
	}
}

wyBool
ExportToExcel::TuneVarChar(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString		tempdata, origdata ;
	wyInt32			textlimit;
    wyChar			*utfdata ;
	wyWChar			*widestring;
	ConvertString	convert;

    tempdata.SetAs(data);
	origdata.SetAs(data);

	if(stricmp(m_datatype.m_mysqltype.GetString(), "VarChar") == 0)
	{
		utfdata = ConvertToUtf8(tempdata);
		if(!utfdata)
		{
			retval.SetAs("");
			return wyFalse;
		}
		
		tempdata.SetAs(utfdata);
		textlimit = atoi(exceldata->m_textlimit.GetString());

        if(textlimit == 0 || exceldata->m_textlimit.GetLength() == 0)
            textlimit = 255;

		widestring = convert.ConvertUtf8ToWideChar((wyChar *)tempdata.GetString());
        
		if((textlimit < origdata.GetLength()))
				widestring[textlimit] = '\0';
		
		utfdata = convert.ConvertWideCharToUtf8(widestring);
		retval.SetAs(utfdata);

		return wyTrue;
	}
	return wyFalse;
}

wyBool
ExportToExcel::TuneFloat(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString	tempdata, finaldecimal, temp2;
    wyChar		*decimal;
    wyInt32		num;
	wyInt32		dotpos ;

	tempdata.SetAs(data);

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Float") == 0 ||
		stricmp(m_datatype.m_mysqltype.GetString(), "Decimal") == 0 ||
		stricmp(m_datatype.m_mysqltype.GetString(), "Double") == 0)
    {   
		num =atoi(exceldata->m_decimal.GetString());

		//if first character is '.' then strtok will return the decimal part.
		if((dotpos = tempdata.FindI(".", 0)) == 0)
		{
			decimal = strtok((wyChar *)tempdata.GetString(), ".");			
		}
		else
		{
			tempdata.SetAs( strtok((wyChar *)tempdata.GetString(), "."));
	        decimal =  strtok(NULL, ".");
		}

        if(decimal == NULL)
        {
            retval.SetAs(tempdata.GetString());
            return wyTrue;
        }

        finaldecimal.SetAs(decimal);
		
		if(finaldecimal.GetLength() > num)
			finaldecimal.Strip(finaldecimal.GetLength() - num);

		//if first character is '.' then we are adding a zero before the the decimal point 
		if(dotpos == 0)
			tempdata.SetAs("0");
		
        retval.Sprintf("%s.%s", tempdata.GetString(), finaldecimal.GetString()); 
		return wyTrue;
    }
	return wyFalse;
}

wyBool
ExportToExcel::TuneBlob(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString		tempdata, part, origdata;
	wyInt32			textlimit;
	wyULong			lenrow = 0;
	wyWChar			*widestring;
	ConvertString	convert;

	wyChar *utfdata;

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Blob") == 0 || stricmp(m_datatype.m_mysqltype.GetString(), "text") == 0)
    {
        tempdata.SetAs(data);
		origdata.SetAs(data);
		
        //If export from 'read-only' result mode. If export from 'table tab' or edit mode result tab(if result set linked list present)
		//this 'length' pointer would be NULL
		if(tdata->length)
			lenrow = *tdata->length;

		//If export from 'table tab' or from 'edit mode' result set.(This time data takes from resultset linkedlist)
		else
			lenrow = strlen(data) + 1;

		if(stricmp(m_datatype.m_mysqltype.GetString(), "text") != 0 && memchr(tempdata.GetString(), NULL, lenrow))
        {
            retval.SetAs("(Binary/Image)");
            return wyTrue;
        }

		textlimit = atoi(exceldata->m_textlimit.GetString());

        if(textlimit == 0 || exceldata->m_textlimit.GetLength() == 0)
            textlimit = 255;

        if(exceldata->m_textlimit.GetString())
        {
			utfdata = (wyChar *)tempdata.GetString();

			if(!utfdata)
			{
				retval.SetAs("");
				return wyFalse;
			}
			widestring = convert.ConvertUtf8ToWideChar(utfdata);
	
			if((textlimit < origdata.GetLength()))
				widestring[textlimit] = '\0';
			
			utfdata = convert.ConvertWideCharToUtf8(widestring);
			retval.SetAs(utfdata);
            return wyTrue;
        }
    }
	return wyFalse;
}

wyBool
ExportToExcel::TuneDateTime(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString origdata, date, time;

	origdata.SetAs(data);

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Date") == 0)
	{
		if(ValidateDate(tdata, data, retval) == wyTrue)
        {
			origdata.Sprintf("%sT00:00:00.000", origdata.GetString());
			retval.SetAs(origdata.GetString());
		}
		return wyTrue;
	}

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Time") == 0)
	{
        if(ValidateTime(exceldata, tdata, data, retval) == wyTrue)
            retval.Sprintf("1899-12-31T%s%s", origdata.GetString(), ".000"); 
	    return wyTrue;
	}

	if(stricmp(m_datatype.m_mysqltype.GetString(), "DateTime") == 0)
	{
        if(ValidateDateTime(tdata, data, retval) == wyTrue)
        {
		    date.SetAs( strtok((wyChar *)origdata.GetString(), " "));
            time.SetAs(strtok(NULL, " "));
		    retval.Sprintf("%sT%s.000",date.GetString(), time.GetString());
        }
		return wyTrue;
	}

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Year") == 0)
	{
		retval.SetAs(data);
		tdata->m_exceltype.SetAs("Number");
	    return wyTrue;
	}

	if(stricmp(m_datatype.m_mysqltype.GetString(), "TimeStamp") == 0)
	{
        if(ValidateTimeStamp(exceldata, tdata, data, retval) == wyTrue)
        {
		    date.SetAs( strtok((wyChar *)retval.GetString(), " "));
            time.SetAs(strtok(NULL, " "));
		    retval.Sprintf("%sT%s.000",date.GetString(), time.GetString());
        }
		return wyTrue;
	}

	return wyFalse;

}

wyBool
ExportToExcel::ValidateTimeStamp(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString temp, origdata;

	temp.SetAs(data);
	origdata.SetAs(temp.GetString());

	if(IsMySQL41(m_tunnel, &exceldata->m_tabrec->m_pmdi->m_mysql) == wyFalse)
	{
		ChangeToTimeStamp(temp);
		origdata.SetAs(temp.GetString());
	}
	if(CheckValidDate(temp) == wyFalse)
	{	
		tdata->m_exceltype.SetAs("String");
		retval.SetAs(origdata.GetString());
		return wyFalse;
	}

	retval.SetAs(origdata.GetString());
	return wyTrue;
}

wyBool
ExportToExcel::ValidateTime(ExportExcelData *exceldata, MysqlDataType *tdata, wyChar *data, wyString &retval)
{
    wyString tempdata, hh; 

    tempdata.SetAs(data);

	if(CheckValidTime(tempdata) == wyFalse)
	{
		tdata->m_exceltype.SetAs("String");
		retval.SetAs(data);
		return wyFalse;
	}

    retval.SetAs(data);
    return wyTrue;
}

wyBool
ExportToExcel::ValidateDateTime(MysqlDataType *tdata, wyChar *data, wyString &retval)
{
    wyString temp, year, month, day;
    
	temp.SetAs(data);

	if(CheckValidDate(temp) == wyFalse)
	{
		tdata->m_exceltype.SetAs("String");
		retval.SetAs(data);
		return wyFalse;
	}

	retval.SetAs(data);
	return wyTrue;
}


wyBool
ExportToExcel::ValidateDate(MysqlDataType *tdata, wyChar *data, wyString &retval)
{
	wyString temp, year, month, day;
  
	temp.SetAs(data);

	if(CheckValidDate(temp, wyTrue) == wyFalse)
	{
		tdata->m_exceltype.SetAs("String");
		retval.SetAs(data);
		return wyFalse;
	}

	retval.SetAs(data);
    return wyTrue;
}


wyBool
ExportToExcel::WriteFieldNames(ExportExcelData *exceldata)
{
	wyInt32         count = 0;
	wyString        fieldname;
    MYSQL_RES		*myres;
    wyChar          *utfstring = NULL;

    myres = exceldata->m_result;

	VERIFY(WriteRowHeader() == wyTrue);

	while(count < myres->field_count)
	{
		if(exceldata->m_selcol[count] == wyFalse)
		{
			count++;
			continue;
		}

		VERIFY(WriteCellHeader(wyTrue) == wyTrue);
		VERIFY(WriteDataHeader("String") == wyTrue);

		fieldname.SetAs(exceldata->m_fields[count].name);
        utfstring = ConvertToUtf8(fieldname);
        if(utfstring)
        {
		    fieldname.SetAs(utfstring);
		    if(WriteToFile(fieldname, wyTrue) == wyFalse)
				return wyFalse;
        }
		else
		{
			fieldname.SetAs("");
			VERIFY(WriteToFile(fieldname) == wyFalse);
		}

		VERIFY(CloseDataHeader() == wyTrue);
		VERIFY(CloseCellHeader() == wyTrue);

		count++;
	}
	VERIFY(CloseRowHeader() == wyTrue);

	return wyTrue;
}


wyChar *
ExportToExcel::ConvertToUtf8(wyString &ansistr)
{
	if(m_codepage.CompareI("utf8") == 0)
		return (wyChar *)ansistr.GetString();

	return (wyChar *)ansistr.GetString();
}

wyChar *
ExportToExcel::GetCellStyle()
{
	wyChar *temp;
	wyInt32 len;
	
	len = m_datatype.m_mysqltype.GetLength() ;

	if(len == 0)
		return "";
	
	temp = (wyChar *) m_datatype.m_mysqltype.GetString();

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Date") == 0)
		return " ss:StyleID=\"s21\"";

	if(stricmp(m_datatype.m_mysqltype.GetString(), "DateTime") == 0)
		return " ss:StyleID=\"s22\"";

	if(stricmp(m_datatype.m_mysqltype.GetString(), "Time") == 0)
		return " ss:StyleID=\"s23\"";

	if(stricmp(m_datatype.m_mysqltype.GetString(), "TimeStamp") == 0)
		return " ss:StyleID=\"s22\"";

	return "";
}

wyBool
ExportToExcel::WriteCellHeader(wyBool header)
{
	wyString cellheader;

	if(header == wyTrue)
		cellheader.Sprintf("\r\n    <ss:Cell  ss:StyleID=\"s27\">");
	else
		cellheader.Sprintf("\r\n    <ss:Cell%s>", GetCellStyle());
	
	return WriteToFile(cellheader);
}

wyBool
ExportToExcel::CloseDataHeader()
{
	wyString dataheader;

	dataheader.SetAs("</Data>");

	return WriteToFile(dataheader);
}

wyBool
ExportToExcel::WriteDataHeader(const wyChar *arg)
{
	wyString dataheader;

	dataheader.Sprintf("<Data ss:Type=\"%s\">", arg);

	return WriteToFile(dataheader);
}

wyBool
ExportToExcel::CloseCellHeader()
{
	wyString cellheader;

	cellheader.SetAs("</ss:Cell>");

	return WriteToFile(cellheader);
}

wyBool
ExportToExcel::WriteTableHeader()
{
	wyString tableheader;

	tableheader.SetAs("\r\n  <ss:Table>");

	return WriteToFile(tableheader);
}

wyBool
ExportToExcel::CloseTableHeader()
{
	wyString tableheader;

	tableheader.SetAs("\r\n  </ss:Table>");

	return WriteToFile(tableheader);
}

wyBool
ExportToExcel::WriteRowHeader()
{
	wyString rowheader;

	rowheader.SetAs("\r\n   <ss:Row>");
	
	return WriteToFile(rowheader);
}

wyBool
ExportToExcel::CloseRowHeader()
{
	wyString rowheader;

	rowheader.SetAs("\r\n   </ss:Row>");

	return WriteToFile(rowheader);
}

wyBool
ExportToExcel::WriteExcelHeaders()
{
	wyString header;

	header.SetAs("<?xml version=\"1.0\"?>\r\n<?mso-application progid=\"Excel.Sheet\"?>\r\n<Workbook\r\n  xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\r\n  xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\r\n  xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\r\n" );

    return WriteToFile(header);

}

wyBool 
ExportToExcel::WriteWorkSheets(wyChar *sheetname)
{
	wyString worksheetinfo;

	worksheetinfo.Sprintf("\r\n <Worksheet ss:Name=\"%s\">", sheetname);

	return WriteToFile(worksheetinfo);
}

wyBool 
ExportToExcel::CloseWorkSheets()
{
	wyString worksheetinfo;
	
	worksheetinfo.SetAs("\r\n </Worksheet>");

    return WriteToFile(worksheetinfo);
}

wyBool 
ExportToExcel::CloseHeader()
{
	wyString info;
	
	info.SetAs("\r\n</Workbook>");

    return WriteToFile(info);
}

wyBool 
ExportToExcel::WriteToFile(wyString &value, wyBool isdata)
{
    DWORD		dwbyteswritten;
	wyInt32		ret	= 0;
	wyString	error;
	wyChar*		encbuffer;
	wyInt32		lenptr = 0;
	if(!m_filename)
		return wyFalse;
	//wyWChar		    *wencbuffer;
	//wyUInt32		widelen = 0;

	//add data to a class level buffer and then to a file
	if(isdata == wyTrue)
	{
		//function to add text to buffer in an xml file handling characters.
		AddXMLToBuffer(&m_buffer, value.GetString(), wyFalse) ;
	}
	else
		m_buffer.Add(value.GetString());
		
	//if size of buffer is more
	if(m_buffer.GetLength() >= SIZE_8K)
	{
		if(m_charset != CPI_UTF8)
		{
			encbuffer =  m_buffer.GetAsEncoding(m_charset, &lenptr);
			ret = WriteFile(m_filename, encbuffer, lenptr, &dwbyteswritten, NULL);
		}
		else
		{
			ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
		}
		//ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
		m_buffer.Clear();

		if(!ret)
		{
			error.Sprintf(_("Error no : %d"), GetLastError());
			MessageBox(m_hwnd, error.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
			return wyFalse;
		}
	}

	return wyTrue;
}

wyBool
ExportToExcel::CheckValidDate(wyString &date, wyBool dateonly)
{
	wyString year, month, day, time, origdate;
	wyChar  *tempdata;
	wyInt32 yy, mm, dd;

	origdate.SetAs(date);

	year.SetAs(strtok((char *)date.GetString(), "- :"));       
	yy = atoi(year.GetString());

	tempdata = strtok(NULL, "- :");
	
	if(tempdata == NULL)
		return wyFalse;

	month.SetAs(tempdata);
	mm = atoi(month.GetString());
   
	tempdata = strtok(NULL, "- :");
	
	if(tempdata == NULL)
		return wyFalse;

	day.SetAs(tempdata);                            
	dd = atoi(day.GetString());

    if(yy == 0 || mm == 0 || dd == 0)
		return wyFalse;

	if(yy < 1900)
		return wyFalse;
        
    if((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31)
		return wyFalse;
    
    if((mm == 4 || mm == 6 || mm == 9 || mm == 11 ) && dd > 30)
		return wyFalse;

    if(mm == 02 && dd > 28) 
	{
        if((yy % 4) == 0)
        {	
            if(yy % 100 == 0 && yy % 400 != 0)
				return wyFalse;
        }
        else
            return wyFalse;
	}
	
	if(dateonly == wyFalse)
	{
		time.SetAs(strtok(NULL, "- :"));
		if(CheckValidTime(time) == wyFalse)
			return wyFalse;
	}

	return wyTrue;
}


wyBool
ExportToExcel::CheckValidTime(wyString &time)
{
	wyString hour, min, sec;
	wyInt32 hh;

	hour.SetAs(strtok((char *)time.GetString(), "- :"));       
	hh = atoi(hour.GetString());

	if(hh > 23)
		return wyFalse;
	
	return wyTrue;
}

void
ExportToExcel::ChangeToTimeStamp(wyString &data)
{
	wyString output;
	
	output.SetAs(data.GetString());
	data.SetAs(output.Substr(0, 4));
	data.AddSprintf("-%s", output.Substr(4, 2));
	data.AddSprintf("-%s", output.Substr(6, 2));
	data.AddSprintf(" %s", output.Substr(8, 2));
	data.AddSprintf(":%s", output.Substr(10, 2));
	data.AddSprintf(":%s", output.Substr(12, 2));

}
