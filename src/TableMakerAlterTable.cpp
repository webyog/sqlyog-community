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

#include "Include.h"
#include "TableMakerAlterTable.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "CommonHelper.h"
#include "SQLMaker.h"
#include "GUIHelper.h"


extern PGLOBALS pGlobals;

TableMakerAlterTable::TableMakerAlterTable()
{
	m_hwnd                  = NULL;
	m_currow                = 0;
	m_changed		        = wyFalse;
	m_lastcol               = 0;
	m_totcol                = 0;
    m_delcols               = NULL;
	// we assume that primary keys are not present
	
	m_flgprimarykeychange	= wyFalse;
	m_flgprimarykeypresent  = wyFalse;


}

wyBool 
TableMakerAlterTable::Create(MDIWindow *querywnd, Tunnel *tunnel, 
                     PMYSQL mysql, const wyChar *dbname, 
                     const wyChar *tablename)
{
    m_hwnd = NULL;
	// copy the default things which are needed for the window.
    SetInitialValues(querywnd, tunnel, mysql, dbname, tablename);
	
	//Post 8.01
	// repaint the scintilla window
	//RepaintTabModule();
	
	if(CreateDialogWindow())
	{
		SetFocus(querywnd->m_pcqueryobject->m_hwnd);
		return wyTrue;
	}
	else
		return wyFalse;
}

TableMakerAlterTable::~TableMakerAlterTable()
{
}

// Initualizes the objects TABLEADVPROPVALUES with the advanced values of the table
wyBool
TableMakerAlterTable::InitAdvProp()
{
	wyInt32		commentindex, index, val, collationindex, charindex;
    wyString	query, collation, criteria;
	wyString	retval,myrowstr;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyInt32		pos = 0, newpos = 0;
	wyBool		flag = wyFalse;
    wyChar      *charset = NULL;

	query.Sprintf("show table status from `%s` like '%s'", m_dbname.GetString(), m_tablename.GetString());
	
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	commentindex = GetFieldIndex(myres, "comment", m_tunnel, m_mysql);

	myrow = m_tunnel->mysql_fetch_row(myres);
	
	if(!myrow || !myrow[0] || !myrow[1])
	{	
		yog_message(GetGrid(), _(L"Could not read data for the table!"), pGlobals->m_appname.GetAsWideChar(), 
								MB_OK | MB_ICONINFORMATION | MB_HELP);

		m_tunnel->mysql_free_result(myres);
		return wyFalse;			
	}
	
	// if the table really exists.
	if(m_tablename.GetLength() == 0)
	{
		m_tunnel->mysql_free_result(myres);
		return wyFalse;
	}

	if(GetActiveWin()->m_ismysql41 == wyTrue)
	{
		collationindex = GetFieldIndex(myres, "collation", m_tunnel, m_mysql);
    
		if(collationindex != -1 && myrow[collationindex] != 0)
			collation.SetAs(myrow[collationindex]);

		m_advprop.m_collation.SetAs(collation.GetString());

		criteria.SetAs("_");

		//we extract the charset by collation info of the table
		//for eg, x_y is the collation then before "_" is the charset
		charindex = collation.Find(criteria.GetString(), 0);
		charset = (wyChar *)collation.GetString();
		
		if(charindex != -1)
		{
			charset[charindex ] = 0;
		}

		m_advprop.m_charset.SetAs(charset);
	}

    /// Autoincrement field
    commentindex = GetFieldIndex(myres, "auto_increment", m_tunnel, m_mysql);
    if(commentindex != -1 && myrow[commentindex] != 0)
        m_advprop.m_auto_incr.SetAs(myrow[commentindex]);

    index = GetFieldIndex(myres, "Create_options", m_tunnel, m_mysql);

    /// Copy type to the structure.
	m_advprop.m_type.SetAs(myrow[1]);
    
	/// Code added to strip off ";Innodb free" in comment string	
	myrowstr.SetAs(myrow[GetFieldIndex(myres, "comment", m_tunnel, m_mysql)]);
        newpos = myrowstr.GetLength();

	while(pos != -1)
	{	
		//in mysql 5.0 servers,Mysql is always adding InnoDB free space to comment
		pos = myrowstr.Find("InnoDB free:", pos);
        if(pos != -1)
		{
			flag = wyTrue;
			if(pos == 0)//if there is no user comment
				newpos = pos;
			else //if user comment is there it will add "; InnoDB free". 
				newpos = pos - 2;

			break;
		}
	}
	
	if(myrowstr.GetLength() >= newpos && flag == wyTrue && (m_advprop.m_type.CompareI("innodb") == 0))
		myrowstr.Strip(myrowstr.GetLength() - (newpos));
	
	/// Gets the comment
    m_advprop.m_comment.SetAs(myrowstr.GetString());
	
	/// Get Max row
    if((val =  GetValues(myrow[index], "max_rows", retval)) > 0)
       m_advprop.m_max_rows.SetAs(retval.GetString());

    /// Get the min rows
    if((val =  GetValues(myrow[index], "min_rows", retval)) > 0)
        m_advprop.m_min_rows.SetAs(retval.GetString());

    /// Get average row length
    if((val =  GetValues(myrow[index], "avg_row_length", retval)) >= 0)
        m_advprop.m_avg_row.SetAs(retval.GetString());

    /// Get checksum 
    if((val =  GetValues(myrow[index], "checksum", retval)) >= 0)
        m_advprop.m_checksum.SetAs(retval.GetString());

    /// Get delay key write
    if((val =  GetValues(myrow[index], "delay_key_write", retval)) >= 0)
        m_advprop.m_delay.SetAs(retval.GetString());

    /// Get the row format
    if((val =  GetValues(myrow[index], "row_format", retval)) >= 0)
        m_advprop.m_rowformat.SetAs(retval.GetString());

    m_advprop.m_changed = wyFalse;

	// dealloc result.
	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

wyBool
TableMakerAlterTable::FillInitData()
{
	MYSQL_RES		*myfieldres, *mykeyres;
	wyString        strcreate, query;
    wyInt32         ret;
    PTEDITCOLVALUE	temp;
	MDIWindow       *wnd = GetActiveWin();

	GetCreateTableString(m_tunnel, m_mysql, m_dbname.GetString(), m_tablename.GetString(), strcreate, query);

	// get the field information.
	query.Sprintf("show full fields from `%s`.`%s`", m_dbname.GetString(), m_tablename.GetString());
	
    myfieldres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
	
    if(!myfieldres)
    {
	    ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
    }

	// get the key information.
	query.Sprintf("show keys from `%s`.`%s`", m_dbname.GetString(), m_tablename.GetString());
    mykeyres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
	if(!mykeyres)
	{
        ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		m_tunnel->mysql_free_result(myfieldres);
		return wyFalse;
	}

    TraverseEachFieldRow(myfieldres, mykeyres);

	// now allocate buffer.
	m_totcol = (wyUInt32)myfieldres->row_count;
	m_delcols = new wyString[m_totcol + 1];

	m_tunnel->mysql_free_result(myfieldres);
	m_tunnel->mysql_free_result(mykeyres);

	// in the end we add one new rows so that one can easily add a row
	ret = CustomGrid_InsertRow(m_hwndgrid);
    			// now add the lparam to it.
	temp = new TEDITCOLVALUE;
	//memset(temp, 0, sizeof(TEDITCOLVALUE));
    InitTeditColValue(temp);

	temp->m_isnew = wyTrue;
	CustomGrid_SetRowLongData(GetGrid(), ret, (LPARAM)temp);
	CustomGrid_SetCurSelection(GetGrid(), 0, 0);

	return wyTrue;
}

void 
TableMakerAlterTable::InitTeditColValue(TEDITCOLVALUE *value)
{
    value->m_alter      = wyFalse;
    value->m_isnew      = wyFalse;
    value->m_onupdate   = wyFalse;
    value->m_oldname.Clear();
}

wyBool 
TableMakerAlterTable::TraverseEachFieldRow(MYSQL_RES *myfieldres, MYSQL_RES *mykeyres)
{
    wyInt32			ret, tempcount, count, fieldno = 0, index = 0, addcol = 0;
    wyInt32         pricolid, colnameid, typeindex, fieldindex, charindex;
    PTEDITCOLVALUE	temp;
    wyChar          *tempstr = NULL, *charset = NULL;
	wyString        strcreate, query, criteria;
    MYSQL_ROW		myfieldrow, mykeyrow;
	wyString		rowname, datatype;
	wyString		myrowstr, isnullstr, defaultstr;
	wyString		mykeyrowstr;
	wyString		mykeyrow1str;
	wyString		myfieldrowstr;
	wyBool			ismysql41 = IsMySQL41(m_tunnel, m_mysql);
        wyBool flag = wyTrue;

    if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;
    else
        addcol = 1;

	// now we traverse thru the field information and see what properties it has.
	while(myfieldrow = m_tunnel->mysql_fetch_row(myfieldres))
	{
		ret = CustomGrid_InsertRow(GetGrid());
		temp = new TEDITCOLVALUE;
	    temp->m_oldname.SetAs(myfieldrow[0], ismysql41);
		temp->m_isnew = wyFalse;
		temp->m_alter = wyFalse;
			
		datatype.SetAs(myfieldrow[1], ismysql41);

		if((stricmp(datatype.GetString(), "timestamp")== 0)&& (CheckForOnUpdate(strcreate, fieldno)))
			temp->m_onupdate = wyTrue;
		else
			temp->m_onupdate = wyFalse;

		CustomGrid_SetRowLongData(GetGrid(), ret, (LPARAM)temp);

		if(myfieldrow[0])
		{
			myrowstr.SetAs(myfieldrow[CNAME], ismysql41);
			CustomGrid_SetText(GetGrid(), ret, CNAME, myrowstr.GetString());
		}

        // for 6.06
        HandleDefaults(myfieldrow, myfieldres, datatype, ret, ismysql41);

		if(index)
		{
			fieldindex = GetFieldIndex(myfieldres, "collation", m_tunnel, m_mysql);
			if(fieldindex != NULL && myfieldrow[fieldindex] && stricmp(myfieldrow[fieldindex], "NULL") != 0)
			{
				myrowstr.SetAs(myfieldrow[fieldindex], ismysql41);

				if(myrowstr.GetString() && myrowstr.CompareI("NULL"))
					CustomGrid_SetText(GetGrid(), ret, COLLATION, myrowstr.GetString());

                criteria.SetAs("_");

                //we extract the charset by collation info of the table
                //for eg, x_y is the collation then before "_" is the charset
                charindex = myrowstr.Find(criteria.GetString(), 0);
                charset = (wyChar *)myrowstr.GetString();
                charset[charindex] = 0;
                CustomGrid_SetText(GetGrid(), ret, CHARSET, charset);
			}
		}
		
		myrowstr.SetAs(myfieldrow[1], ismysql41);

		if(strstr(myrowstr.GetString(), "unsigned"))
			CustomGrid_SetBoolValue(GetGrid(), ret, UNSIGNED + addcol, GV_TRUE);

		if(ismysql41 == wyFalse)
			if(strstr(myrowstr.GetString(), "binary"))
				CustomGrid_SetBoolValue(GetGrid(), ret, BINARY, GV_TRUE);

		if(strstr(myrowstr.GetString(), "zerofill"))
			CustomGrid_SetBoolValue(GetGrid(), ret, ZEROFILL + addcol, GV_TRUE);

		if(ismysql41&& myfieldrow[GetFieldIndex(myfieldres, "comment", m_tunnel, m_mysql)])
		{
			myrowstr.SetAs(myfieldrow[GetFieldIndex(myfieldres, "comment", m_tunnel, m_mysql)], ismysql41);
			CustomGrid_SetText(GetGrid(), ret, COMMENT, myrowstr.GetString());
		}
		if(myfieldrow[GetFieldIndex(myfieldres, "null", m_tunnel, m_mysql)])
		{
			myrowstr.SetAs(myfieldrow[GetFieldIndex(myfieldres, "null", m_tunnel, m_mysql)], ismysql41);
			if(!(strstr(myrowstr.GetString(), "YES")))
				CustomGrid_SetBoolValue(GetGrid(), ret, NOTNULL + addcol, GV_TRUE);
		}
		if(myfieldrow[GetFieldIndex(myfieldres, "extra", m_tunnel, m_mysql)])
		{
			myrowstr.SetAs(myfieldrow[GetFieldIndex(myfieldres, "extra", m_tunnel, m_mysql)], ismysql41);
			if(strstr(myrowstr.GetString(), "auto_increment"))
			{
				CustomGrid_SetBoolValue(GetGrid(), ret, AUTOINCR + addcol, GV_TRUE);
				m_autoinccheck = wyTrue;	
				m_autoincpresent = wyTrue;
				m_autorowname.SetAs(myfieldrow[CNAME], ismysql41);
			}
		}
				// check for primary.
		m_tunnel->mysql_data_seek(mykeyres, 0);
		pricolid = GetFieldIndex(mykeyres, "key_name", m_tunnel, m_mysql);
		colnameid = GetFieldIndex(mykeyres, "column_name", m_tunnel, m_mysql);

		while(mykeyrow = m_tunnel->mysql_fetch_row(mykeyres))
		{
			if(mykeyrow[pricolid])
				mykeyrowstr.SetAs(mykeyrow[pricolid], ismysql41);
			if(mykeyrow[colnameid])
				mykeyrow1str.SetAs(mykeyrow[colnameid], ismysql41);
			if(myfieldrow[0])
				myfieldrowstr.SetAs(myfieldrow[0], ismysql41);

			if(((strstr(mykeyrowstr.GetString(), "PRIMARY"))&&(stricmp(mykeyrow1str.GetString(), myfieldrowstr.GetString()))== 0))
			{
				CustomGrid_SetBoolValue(GetGrid(), ret, PRIMARY, GV_TRUE);
				CustomGrid_SetColumnReadOnly(GetGrid(), ret, NOTNULL + addcol, wyTrue);
				m_flgprimarykeypresent = wyTrue;
				m_prirowname.SetAs(myfieldrow[CNAME], IsMySQL41(m_tunnel, m_mysql));
				break;
			}
		}

		typeindex = GetFieldIndex(myfieldres, "type", m_tunnel, m_mysql);
		
		// now get the datatype but first alloc the buffer.
		if(myfieldrow[typeindex])
		{
			myfieldrowstr.SetAs(myfieldrow[typeindex], ismysql41);
			VERIFY(tempstr =(wyChar*)calloc(sizeof(wyChar), myfieldrowstr.GetLength() + 2));
		}

		count = 0;
		while(myfieldrow[typeindex][count] && myfieldrow[typeindex][count] != '(')
		{
			tempstr[count] = *(myfieldrow[1] + count);
			count++;
		}
		tempstr[count] = NULL;

		CustomGrid_SetText(GetGrid(), ret, DATATYPE, tempstr);
		TableMakerBase::SetValidation(ret, tempstr);

		tempcount = 0;
		// now we see if the previous row has ended or not then only w ein crease the index otherwise it
		// creates an error in filling data in the length when the field is of type date, text etc.
		if(myfieldrow[typeindex][count] != NULL)
			count++;

        

		while(myfieldrow[typeindex][count])
		{
            if(myfieldrow[typeindex][count] == ')' && flag == wyTrue)
                break;

			tempstr[tempcount] = *(myfieldrow[1] + count);

            if(*(myfieldrow[1] + count) == '\'')
            {
                if(flag == wyTrue)
                    flag = wyFalse;
                else
                    flag = wyTrue;
            }

			tempcount++;
			count++;
		}

		tempstr[tempcount] = NULL;
		CustomGrid_SetText(GetGrid(), ret, LENGTH, tempstr);

		// dealloc
		if(tempstr)
			free(tempstr);
		
		tempstr = NULL;
		fieldno++;
	}
    return wyTrue;
}

void
TableMakerAlterTable::HandleDefaults(MYSQL_ROW myfieldrow, MYSQL_RES *myfieldres, wyString &datatype, wyInt32 &ret, wyBool ismysql41)
{
    wyString defaultstr;

		if(myfieldrow[GetFieldIndex(myfieldres, "default", m_tunnel, m_mysql)])
		{
			defaultstr.SetAs(myfieldrow[GetFieldIndex(myfieldres, "default", m_tunnel, m_mysql)], ismysql41);
            if(defaultstr.GetLength() == 0)
            {
                if(myfieldrow[GetFieldIndex(myfieldres, "null", m_tunnel, m_mysql)])
		        {
                    if(strstr(datatype.GetString(), "varchar") != NULL || strstr(datatype.GetString(), "char") != NULL || 
                        strstr(datatype.GetString(), "varbinary") != NULL || strstr(datatype.GetString(), "set") != NULL 
                        || strstr(datatype.GetString(), "enum") != NULL || strstr(datatype.GetString(), "binary") != NULL)
                    {
                        defaultstr.SetAs("''");
                    }
                }
			}
            CustomGrid_SetText(GetGrid(), ret, DEFVALUE, defaultstr.GetString());
		}
}

wyBool
TableMakerAlterTable::DropColumn()
{
	wyInt32         ret;
	PTEDITCOLVALUE	temp;

	temp = (PTEDITCOLVALUE)CustomGrid_GetRowLongData(GetGrid(), m_currow);

	if(temp->m_isnew)
	{
		delete temp;
		return wyTrue;
	}

	ret = yog_message(GetGrid(), _(L"Do you really want to drop the field?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL | MB_HELP);

	switch(ret)
	{
	case IDNO:
		return wyFalse;

	case IDYES:
		break;
	}

	m_delcols[m_lastcol].SetAs(temp->m_oldname);
	m_lastcol++;
	// free the other buffer.
	delete temp;
	m_changed = wyTrue;

	return wyTrue;
}

wyBool
TableMakerAlterTable::AlterTable()
{
	wyString				addquery, modifyquery, primaryquery, completequery, otherprop, dropcols;
	wyString				autoincrquery;
	wyString				query;
	TableMakerAdvProperties	ctav;
    MYSQL_RES				*res;
	
	if(CustomGrid_GetRowCount(GetGrid()) == 0)
	{
		yog_message(GetGrid(), _(L"Please add atleast one row to the table"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	if(CreateQuery(completequery) == wyFalse)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	
	res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, completequery);

    if(!res && m_tunnel->mysql_affected_rows(*m_mysql) == -1)
		goto cleanup;	

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;

cleanup:
	ShowMySQLError(GetGrid(), m_tunnel, m_mysql, completequery.GetString());
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyFalse;
}

wyBool 
TableMakerAlterTable::AddPrimary(wyString &primary)
{
	wyInt32     count, rowcount, index = 0;
	wyBool		ret;
	PROWDATA	prowdata = NULL;
   
    primary.Clear();

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;

	rowcount = CustomGrid_GetRowCount(GetGrid());
	ret = AnyColumnPrimary(rowcount);

	if(m_flgprimarykeypresent == wyTrue)
		primary.Sprintf("\n   drop primary key");

	/* since there are no primary keys we have to return */
	if(!ret)
		return wyFalse;

	primary.AddSprintf("%s\n   add primary key(",(m_flgprimarykeypresent == wyTrue)?(", "):(""));

	for(count=0; count < rowcount; count++)
	{
		prowdata = CustomGrid_GetItemRow(GetGrid(), count);
		
		if(stricmp(prowdata->pszData[PRIMARY], GV_TRUE) == 0)
			primary.AddSprintf("`%s`, ", prowdata->pszData[CNAME]);

		prowdata = CustomGrid_FreeRowData(GetGrid(), prowdata);
	}

    primary.Strip(2);
    primary.Add(")");
	return wyTrue;
}

wyBool 
TableMakerAlterTable::AddAutoIncrement(wyString &primary)
{
	wyInt32     rowcount, index = 0;
	wyBool		ret;
//	PROWDATA	prowdata = NULL;
   
    primary.Clear();

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;

	rowcount = CustomGrid_GetRowCount(GetGrid());
	ret = AnyColumnAutoIncrement(rowcount);


	if(!ret)
		return wyTrue;

	return wyTrue;
}

wyInt32
TableMakerAlterTable::AddNewColumn(wyString &newcolumns)
{
	wyInt32         rowcount, count, addcol = 0;
    wyString        defvalue, datatype;
	PROWDATA		prowdata = NULL, pprevrowdata = NULL;
	PTEDITCOLVALUE	temp;
	wyInt32			index = 0, i = 1, j = 0, k = 0; //just a count
	wyInt32			blankrow = 0; // Holdes the number of black rows in the grid
	wyString		query;
	wyString		pszdatastr;
	wyString		prevfieldname, curfieldname; //Holds the field name
	wyString		datatypename; // Holds the datatype name
    wyString        fieldname;   // Holds the field name
	
    newcolumns.Clear();

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;
    else
        addcol = 1;
	
	rowcount = CustomGrid_GetRowCount(GetGrid());

	for(count = 0; count < rowcount; count++)
	{
		temp =(PTEDITCOLVALUE)CustomGrid_GetRowLongData(GetGrid(), count);
		prowdata = CustomGrid_GetItemRow(GetGrid(), count);

		if(temp->m_isnew)
		{
			if(prowdata->pszData[CNAME][0] == 0) 
			{
				temp->m_alter = wyFalse;
				prowdata = CustomGrid_FreeRowData(GetGrid(), prowdata);
				continue;
			}
			else
			{
				pszdatastr.SetAs(prowdata->pszData[DEFVALUE]);
				datatype.SetAs(prowdata->pszData[DATATYPE]);

				newcolumns.Add("\n   add column ");
				
				// first get the default value.
				if(pszdatastr.GetString())
                {
					if((stricmp(prowdata->pszData[NOTNULL + addcol], GV_FALSE)==0) 
					&& (stricmp(prowdata->pszData[DEFVALUE], "NULL") == 0))
					{
						defvalue.Sprintf("");
					}
					//If the datatype is timestamp and default string is 0, NULL or CURRENT_TIMESTAMP, then no need to add quotes
					else if((0 == stricmp(pszdatastr.GetString(), CURRENT_TIMESTAMP))
						|| (((stricmp(datatype.GetString(), "timestamp")) == 0)
						&& (((stricmp(pszdatastr.GetString(), "0")) == 0))))
					{
						defvalue.Sprintf("DEFAULT %s ", pszdatastr.GetString());
					}
                 
                    //6.06
					else
                    {
                        if(pszdatastr.Compare("''") != 0)
                        {
                            // First Search for `` pattern
                            if(pszdatastr.GetCharAt(0) == '`' && pszdatastr.GetCharAt(pszdatastr.GetLength() - 1) == '`' 
                              && pszdatastr.GetLength() == MAXLENWITHBACKTICKS)
                            {
                                if(pszdatastr.Find("''", 1))
				                   pszdatastr.SetAs("''");
                            }
                            wyChar *tbuff = GetEscapedValue(m_tunnel, m_mysql, pszdatastr.GetString());
                            defvalue.Sprintf("DEFAULT '%s' ", tbuff);
                        }
                        else
                            defvalue.Sprintf("DEFAULT '' ");
                    }
                }
				
				newcolumns.AddSprintf("`%s` %s ", prowdata->pszData[CNAME], prowdata->pszData[DATATYPE]);
				//datatype.SetAs(prowdata->pszData[DATATYPE]);	
											
				if(strlen(prowdata->pszData[LENGTH])>0)
				{	newcolumns.Strip(1);
					newcolumns.AddSprintf("(%s) ", prowdata->pszData[LENGTH]);
				}

				if (m_flgprimarykeychange == wyFalse)
					query.Add("AUTO_INCREMENT UNIQUE ");

				else
					query.Add("AUTO_INCREMENT ");

				if(IsMySQL41(m_tunnel, m_mysql))
				{
                    if(strlen(prowdata->pszData[CHARSET])> 0 && strcmp(prowdata->pszData[CHARSET], STR_DEFAULT) != 0)
						newcolumns.AddSprintf("CHARSET %s ", prowdata->pszData[CHARSET] );

                    if(strlen(prowdata->pszData[COLLATION])> 0 && strcmp(prowdata->pszData[COLLATION], STR_DEFAULT) != 0)
						newcolumns.AddSprintf("COLLATE %s ", prowdata->pszData[COLLATION] );

                    newcolumns.AddSprintf("%s%s%s%s%s%s", 
					((stricmp(prowdata->pszData[UNSIGNED + addcol], GV_TRUE)==0)?("UNSIGNED "):("")),
					((stricmp(prowdata->pszData[ZEROFILL + addcol], GV_TRUE)==0)?("ZEROFILL "):("")),
					(((stricmp(prowdata->pszData[BINARY], GV_TRUE)==0) && (IsMySQL41(m_tunnel,m_mysql) == wyFalse))?("BINARY "):("")),
					(((strlen(prowdata->pszData[DEFVALUE]))>0)?(defvalue.GetString()):("")),
					((stricmp(prowdata->pszData[NOTNULL + addcol], GV_TRUE)==0)?("NOT NULL "):("NULL ")),
					((stricmp(prowdata->pszData[AUTOINCR + addcol], GV_TRUE)==0)?(query.GetString()):(""))
					);    

					if(strlen(prowdata->pszData[COMMENT])> 0)
					{
						wyChar  *tbuff = (wyChar*)calloc((strlen(prowdata->pszData[COMMENT])*2 + 1), sizeof(wyChar));
			
						m_tunnel->mysql_real_escape_string(*m_mysql, tbuff, prowdata->pszData[COMMENT], strlen(prowdata->pszData[COMMENT]));
						newcolumns.AddSprintf("COMMENT '%s' ", tbuff);
			
						free(tbuff);
					}
					//else
						//newcolumns.Add(",\n");
				}
				//else
			//		newcolumns.Add(",\n");

                 
                else
                {
                    newcolumns.AddSprintf("%s%s%s%s%s%s", 
				    ((stricmp(prowdata->pszData[UNSIGNED + addcol], GV_TRUE)==0)?("UNSIGNED "):("")),
				    ((stricmp(prowdata->pszData[ZEROFILL + addcol], GV_TRUE)==0)?("ZEROFILL "):("")),
				    (((stricmp(prowdata->pszData[BINARY], GV_TRUE)==0) && (IsMySQL41(m_tunnel,m_mysql) == wyFalse))?("BINARY "):("")),
				    (((strlen(prowdata->pszData[DEFVALUE]))>0)?(defvalue.GetString()):("")),
				    ((stricmp(prowdata->pszData[NOTNULL + addcol], GV_TRUE)==0)?("NOT NULL "):("NULL ")),
				    ((stricmp(prowdata->pszData[AUTOINCR + addcol], GV_TRUE)==0)?(query.GetString()):(""))
				    );
                }
				// now remove the last ,
                //newcolumns.Strip(2);
				
				// now we add check if positional insert is needed or not.
				/*if(count == 0)
					newcolumns.Add(" first");*/
				
				j = k = count;
				i = 1;
				//Check the postion to add new field,removes all the blank row(s) in b/w finds the valid field 
				while(k <(rowcount - 1))
				{
					pprevrowdata = CustomGrid_GetItemRow(GetGrid(), k - i);
													
					if(pprevrowdata)
					{
						if(blankrow == j)
						{
							newcolumns.Add("first");
							break;
						}
						else if((strlen(pprevrowdata->pszData[CNAME])== 0)) //continue if no data is inserted in the row
						{
							blankrow++;
							i++;
							continue;
						}
						else if(j == 0)
						{
							newcolumns.Add("first");
							break;
						}
						
						else
						{
							newcolumns.AddSprintf("after `%s`", pprevrowdata->pszData[CNAME]);
							CustomGrid_FreeRowData(GetGrid(), pprevrowdata);
							break;
						}
					}
						k++;
				}

				newcolumns.Add(", ");	
			}
		}
	
		CustomGrid_FreeRowData(GetGrid(), prowdata);
	}

	if(newcolumns.GetCharAt(newcolumns.GetLength() - 2) == ',')
		newcolumns.Strip(2);

	if(newcolumns.GetLength()== 0)
		return 1;

	return 0;
}

//Handles if duplicate field name is present
void
TableMakerAlterTable::DuplicateFieldName()
{
	MessageBox(NULL, DUPLICATED_FIELDNAME, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

//Handles if datatype field not specified
wyBool
TableMakerAlterTable::DatatypeNotSpecified(wyInt32 length, const wyChar * fieldname)
{
	wyString fieldn, message;
	fieldn.SetAs(fieldname);
	if(length == 0)
	{
		message.Sprintf(DATATYPE_NOTSPECIFIED, fieldn.GetString());
		MessageBox(GetParent(m_hwndgrid), message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyTrue;
	}
	return wyFalse;
}

//Handles if length is not specifeied for varchar datatype
wyBool
TableMakerAlterTable::LengthNotSpecified(wyString *datatype, wyInt32 length)
{
	if((datatype->CompareI("varchar") == 0) && (length == 0))
	{ 
		MessageBox(GetParent(m_hwndgrid), VARCHAR_LENGTH_NOTSPECIFIED, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyTrue;
    }
	if((datatype->CompareI("varbinary") == 0) && (length == 0))
	{ 
		MessageBox(GetParent(m_hwndgrid), VARBINARY_LENGTH_NOTSPECIFIED, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyTrue;
    }
	
	return wyFalse;
}

wyBool 
TableMakerAlterTable::ModifyColumns(wyString &modifycolumns)
{
	wyInt32	        count, rowcount, collationlen, addcol = 0;
	wyString        defvalue, onupdate, collationstr;
    wyString        trim, charsetstr;
    wyChar          getname[SIZE_1024];
	PROWDATA		prowdata = NULL;
	PTEDITCOLVALUE	temp;
	wyInt32         index = 0;
    wyChar          *tbuff = NULL;

    modifycolumns.Clear();

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;
    else
        addcol = 1;
	
	rowcount = CustomGrid_GetRowCount(GetGrid());

	for(count = 0; count < rowcount; count++)
	{
		temp = (PTEDITCOLVALUE)CustomGrid_GetRowLongData(GetGrid(), count);
		prowdata = CustomGrid_GetItemRow(GetGrid(), count);

		if(temp->m_alter && !(temp->m_isnew))
		{
			modifycolumns.Add("\n   change");
			
			if((stricmp(prowdata->pszData[DATATYPE], "timestamp")== 0)&&(temp->m_onupdate))
				onupdate.SetAs(" on update CURRENT_TIMESTAMP");

			if(prowdata->pszData[DEFVALUE])
            {
				if((stricmp(prowdata->pszData[NOTNULL + addcol], GV_FALSE)==0) 
					&& (stricmp(prowdata->pszData[DEFVALUE], "NULL") == 0))
				{
					defvalue.Sprintf("");
				}
				//If the datatype is timestamp and default string is 0, NULL or CURRENT_TIMESTAMP, then no need to add quotes
				else if((0 == stricmp(prowdata->pszData[DEFVALUE], CURRENT_TIMESTAMP))
					|| (((stricmp(prowdata->pszData[DATATYPE], "timestamp")) == 0)
						&& (((stricmp(prowdata->pszData[DEFVALUE], "0")) == 0) )))
				{
					defvalue.Sprintf(" default %s%s", prowdata->pszData[DEFVALUE], onupdate.GetString());
				}
               	else
                {
                    if(strcmp(prowdata->pszData[DEFVALUE], "''") != 0)
                    {
                        if(prowdata->pszData[DEFVALUE][0] == '`' && prowdata->pszData[DEFVALUE][strlen(prowdata->pszData[DEFVALUE]) - 1] == '`'
                            && strlen(prowdata->pszData[DEFVALUE]) == MAXLENWITHBACKTICKS)
                        {
                            if(prowdata->pszData[DEFVALUE][1] == '\'' && prowdata->pszData[DEFVALUE][2] == '\'')
								strcpy(prowdata->pszData[DEFVALUE], "''");
                        }

                        tbuff = GetEscapedValue(m_tunnel, m_mysql, prowdata->pszData[DEFVALUE]);
					    defvalue.Sprintf(" default '%s'%s", tbuff, onupdate.GetString());
                    }
                    else
                        defvalue.Sprintf(" default ''%s", onupdate.GetString());
                }
			}
			
            /// We r trimming if the datatype is float or double
            strcpy(getname, prowdata->pszData[DATATYPE]);
            strtok(getname, " ");

            if(stricmp(getname, "float") == 0 || stricmp(getname, "double") == 0)
                strcpy(prowdata->pszData[DATATYPE], getname);


			modifycolumns.AddSprintf(" `%s` `%s` %s", temp->m_oldname.GetString(), 
                prowdata->pszData[CNAME], prowdata->pszData[DATATYPE]);
			
			if(strlen(prowdata->pszData[LENGTH]) > 0)
				modifycolumns.AddSprintf("(%s)", prowdata->pszData[LENGTH]);
			
            if(IsMySQL41(m_tunnel, m_mysql))
			{
                charsetstr.SetAs(prowdata->pszData[CHARSET]);
                collationstr.SetAs(prowdata->pszData[COLLATION]);

                collationlen = strlen(prowdata->pszData[CHARSET]);
                if(collationlen > 1 && charsetstr.Compare(STR_DEFAULT) != 0)
                    modifycolumns.AddSprintf(" character set %s", charsetstr.GetString());
                
				collationlen = strlen(prowdata->pszData[COLLATION]);
                if(collationlen > 1 && collationstr.Compare(STR_DEFAULT) != 0)
                    modifycolumns.AddSprintf(" collate %s", collationstr.GetString());

                modifycolumns.AddSprintf("%s%s%s%s%s%s", 
								((stricmp(prowdata->pszData[UNSIGNED + addcol], GV_TRUE)==0)?(" UNSIGNED"):("")),
								((stricmp(prowdata->pszData[ZEROFILL + addcol], GV_TRUE)==0)?(" ZEROFILL"):("")),
								(((stricmp(prowdata->pszData[BINARY], GV_TRUE)==0) && (IsMySQL41(m_tunnel, m_mysql) == wyFalse))?("BINARY "):("")),
								(((strlen(prowdata->pszData[DEFVALUE]))>0)?(defvalue.GetString()):("")),
								((stricmp(prowdata->pszData[NOTNULL + addcol], GV_TRUE)==0)?(" NOT NULL"):(" NULL ")),
								((stricmp(prowdata->pszData[AUTOINCR + addcol], GV_TRUE)==0)?(" AUTO_INCREMENT"):(""))
								);

				if(strlen(prowdata->pszData[COMMENT])> 0)
				{
					wyChar *tbuff = (wyChar*)calloc((strlen(prowdata->pszData[COMMENT])*2 + 1), sizeof(wyChar));

					m_tunnel->mysql_real_escape_string(*m_mysql, tbuff, prowdata->pszData[COMMENT], strlen(prowdata->pszData[COMMENT]));
					modifycolumns.AddSprintf(" comment '%s'", tbuff);
					free(tbuff);
				}
              
				modifycolumns.Add(", ");
				
			}
			
            else
            {
				modifycolumns.AddSprintf("%s%s%s%s%s%s", 
								((stricmp(prowdata->pszData[UNSIGNED + addcol], GV_TRUE)==0)?(" UNSIGNED"):("")),
								((stricmp(prowdata->pszData[ZEROFILL + addcol], GV_TRUE)==0)?(" ZEROFILL"):("")),
								(((stricmp(prowdata->pszData[BINARY], GV_TRUE)==0) && (IsMySQL41(m_tunnel, m_mysql) == wyFalse))?("BINARY "):("")),
								(((strlen(prowdata->pszData[DEFVALUE]))>0)?(defvalue.GetString()):("")),
								((stricmp(prowdata->pszData[NOTNULL + addcol], GV_TRUE)==0)?(" NOT NULL"):(" NULL")),
								((stricmp(prowdata->pszData[AUTOINCR + addcol], GV_TRUE)==0)?(" AUTO_INCREMENT"):(""))
								);

				modifycolumns.Add(", ");
            }

			
		}
		CustomGrid_FreeRowData(GetGrid(), prowdata);
	}

	if( modifycolumns.GetCharAt(modifycolumns.GetLength()-2) == ',')
        modifycolumns.Strip(2);

    if(modifycolumns.GetLength() == 0)
		return wyFalse;

	return wyTrue;
}

// create the drop column query.
wyInt32
TableMakerAlterTable::DropColumnQuery(wyString &dropcolumns)
{
	wyUInt32 count;
	wyBool	retvalue;

    dropcolumns.Clear();

	if(m_lastcol == 0)
		return 1;

	//Handles if all the rows are deleted and alter button is clicked
	retvalue = Dropallcolumns();

	if(retvalue == wyTrue)
		return -1;

	for(count = 0; count < m_totcol; count++)
	{
		if(m_delcols[count].GetLength())
			dropcolumns.AddSprintf("drop column `%s`, ", m_delcols[count].GetString());
	}

	if(dropcolumns.GetLength() && (dropcolumns.GetCharAt(dropcolumns.GetLength() - 2) == C_COMMA))
		dropcolumns.Strip(2);

    return 0;
}

wyBool
TableMakerAlterTable::Dropallcolumns()
{
	if((m_totcol == 1 ) || (m_lastcol == m_totcol))
	{
		MessageBox(GetParent(m_hwndgrid), DROP_ALLCOLUMNS, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return wyTrue;
	}
	return wyFalse;
}

wyBool
TableMakerAlterTable::AnyColumnPrimary(wyInt32 rowcount)
{
	wyInt32     count;
	wyWChar     strbool[10];
	wyInt32     index = 0;

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;

	for(count = 0; count < rowcount; count++)
	{
		CustomGrid_GetItemText(GetGrid(), count, PRIMARY, strbool);
		if(wcsicmp(strbool, TEXT(GV_TRUE)) == 0)
			return wyTrue;
	}
	
	return wyFalse;	
}

wyBool
TableMakerAlterTable::AnyColumnAutoIncrement(wyInt32 rowcount)
{
	wyInt32     count;
	wyWChar     strbool[10];
	wyInt32     index = 0;

	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;

	for(count = 0; count < rowcount; count++)
	{
		CustomGrid_GetItemText(GetGrid(), count, AUTOINCR , strbool);
		if(wcsicmp(strbool, TEXT(GV_TRUE)) == 0)
			return wyTrue;
	}
	
	return wyFalse;	
}

wyBool
TableMakerAlterTable::FreeResource()
{
	wyInt32         count, rowcount;
	PTEDITCOLVALUE	temp;
	
	rowcount = CustomGrid_GetRowCount(GetGrid());

	for(count = 0; count < rowcount; count++)
	{
		temp = (PTEDITCOLVALUE)CustomGrid_GetRowLongData(GetGrid(), count);
		delete temp;
	}
	
	return wyTrue;
}

// Function handles wrong entering of data in the grid, if the user has not entered the first
// column then I dont allow him to edit other columns. Also it is the first column and the previous row column has not been set then I dont
// allow to change the content either.
LRESULT
TableMakerAlterTable::OnGVNBeginLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyUInt32		row = wParam;
	wyUInt32		col = lParam;
	wyUInt32		totalrow = 0, index = 0, ret = 0;
    wyInt32         charsetlen = 0;
    HWND            gridhwnd = GetGrid();
    wyWChar         *selcharset = NULL;
    wyString        selcharsetstr;
	wyBool			ismysql41	= IsMySQL41(m_tunnel, m_mysql);
	wyWChar			*oldvalue;
	wyInt32			len;
	
	wyString		values, oldvalues;
	wyString		datatypestr;
		
	PTEDITCOLVALUE	temp=NULL, rowedited=NULL;

	// different column index if its 41
	if(IsMySQL41(m_tunnel, m_mysql))
		index = 1;

	// do validation
	if(ValidateOnBeginLabelEdit(row,col) == wyFalse)
		return wyFalse;
    if(ismysql41 == wyTrue)
	{
		if(col == CHARSETCOL)
		{
			CustomGrid_DeleteListContent(gridhwnd, CHARSETCOL);
			InitCharsetCol(gridhwnd);
		}
		else if(col == COLLATIONCOL)
		{
			charsetlen = CustomGrid_GetItemTextLength(gridhwnd, row, CHARSETCOL);
			selcharset = AllocateBuffWChar(charsetlen + 1);
			CustomGrid_GetItemText(gridhwnd, row, CHARSETCOL, selcharset);
			selcharsetstr.SetAs(selcharset);

			CustomGrid_DeleteListContent(gridhwnd, COLLATIONCOL);

			if(selcharsetstr.CompareI(STR_DEFAULT) == 0)
				InitCollationCol(gridhwnd);
			else if(charsetlen > 0)
				FilterCollationColumn(gridhwnd, row, charsetlen);

			free(selcharset); 
		}
	}
	
	//Saving the old value
	len = CustomGrid_GetItemTextLength(GetGrid(), wParam, lParam);
	oldvalue = AllocateBuffWChar(len + 1);
	CustomGrid_GetItemText(GetGrid(), wParam, lParam, oldvalue);
	m_oldvalue.SetAs(oldvalue);

	if(col == LENGTH)
	{
		m_changed = HandleEnumColumn(GetGrid(), row, col) == wyFalse ? m_changed : wyTrue ;
		/*CustomGrid_GetItemText(GetGrid(), row, DATATYPE, datatype);
		datatypestr.SetAs(datatype);
		if(stricmp(datatypestr.GetString(),"enum") == 0 || stricmp(datatypestr.GetString(),"set") == 0)
		{
			if(stricmp(datatypestr.GetString(),"enum") == 0)
				isenum = wyTrue;
			else 
				isenum = wyFalse;

			CustomGrid_SetColumnReadOnly(GetGrid(), row, LENGTH, wyTrue);
			values.SetAs(oldvalue);
			valuelist.Create(GetGrid(), &values, isenum);
			CustomGrid_SetText(GetGrid(), row, LENGTH, values.GetString());
			if(values.CompareI(m_oldvalue.GetString()) != 0)
				m_changed = wyTrue;

		}*/
	}
	
	// since its now edited we set the long value for the row to be edited so that we can generate 
	rowedited =(PTEDITCOLVALUE)CustomGrid_GetRowLongData(gridhwnd, row);
	rowedited->m_alter = wyTrue;

	// We set the primary key change flag to on if somebody has click on it.
	// We remove and recreate the PRIMARY KEY only when somebody has checked/unchecked it.
	if(PRIMARY == col)
		m_flgprimarykeychange = wyTrue;

	// if last row is edited then we attach a long pointer to it
	totalrow = CustomGrid_GetRowCount(gridhwnd);
	if(row ==(totalrow - 1))
	{
		temp = new TEDITCOLVALUE;
		//memset(temp, 0, sizeof(TEDITCOLVALUE));
        InitTeditColValue(temp);

		temp->m_isnew = wyTrue;

		ret = CustomGrid_InsertRow(gridhwnd);
		CustomGrid_SetRowLongData(gridhwnd, ret,(LPARAM)temp);
	}

	free(oldvalue);

	return wyTrue;
}

LRESULT
TableMakerAlterTable::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyChar			*newvalue;
	wyString		newvaluestr;
	
	//newvalue
	newvalue = (wyChar*)lParam;
	newvaluestr.SetAs(newvalue);
		
	//checking whether any change is made or not. If it is changed we are setting this variable.
	if(newvaluestr.Compare(m_oldvalue.GetString()) != 0)
		m_changed = wyTrue;

    ValidateOnEndLabelEdit(wParam, lParam);

	return 1;
}

LRESULT
TableMakerAlterTable::OnGVNEndAddNewRow(WPARAM wParam, LPARAM lParam)
{
	// We need to keep extra info with all rows.
	// Whenever a new row is added, grid calls a callback function, we create a new LPARAM value and
	// attach it to the row.
	PTEDITCOLVALUE		newrow;
	
	newrow	= new TEDITCOLVALUE;
	//memset(newrow, 0, sizeof(TEDITCOLVALUE));
    InitTeditColValue(newrow);

	newrow->m_isnew = wyTrue;

	CustomGrid_SetRowLongData(GetGrid(), wParam,(LPARAM)newrow);

	return wyTrue;
}

// function to free the del columns buffer.
wyBool
TableMakerAlterTable::ClearDelRowBuf()
{
	if(m_delcols)
	delete[] m_delcols;

	return wyTrue;
}

void  
TableMakerAlterTable::GetTitle(wyString& str)
{
	str.Sprintf(" Alter Table '%s' in '%s'", m_tablename.GetString(), m_dbname.GetString());
}

void
TableMakerAlterTable::HandleHelp()
{
	ShowHelp("Alter%20Table.htm");
}

wyBool
TableMakerAlterTable::Initialize()
{
	HICON hicon;

	::SetWindowText(GetDlgItem(GetDlgHwnd(), IDOK), _(TEXT("&Alter")));

	//Set icon for dialog	
	hicon = CreateIcon(IDI_ALTERTABLE);
	SendMessage(GetDlgHwnd(), WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
	
	SetFont();

	if(FillInitData() == wyFalse)
        return wyFalse;

	if(InitAdvProp() == wyFalse)
        return wyFalse;

    return wyTrue;
}

wyBool	
TableMakerAlterTable::ProcessOk()
{
	wyBool retvalue;

	CustomGrid_ApplyChanges(GetGrid());

	// We check if the change value for the dialog or the advance property has changed or not
	// If nothing is changed then only we generate and execute the alter statement otherwise
	// we just exit
	
	if(m_changed == wyFalse && m_advprop.m_changed == wyFalse)
	{
		yog_enddialog(GetDlgHwnd(), 0);
		return wyTrue;
	}

	if(m_changed == wyTrue || m_advprop.m_changed == wyTrue)
	{
 		retvalue = AlterTableValidation();

		if(retvalue == wyFalse)
			return wyFalse;

		if(!AlterTable())
			return wyFalse;
	}

	yog_message(GetDlgHwnd(), ALTERED_SUCESSFULLY, pGlobals->m_appname.GetAsWideChar(), MB_OK|MB_ICONINFORMATION);

	yog_enddialog(GetDlgHwnd(), 1);

	return wyTrue;
}

wyBool
TableMakerAlterTable::AlterTableValidation()
{
	wyUInt32    count = 0, j = 0; // just a counter
	wyUInt32	noofrows = 0;		// No of valide(data entered) rows in gride                
    wyUInt32    rowcount;           // number of rows in the grid
	PROWDATA	prevprowdata = NULL, curprowdata = NULL;
	wyInt32     length;				//Holds the field name length
	wyInt32     datatypelength;     //Holds the datetype length
	
    wyString	prevfieldname, curfieldname; //Holdes the field name
	wyString	datatypename;       //Holds the datatype name
	wyString	lengthfield;		//Holds the length of the datatype(length field)
	wyInt32		len;				//Holds the length field length
	wyString	defaultfield;
	wyInt32		deffieldlen;
	wyString	message;
	wyBool		retvalue = wyFalse;
	wyBool		typeflag = wyFalse;

	rowcount = CustomGrid_GetRowCount(GetGrid());

	// Handles if same field name specified
	for(count = 0; count < rowcount; count++)
	{
		prevprowdata = CustomGrid_GetItemRow(GetGrid(), count);
		prevfieldname.SetAs(prevprowdata->pszData[CNAME]);
		length = prevfieldname.GetLength();
		datatypename.SetAs(prevprowdata->pszData[DATATYPE]);
		datatypelength = datatypename.GetLength();
		lengthfield.SetAs(prevprowdata->pszData[LENGTH]);
		len = lengthfield.GetLength();
		defaultfield.SetAs(prevprowdata->pszData[DEFVALUE]);
		deffieldlen = defaultfield.GetLength();
		
		noofrows++;
		if((length == 0) && (datatypelength != 0)) //checks wheather field name is empty and datatype field is entered
		{
			MessageBox(GetParent(m_hwndgrid), FIELDNAME_NOTSPECIFEID, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return wyFalse;
		}
		if(length != 0)  // checks wheather field name is entered and datatype field is empty 
			retvalue = DatatypeNotSpecified(datatypelength, prevfieldname.GetString());

		if(retvalue == wyTrue)
			return wyFalse;

		if((deffieldlen != 0) && (defaultfield.CompareI("''") == 0))
		{
			if((datatypename.CompareI("varchar") == 0) || (datatypename.CompareI("char") == 0) || (datatypename.CompareI("varbinary") == 0) || (datatypename.CompareI("enum") == 0) || (datatypename.CompareI("set") == 0))
				typeflag = wyTrue;

			else
			{
				message.Sprintf("Default empty string ('') is not supported for the datatype (%s)", datatypename.GetString());
				MessageBox(GetParent(m_hwndgrid), message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
				return wyFalse;
			}
			
		}
	
		if(LengthNotSpecified(&datatypename, len) == wyTrue)// checks wheather length field is specified or not for varchar
			return wyFalse;


        for(j = count + 1; j < rowcount - 1; j++)
		{
			curprowdata = CustomGrid_GetItemRow(GetGrid(), j);
			curfieldname.SetAs(curprowdata->pszData[CNAME]);
			length = curfieldname.GetLength();
			if((prevfieldname.Compare(curfieldname) == 0) && (length != 0))
			{
				DuplicateFieldName(); //Handles if duplicate field name is present
				return wyFalse;
			}
		}
	}
	
	if(prevprowdata != NULL)
		CustomGrid_FreeRowData(GetGrid(), prevprowdata);

	if(curprowdata != NULL)
		CustomGrid_FreeRowData(GetGrid(), curprowdata);

	return wyTrue;
}
//Handles if no data is entered and still Alter button is clicked
wyBool	
TableMakerAlterTable::ProcessCancel()
{
	wyInt32		ret;

    CustomGrid_ApplyChanges(GetGrid());

	// Same as ProcessOK, we check if something is changed or not. 
	// If something is changed then we ask for confirmation from the user before exiting.
	if(m_changed == wyFalse && m_advprop.m_changed == wyFalse)
	{
		FreeResource();
		yog_enddialog(GetDlgHwnd(), 0);
		return wyTrue;
	}

	ret = yog_message(GetDlgHwnd(), _(TEXT("You will lose all changes. Do you want to quit?")), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2|MB_APPLMODAL | MB_HELP);

	switch(ret)
	{
	case IDYES:
		FreeResource();
		yog_enddialog(GetDlgHwnd(), 0);
		break;
	}

	return wyTrue;
}

wyBool	
TableMakerAlterTable::ProcessDelete()
{
	// we delete the selected row but if there are no rows selected we just return.
	wyInt32		selrow;

	if(CustomGrid_GetRowCount(GetGrid())== 1)
		return wyFalse;
	
	// First get the current selection
	selrow = LOWORD(CustomGrid_GetCurSelection(GetGrid()));
    
	if(selrow == -1)
		return wyFalse;

	CustomGrid_ApplyChanges(GetGrid());

	m_currow = selrow;
	
	if(DropColumn() == wyTrue)
	{
		// if drop was successful then we remove the row and select previous row
		CustomGrid_DeleteRow(GetGrid(), m_currow);
		if(selrow == 0)
		    // if th selected row was first then we just select the first row and column
			CustomGrid_SetCurSelection(GetGrid(), 0, 0);
		else
			CustomGrid_SetCurSelection(GetGrid(), --m_currow, 0);
	}

	return wyTrue;
}

wyBool	
TableMakerAlterTable::ProcessInsert()
{
    wyUInt32        newrowindex;
    TEDITCOLVALUE   *newrowlongvalue;

    CustomGrid_ApplyChanges(GetGrid(), wyTrue);

    newrowindex = InsertRowInBetween();

	newrowlongvalue = new TEDITCOLVALUE;
    InitTeditColValue(newrowlongvalue);
    //memset(newrowlongvalue, 0, sizeof(TEDITCOLVALUE));

    newrowlongvalue->m_isnew = wyTrue;

    CustomGrid_SetRowLongData(GetGrid(), newrowindex,(LPARAM)newrowlongvalue);

	return wyTrue;
}

wyBool	
TableMakerAlterTable::ProcessAdvanceProperties()
{
	TableMakerAdvProperties	    ctap;
	
    ctap.Create(GetDlgHwnd(), m_dbname.GetString(), m_tablename.GetString(), m_tunnel, GetMySQL(), &m_advprop, wyTrue);

	return wyTrue;
}

wyBool
TableMakerAlterTable::ProcessWMNCDestroy()
{
    ClearDelRowBuf();

    return wyTrue;
}
wyInt32 
TableMakerAlterTable::GetValues(wyChar *textrow, wyChar *tofind, wyString &retval)
{
	wyString    max;
	wyInt32     count = 0, valcount;
    wyChar      *temp;

	if(textrow && (temp = strstr(textrow, tofind)))
	{
		// move till the equal.
		for(count = 0; (temp[count] != NULL) && (temp[count] != C_EQUAL); count++);

		count++;

		for(valcount = 0; temp[count] != NULL && temp[count] != C_SPACE; count++, valcount++)
			max.AddSprintf("%c", temp[count]);

        retval.SetAs(max.GetString());

        if(max.GetLength())
            return atoi(max.GetString());
	}
	
	return -1;
}

wyBool
TableMakerAlterTable::CreateQuery(wyString &query)
{
	wyString				addquery, modifyquery, primaryquery, completequery, otherprop, dropcols;
	wyString				autoincrquery;
	TableMakerAdvProperties	ctav;
	wyInt32					retvalue;

	retvalue = AddNewColumn(addquery);
	if(retvalue == -1)
		return wyFalse;
	
	ModifyColumns(modifyquery);
	
	AddPrimary(primaryquery);
	
	AddAutoIncrement(autoincrquery);
	
	retvalue = DropColumnQuery(dropcols);
	if(retvalue == -1)
		return wyFalse;

    ctav.GetAdvPropString(m_tunnel, m_mysql, &m_advprop, otherprop);
	// allocate buffer for all the queries.
	if(IsMySQL5(m_tunnel, m_mysql))
	{
		// prepare the query.
		completequery.Sprintf("alter table `%s`.`%s` %s", m_dbname.GetString(), m_tablename.GetString(), (dropcols.GetLength())?(dropcols.GetString()):(""));
        completequery.Add((dropcols.GetLength() && addquery.GetLength())?(","):(""));
        completequery.Add((addquery.GetLength())?(addquery.GetString()):(""));
        completequery.Add(((dropcols.GetLength() || addquery.GetLength())&& modifyquery.GetLength())?(","):(""));
        completequery.Add((modifyquery.GetLength())?(modifyquery.GetString()):(""));
        completequery.Add(((dropcols.GetLength() || addquery.GetLength() || modifyquery.GetLength())&&primaryquery.GetLength() && primaryquery.GetCharAt(0) && m_flgprimarykeychange == wyTrue)?(","):(""));
	    completequery.Add((primaryquery.GetLength() && m_flgprimarykeychange == wyTrue)?(primaryquery.GetString()):(""));
        completequery.Add(((dropcols.GetLength() || addquery.GetLength() || modifyquery.GetLength() ||(primaryquery.GetLength() && primaryquery.GetCharAt(0) && m_flgprimarykeychange == wyTrue))&& otherprop.GetLength())?(","):(""));
        completequery.Add(otherprop.GetLength()?otherprop.GetString():(""));
	}
	else
	{
		// prepare the query.
		completequery.Sprintf(" alter table `%s`.`%s` ", m_dbname.GetString(), m_tablename.GetString(), (dropcols.GetLength())?(","):(""));
        completequery.Add((dropcols.GetLength())?(dropcols.GetString()):(""));
        completequery.Add((addquery.GetLength())?(","):(""));
        completequery.Add((addquery.GetLength())?(addquery.GetString()):(""));
        completequery.Add((modifyquery.GetLength())?(","):(""));
        completequery.Add((modifyquery.GetLength())?(modifyquery.GetString()):(""));
        completequery.Add((primaryquery.GetLength() && primaryquery.GetCharAt(0) && m_flgprimarykeychange)?(","):(""));
        completequery.Add((primaryquery.GetLength() && m_flgprimarykeychange)?(primaryquery.GetString()):(""));
        completequery.Add(otherprop.GetLength()?(","):(""));
        completequery.Add(otherprop.GetLength()?(otherprop.GetString()):(""));
	}
	
	if(autoincrquery.GetLength() != 0)
	{
		completequery.Add("\n   ");   
		completequery.Add(autoincrquery.GetString());
	}

	query.SetAs(completequery);
	return wyTrue;
}    

wyBool
TableMakerAlterTable::ShowPreview()
{
	QueryPreview	querypreview;
	wyString		query;
	wyInt32			retvalue;
	
	CustomGrid_ApplyChanges(m_hwndgrid);

	if(m_changed == wyTrue || m_advprop.m_changed == wyTrue)
	{
 		retvalue = AlterTableValidation();

		if(retvalue == wyFalse)
			return wyFalse;
	}
	else	
	{
		MessageBox(GetParent(m_hwndgrid), _(L"Nothing was altered."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
		return wyFalse;
	}

	CreateQuery(query);

	querypreview.Create(GetDlgHwnd(),(wyChar *)query.GetString());
	return wyTrue;
}
