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

#include "ExportAsSimpleSQL.h"
#include "MySQLVersionHelper.h"

#define  SIZE_8K   (8*1024)

ExportAsSimpleSQL::ExportAsSimpleSQL()
{
	m_escapeddata		= NULL;
    m_myres				= NULL;
    m_myrow				= NULL;
	m_rowlength			= NULL;
	m_isdatafromquery	= wyFalse;
}

ExportAsSimpleSQL::~ExportAsSimpleSQL()
{
	if(m_escapeddata)
		free(m_escapeddata);
}

wyBool
ExportAsSimpleSQL::StartSQLExport(ExportSQLData *data)
{
	DWORD       dwbyteswritten;
	wyInt32	    ret;
	wyString    error;
	
	Init(data);
	
	//first add in a buffer and then write to file
	if(WriteHeaders(data) == wyFalse)
		return wyFalse;

    if(WriteSetNames() == wyFalse)
        return wyFalse;

	if(data->m_structonly == wyTrue || data->m_structdata == wyTrue)
    {
		if(WriteCreateStatement(data) == wyFalse)
			return wyFalse;
    }

	if(data->m_dataonly == wyTrue || data->m_structdata == wyTrue)
    {
		if(WriteInsertStatement(data) == wyFalse)
			return wyFalse;
    }

	//if filebuffer is not empty then write to file
	if(m_buffer.GetLength() != 0)
	{
		ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
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
ExportAsSimpleSQL::WriteSetNames()
{
    wyString	query;
    wyBool      ismysql41 = IsMySQL41(m_tunnel, m_mysql);

    if(ismysql41 == wyTrue)
        query.SetAs("/*!40101 SET NAMES utf8 */;\n\n");

    return WriteToFile(query);
}

wyBool
ExportAsSimpleSQL::WriteHeaders(ExportSQLData *data)
{
	wyString	query, appname;

	/* set the app name for comment */
	appname.Sprintf("%s %s", pGlobals->m_appname.GetString(), APPVERSION);
    
    query.AddSprintf("/*\n%s\nMySQL - %s \n%s\n*/\r\n",
						 appname.GetString(),
						 (data->m_tabrec->m_pmdi->m_mysql)->server_version,
						 "*********************************************************************"
						 );

	return WriteToFile(query);
}

wyBool
ExportAsSimpleSQL::WriteCreateStatement(ExportSQLData *data)
{
	wyInt32			count = 0;
	wyString		createquery;
    wyBool          ret;
    MysqlDataType   retdatatype;
    wyBool          ismysql41 = IsMySQL41(m_tunnel, m_mysql);

    if(m_myres->fields == 0)
        return wyFalse;

    if(ismysql41 == wyTrue)
	    createquery.AddSprintf("create table `%s` (\r\n", data->m_tablename.GetString());
    else
    {
        createquery.AddSprintf("create table `%s` (\r\n", data->m_tablename.GetAsAnsi());
    }


	for(count = 0; count < m_myres->field_count; count ++)
	{
        if(*data->m_stopped == wyTrue)
        {
            SetWindowText(GetDlgItem(data->m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }
        
        if(data->m_selcol[count] == wyFalse)
            continue;

        ret = GetMySqlDataType(&retdatatype, m_myres->fields, count);

		createquery.AddSprintf("\t`%s` ", m_myres->fields[count].name); 
	    createquery.AddSprintf("%s ", retdatatype.m_mysqltype.GetString());

        if(SkipLength(retdatatype) == wyFalse)
        {
            if(m_myres->fields[count].length > m_myres->fields[count].max_length)
                createquery.AddSprintf("(%d),\r\n", m_myres->fields[count].length);
            else
                createquery.AddSprintf("(%d),\r\n", m_myres->fields[count].max_length);
        }
        else
            createquery.Add(",\r\n");
    }

    /// To strip 3 character from the end ('space', '\r' and '\n')
	createquery.Strip(3);
	createquery.Add("\r\n); \r\n");

    SendMessage(data->m_hwndmessage, WM_SETTEXT, 0, (LPARAM)_(L"Structure Exported Successfully"));
	return WriteToFile(createquery);
}

wyBool 
ExportAsSimpleSQL::SkipLength(MysqlDataType &retdatatype)
{
    if(retdatatype.m_mysqltype.CompareI("blob")         == 0 ||
       retdatatype.m_mysqltype.CompareI("date")         == 0 ||
       retdatatype.m_mysqltype.CompareI("timestamp")    == 0 || 
       retdatatype.m_mysqltype.CompareI("time")         == 0 || 
       retdatatype.m_mysqltype.CompareI("datetime")     == 0 || 
       retdatatype.m_mysqltype.CompareI("double")       == 0 ||
       retdatatype.m_mysqltype.CompareI("long")         == 0 ||
       retdatatype.m_mysqltype.CompareI("float")        == 0 ||
       retdatatype.m_mysqltype.CompareI("text")         == 0)

       return wyTrue;
    else
        return wyFalse;
}

wyBool 
ExportAsSimpleSQL::WriteToFile(wyString &buffer)
{
    DWORD       dwbyteswritten;
	wyInt32	    ret;
	wyString    error;
		
	
	m_buffer.Add(buffer.GetString());
	
	//if size of filebuffer is more then write to file
	if(m_buffer.GetLength() >=  SIZE_8K)
	{
		ret = WriteFile(m_filename, m_buffer.GetString(), m_buffer.GetLength(), &dwbyteswritten, NULL);
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

void
ExportAsSimpleSQL::Init(ExportSQLData *data)
{
	wyString	query;

	m_tunnel	= data->m_tunnel;
	m_filename	= data->m_filename;
	m_myres		= data->m_result;
	m_mysql		= &data->m_tabrec->m_pmdi->m_mysql;
	m_hwnd		= data->m_hwnd;

    VERIFY(m_myres->fields != 0);
}

wyBool
ExportAsSimpleSQL::WriteInsertStatement(ExportSQLData *data)
{
	wyInt32		    messagecount = 0, rowcount = 0, rowptr = 0;
	wyString	    value, messbuff, table;
    wyBool          ismysql41 = IsMySQL41(m_tunnel, m_mysql), retval = wyTrue;
	MYSQL_ROWS		*rowswalker = NULL;
	//HWND			hwndgrid;
	MDIWindow		*wnd;

	wnd = GetActiveWin();

	/*if(wnd &&
		wnd->GetActiveTabEditor() &&
		wnd->GetActiveTabEditor()->m_pctabmgmt &&
		wnd->GetActiveTabEditor()->m_pctabmgmt->m_pcdataviewquery &&
		wnd->GetActiveTabEditor()->m_pctabmgmt->m_pcdataviewquery->m_hwndgrid)
	{
		hwndgrid = wnd->GetActiveTabEditor()->m_pctabmgmt->m_pcdataviewquery->m_hwndgrid;
	}*/
	   
	m_tunnel->mysql_data_seek(m_myres,(my_ulonglong)0);

/*	if(data->m_tabrec->m_data)
		tmp = data->m_tabrec->m_data->m_data; */	

	while(1)
    {        	
		if(*data->m_stopped == wyTrue)
        {
            SetWindowText(GetDlgItem(data->m_hwnd, IDC_MESSAGE), _(L"Aborted by user"));
			return wyFalse;
        }
		
		//If exporting from result tab with edit mode 'data->m_tabrec->m_data' will not be NULL , this contains rows to export.
		//If export from 'Tabletab' or result tab in 'read only mode' use the normal MYSQL_RES pointer, to retrive the data
        if(!data->m_tabrec->m_rowarray->GetLength())
		{	
			if(m_isdatafromquery == wyTrue)
			{
				//If use mysql_useresult() then we should use this
				m_myrow = m_tunnel->mysql_fetch_row(m_myres);
			}

			else
			{
				SeekCurrentRowFromMySQLResult(m_myres, &rowswalker, m_tunnel, &m_myrow, &m_rowlength);
			}

			if(!m_myrow)
			{
				//SendMessage(hwndgrid, WM_SETREDRAW, TRUE, 0);
				break;
			}		
		}

		//In result tab with 'edit' mode or the result is edited
		else 
        {
            //bug fixed.http://code.google.com/p/sqlyog/issues/detail?id=494
			//no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            if(rowptr == data->m_tabrec->m_rowarray->GetLength())
                break;

            //no need to export unsaved row.. if it is a saved row then m_newrow =wyFalse
            
            
            if(data->m_tabrec->m_rowarray->GetRowExAt(rowptr)->IsNewRow())
			{
                rowptr++;
                break;
            }
            if(data->m_tabrec->m_modifiedrow >=0 && data->m_tabrec->m_modifiedrow == rowcount && data->m_tabrec->m_oldrow->IsNewRow() == wyFalse)
            {
                m_myrow = data->m_tabrec->m_oldrow->m_row;
            }
            else
            {
                m_myrow = data->m_tabrec->m_rowarray->GetRowExAt(rowptr)->m_row;
            }

            rowptr++;
        }

        if(ismysql41 == wyTrue)
            value.Sprintf("insert into `%s` ", data->m_tablename.GetString());
        else
        {
            //table.SetAs(data->m_tabrec->m_table.GetString());    
            value.Sprintf("insert into `%s` ", data->m_tablename.GetAsAnsi());
        }
		AddColumnNames(data, value);
		value.Add("values(");

		retval = AddValues(data, value);

		/*if(!data->m_tabrec->m_data)
		{
			SendMessage(hwndgrid, WM_SETREDRAW, TRUE, 0);
		}*/

		//Freeing the buffer that keeps the row-lengths
		if(m_rowlength)
		{
			free(m_rowlength);
			m_rowlength = NULL;
		}

		if(retval == wyFalse)
		{
			return wyFalse;
		}

		value.Strip(1); /// Strip 1 for the last ','
		value.Add(");\r\n");

		messbuff.Sprintf(_("  %d Rows Exported"), ++messagecount);
        SendMessage(data->m_hwndmessage, WM_SETTEXT, 0, (LPARAM)messbuff.GetAsWideChar());

		if(WriteToFile(value) == wyFalse)
			return wyFalse;
        rowcount++;
	}

	value.Add(";\r\n");

	if(m_rowlength)
	{
		free(m_rowlength);
		m_rowlength = NULL;
	}
	return wyTrue;
}

void
ExportAsSimpleSQL::AddColumnNames(ExportSQLData *data, wyString &value)
{
	wyInt32 count;

	value.Add("(");

	for(count = 0; count < m_myres->field_count; count++)
    {
		if(data->m_selcol[count] == wyTrue)
			value.AddSprintf("`%s`, ", m_myres->fields[count].name);
    }

	value.Strip(2); // Strip one for ',' and one for blank space
	value.Add(") ");
}

wyBool
ExportAsSimpleSQL::AddValues(ExportSQLData *data, wyString &value)
{
	wyInt32		count, last, lenrow;
	wyString	temp, resrow, buffer;
	wyInt32     *lengths = NULL; 
	wyInt32		escapeddatalen;
	wyBool		startflag = wyFalse;

	if(WriteToFile(value) == wyFalse)
		return wyFalse;

	//this condition is true if export from from result tab if result set is not edited
    if(!data->m_tabrec->m_rowarray)
	{
		if(m_isdatafromquery == wyTrue)
		{
			lengths = (wyInt32 *)sja_mysql_fetch_lengths(m_tunnel, m_myres);
		}
		else
		{
			lengths = (wyInt32 *)m_rowlength; 
		}
	}

	for(count = 0; count < data->m_result->field_count; count++)
	{
		if(data->m_selcol[count] == wyFalse)
			continue;

		if(startflag == wyTrue)
			buffer.Add(",");

        /// Check for NULL value or not
        if(!m_myrow[count])
        {
            buffer.Add("NULL");
            startflag = wyTrue;

            continue;
        }

		if(m_escapeddata)
			free(m_escapeddata);

				
		//This condion true if the result tab in table tab or edit mode or result set is edited		
		if(!lengths)		
		{
			resrow.SetAs(m_myrow[count]);
			lenrow = resrow.GetLength();
		}		
		else
		{			
			lenrow = lengths[count];		
		}		
				
		//It requires double buffer bcs the real_escape_string returns 2 char for special chars like '\'
		escapeddatalen = (lenrow * 2) + 1;
		m_escapeddata = AllocateBuff(escapeddatalen);

		last = sja_mysql_real_escape_string(m_tunnel, *m_mysql, m_escapeddata, m_myrow[count], lenrow);
		m_escapeddata[last] = 0;

		buffer.Add("'");
		startflag = wyTrue;

		buffer.Add(m_escapeddata);

		buffer.Add("'");
	}

	//Write to file
	WriteToFile(buffer);

	buffer.SetAs("");
	value.SetAs("");
	return wyTrue;
}