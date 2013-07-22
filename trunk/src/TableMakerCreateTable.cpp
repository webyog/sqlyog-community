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


#include <ocidl.h>
#include <unknwn.h>
#include <olectl.h>
#include <wingdi.h>
#include <malloc.h>

#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "Global.h"
#include "TableMakerCreateTable.h"
#include "CustGrid.h"
#include "CommonHelper.h"
#include "InputBoxGeneric.h"
#include "GUIHelper.h"

extern PGLOBALS pGlobals;

TableMakerCreateTable::TableMakerCreateTable()
{
	m_isinsertlist		= wyFalse;
	m_changed			= wyFalse;
	InitAdvProperties(&m_advprop);
}

TableMakerCreateTable::~TableMakerCreateTable() 
{
}

/// If table is created(atleast one) this function returns wyTrue else returns wyFalse
wyBool 
TableMakerCreateTable::Create(MDIWindow * querywindow, Tunnel * tunnel, 
                    PMYSQL mysql, const wyChar * dbname, const wyChar * tablename )
{
	// copy the default things which are needed for the window.
    SetInitialValues(querywindow,tunnel,mysql,dbname,"TableName1");
	
    //Post 8.01
	// to overcome one issue with scintilla where it does not get repainted due to optmization done inside, so we have to forcefully 
	// repaint it
	//RepaintTabModule();

    // if atleast one table is created or the table is altered successfully, refresh the object browser
	if(CreateDialogWindow())
	{
		// after returning from the dialog we set 
		::SetFocus( GetQueryWindow()->m_pcqueryobject->m_hwnd );

		return wyTrue;
	}

    // after returning from the dialog we set 
    ::SetFocus( GetQueryWindow()->m_pcqueryobject->m_hwnd );

	return wyFalse;
}

wyBool
TableMakerCreateTable::CreateTable()
{
	HCURSOR						hCursor;
	NewTableElem				*ptableinfo;
    MYSQL_RES					*res;

    wyInt32						ret;
	wyString					tbn;
    wyString					tabledefn;                  // table column info
 
	// if there are no rows, we return FALSE :)
    if (CustomGrid_GetRowCount(GetGrid()) == 0)
	{
		yog_message(GetGrid(), _(L"Please add atleast one row to the table"), pGlobals->m_appname.GetAsWideChar(), MB_OK|MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

    hCursor = ::GetCursor();
    ::SetCursor(LoadCursor(NULL,IDC_WAIT));
    ::ShowCursor(1);

	if(CreateQuery(tabledefn) == wyFalse)
		 goto setcursor;

    ret = my_query(GetActiveWin(), GetTunnel(), GetMySQL(), tabledefn.GetString(), tabledefn.GetLength());
	if( ret )
        goto error;

	/// If atleast one table created flag set to wyTrue
	if(m_ret == wyFalse)
		m_ret = wyTrue;

	tbn.SetAs(m_tablename.GetString());

	// inserting tablename into linkedlist
	if(m_isinsertlist == wyTrue)
	{
		ptableinfo = new NewTableElem(tbn.GetString());
		m_newtableslist.Insert(ptableinfo);
	}

	res = GetTunnel()->mysql_store_result(*GetMySQL(), false, false, GetActiveWin());
	if(!res && GetTunnel()->mysql_affected_rows(*GetMySQL()) == -1)
        goto error;

    //yog_message(GetDlgHwnd(), L"Table created successfully", pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION|MB_OK);


    GetActiveWin()->m_pcqueryobject->m_seltable.SetAs(m_tablename.GetString());
    GetActiveWin()->m_pcqueryobject->m_seldatabase.SetAs(m_dbname.GetString());

    ::SetCursor( hCursor );
    ::ShowCursor(1);

	return wyTrue;

error:

	ShowMySQLError(GetDlgHwnd(), GetTunnel(), GetMySQL(), tabledefn.GetString());

setcursor:

    ::SetCursor( hCursor );
    ::ShowCursor(1);

	return wyFalse;
}

wyBool
TableMakerCreateTable::CreateCompleteQuery(wyString& query, wyString& primary, wyString &autoincr)
{
	wyUInt32    i = 0;  // just a counter
	wyUInt32	noofrows = 0;		// No of valide(data entered) rows in gride
    wyUInt32    rowcount;           // number of rows in the grid
	PROWDATA	prowdata = NULL;
	wyInt32		ret;                //Holds the return value
	wyString	prevfieldname, curfieldname; //Holdes the field name
	wyString	datatypename;       //Holds the datatype name

    rowcount = CustomGrid_GetRowCount(GetGrid());
	
	for(;i<rowcount; i++)
	{
		// get all the text in a simplr character array
        // CustomGrid_GetItemRow allocates a 2-d chracter array, fills up the string data and sends.
        prowdata = CustomGrid_GetItemRow(GetGrid(), i);
		
		if(strlen(prowdata->pszData[0]) == 0) //continue if no data is inserted in the row
			continue;

		query.AddSprintf("\n   ");
	    
		// get all the details
        noofrows ++;
		ret = GetColumnAndDataType(query,prowdata);

		if(ret == 1)
			return wyFalse;
			
		if(IsMySQL41(GetTunnel(), GetMySQL()))
        {
            GetCharsetAndCollationValue(query,prowdata);
            GetExtraValues(query,prowdata);
            GetDefaultValue(query,prowdata);
            GetCommentValue(query,prowdata);
        }
		else
        {
            GetExtraValues(query,prowdata);
            GetDefaultValue(query,prowdata);
			query.Add(",");
        }
        		
        GetPrimaryKeyValue(primary,prowdata);
		//GetAutoIncrementKeyValue(autoincr,prowdata); 
		
        // ask the grid to free the resource that has been allocated by grid
        prowdata = CustomGrid_FreeRowData(GetGrid(), prowdata);
	}

	// now check whether there is any data in the primary, index and unique field.
    // if yes then we strip off the last 2 characters as it will have an extra ,
    // otherwise we strip the last , from the table defnition
    if(primary.GetCharAt(primary.GetLength()-2) == ',') 
    {
       primary.Strip(2);
       primary.Add(")");            // add the closing bracket.
    }
    else 
    {
        primary.Strip(primary.GetLength());
        query.Strip(2);
    }
	return wyTrue;
}

//Handles if duplicate field name is present
void
TableMakerCreateTable::DuplicateFieldName()
{
	MessageBox(NULL, DUPLICATED_FIELDNAME, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

wyInt32       
TableMakerCreateTable::GetColumnAndDataType(wyString& query, PROWDATA prowdata)
{
	wyString		dbdefaultvalstr1;  //Holds the datatype 
	wyString		dbdefaultvalstr2;  //Holds the field name
	wyString		datatype;		  // Holdes the current datatype selected
	wyString		fieldname;		  // Holdes the current field selected
		
	dbdefaultvalstr1.SetAs(prowdata->pszData[DATATYPE]);
	
	dbdefaultvalstr2.SetAs(prowdata->pszData[CNAME]);

	query.AddSprintf("`%s` %s ", dbdefaultvalstr2.GetString(), dbdefaultvalstr1.GetString());

    // get the datatype length
    dbdefaultvalstr1.SetAs(prowdata->pszData[LENGTH]);
	
	if(dbdefaultvalstr1.GetString() &&  strlen(dbdefaultvalstr1.GetString())) 
	{	query.Strip(1);
		query.AddSprintf("(%s) ", dbdefaultvalstr1.GetString());
	}

	return 0;
}
//Handles id Datatype field is empty
wyBool
TableMakerCreateTable::DatatypeNotSpecified(wyInt32 length, const wyChar * fieldname)
{
	wyString fieldn, message;
	fieldn.SetAs(fieldname);
	if(length == 0)
	{
		message.Sprintf(_("Datatype not specified for field name (%s)"), fieldn.GetString());
		MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return wyTrue;
	}
	return wyFalse;
}

//Handles if length is not specifeied for varchar datatype
wyBool
TableMakerCreateTable::LengthNotSpecified(wyString *datatype, wyInt32 length)
{
	if((datatype->CompareI("varchar") == 0) && (length == 0))
	{ 
		MessageBox(m_hwnd, VARCHAR_LENGTH_NOTSPECIFIED, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return wyTrue;
    }
	if((datatype->CompareI("varbinary") == 0) && (length == 0))
	{ 
		MessageBox(m_hwnd, VARBINARY_LENGTH_NOTSPECIFIED, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return wyTrue;
    }
	return wyFalse;
}
void
TableMakerCreateTable::GetDefaultValue(wyString& query, PROWDATA prowdata)
{
    // add the deafult value.
    // if a user has selected CURRENT_TIMESTAMP then we dont need to put in ''
	wyString	dbdefaultvalstr, datatype;
	wyInt32     addcol = 0;
	
	if(!IsMySQL41(GetTunnel(), GetMySQL()))
	{
		addcol = 1;
	}

	dbdefaultvalstr.SetAs(prowdata->pszData[DEFVALUE]);
	datatype.SetAs(prowdata->pszData[DATATYPE]);

	if(strlen(dbdefaultvalstr.GetString()) == 0)
        return;

    
	if((stricmp(prowdata->pszData[NOTNULL + addcol], GV_FALSE)==0) 
		&& ((stricmp(dbdefaultvalstr.GetString(), "NULL")) == 0))
	{
		query.Add("");
	}
	else
	{
		query.Add("DEFAULT ");

		//If the datatype is timestamp and default string is 0, NULL or CURRENT_TIMESTAMP, then no need to add quotes
		if(((stricmp(dbdefaultvalstr.GetString(), CURRENT_TIMESTAMP)) == 0)
			|| (((stricmp(datatype.GetString(), "timestamp")) == 0)
					&& (((stricmp(dbdefaultvalstr.GetString(), "0")) == 0))))
		{
			if(((stricmp(datatype.GetString(), "timestamp")) == 0)
					&& (((stricmp(dbdefaultvalstr.GetString(), "NULL")) == 0)))
			{
				query.AddSprintf("%s %s", dbdefaultvalstr.GetString(), dbdefaultvalstr.GetString());
			}
			else
			{
				query.AddSprintf("%s ", dbdefaultvalstr.GetString());
			}
		}
		else
		{
			HandleDefaults(query, dbdefaultvalstr);
		}
	}

}
void
TableMakerCreateTable::HandleDefaults(wyString &query, wyString &dbdefaultvalstr)
{
    wyChar *tbuff =  NULL;

   if(dbdefaultvalstr.Compare("''") != 0)
   {
        if(dbdefaultvalstr.GetCharAt(0) == '`' && dbdefaultvalstr.GetCharAt(dbdefaultvalstr.GetLength() - 1) == '`' 
            && dbdefaultvalstr.GetLength() == MAXLENWITHBACKTICKS)
        {
            if(dbdefaultvalstr.Find("''", 1))
                dbdefaultvalstr.SetAs("''");
        }
        tbuff = GetEscapedValue(m_tunnel, m_mysql, dbdefaultvalstr.GetString());
        query.AddSprintf("'%s' ", tbuff);
   }
   else
       query.AddSprintf("'' ");
}
void        
TableMakerCreateTable::GetExtraValues(wyString& query, PROWDATA prowdata)
{

    // Add the extra values like NOT NULL, AUTOINCREMENT etc.
    // Customgrid sends BOOL value as GV_TRU
	wyUInt32    index = 0, rowcount, count = 0, pkcount = 0, addcol = 0;
    HWND        hwnd;
	wyString		dbdefaultvalstr;

	if(IsMySQL41(GetTunnel(), GetMySQL()))
		index = 1;
    else
        addcol = 1;
		
    if(stricmp(prowdata->pszData[UNSIGNED + addcol], GV_TRUE) == 0)
        query.Add("UNSIGNED ");

    if(stricmp(prowdata->pszData[ZEROFILL + addcol], GV_TRUE) == 0)
        query.Add("ZEROFILL ");

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
		if(stricmp(prowdata->pszData[BINARY], GV_TRUE) == 0)
			query.Add("BINARY ");

    if(stricmp(prowdata->pszData[NOTNULL + addcol], GV_TRUE) == 0)
        query.Add("NOT NULL ");

    if(stricmp(prowdata->pszData[AUTOINCR + addcol], GV_TRUE) == 0)
    {
		query.Add("AUTO_INCREMENT ");
        
        if(stricmp(prowdata->pszData[PRIMARY], GV_FALSE) == 0)
            query.Add("UNIQUE ");
        else
        {
            hwnd = GetGrid();
            rowcount = CustomGrid_GetRowCount(hwnd);

            for(count = 0; count < rowcount; count++)
            {
                if(CustomGrid_GetBoolValue(hwnd, count, PRIMARY))
                    pkcount++;

                if(pkcount > 1)
                    break;
            }

            if(pkcount != 1)
                query.Add("UNIQUE ");
        }
    }
}

void        
TableMakerCreateTable::GetCharsetAndCollationValue(wyString& query, PROWDATA prowdata)
{
	wyString		collationstr, charsetstr;

	collationstr.SetAs(prowdata->pszData[COLLATION]);
    charsetstr.SetAs(prowdata->pszData[CHARSET]);

    if(charsetstr.GetLength() > 0 && charsetstr.Compare(STR_DEFAULT) != 0)
		query.AddSprintf("CHARSET %s ", charsetstr.GetString());
    if(collationstr.GetLength() > 0 && collationstr.Compare(STR_DEFAULT) != 0)
		query.AddSprintf("COLLATE %s ", collationstr.GetString());
}

void        
TableMakerCreateTable::GetCommentValue(wyString& query, PROWDATA prowdata)
{
    wyUInt32        index=0;
	wyString		dbdefaultvalstr;

	dbdefaultvalstr.SetAs(prowdata->pszData[COMMENT]);

    if(IsMySQL41(GetTunnel(), GetMySQL()))
       index = 1;

    // get comments but we will need to real_escape as somebody can put ' in a comment
	if(strlen(dbdefaultvalstr.GetString()))
	{
		wyChar * commentbuff = GetEscapedValue(m_tunnel, m_mysql, dbdefaultvalstr.GetString());
        query.AddSprintf("COMMENT '%s', ", commentbuff);
        free(commentbuff);
		return;
	}
        
	query.Add(", ");

}

void        
TableMakerCreateTable::GetPrimaryKeyValue(wyString& query, PROWDATA prowdata)
{
    wyUInt32        index=0;
	wyString		dbdefaultvalstr;
	wyString		dbdefaultvalstr1;

    if(IsMySQL41(GetTunnel(), GetMySQL()))
        index = 1;
	
	dbdefaultvalstr.SetAs(prowdata->pszData[PRIMARY]);

	if(stricmp(dbdefaultvalstr.GetString(), GV_TRUE) == 0)
	{
		dbdefaultvalstr1.SetAs(prowdata->pszData[0]);
		query.AddSprintf("`%s`, ", dbdefaultvalstr1.GetString());
	}
}

void        
TableMakerCreateTable::GetAutoIncrementKeyValue(wyString& query, PROWDATA prowdata)
{
    wyUInt32        index=0;
	wyString		dbdefaultvalstr;
	wyString		dbdefaultvalstr1;

    if(IsMySQL41(GetTunnel(), GetMySQL()))
       index = 1;
	
	dbdefaultvalstr.SetAs(prowdata->pszData[0]);
	dbdefaultvalstr1.SetAs(prowdata->pszData[AUTOINCR]);
	
	if(stricmp(dbdefaultvalstr1.GetString(), GV_TRUE) == 0)
        query.AddSprintf("%s`(`%s`) ", dbdefaultvalstr.GetString(), dbdefaultvalstr.GetString());
}

LRESULT
TableMakerCreateTable::OnGVNBeginLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyUInt32		row = wParam;
	wyUInt32		col = lParam;
	wyUInt32		totalrows = 0;
    wyInt32         charsetlen = 0, lentxt = 0;
	HWND			gridhwnd;
    wyWChar         *selcharset = NULL;
    wyString        selcharsetstr;
	wyBool			ismysql41 = IsMySQL41(m_tunnel, m_mysql);

	ValueList		valuelist(m_mysql);
	wyString		values;
	wyWChar			datatype[256] = {0}, *value = NULL;
	wyString		datatypestr;
	wyBool			isenum;
    	
	gridhwnd = GetGrid();
	// do validation
	if(ValidateOnBeginLabelEdit(row,col) == wyFalse)
		return FALSE;
    
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
			selcharset  = AllocateBuffWChar(charsetlen + 1);
			CustomGrid_GetItemText(gridhwnd, row, CHARSETCOL, selcharset);
			selcharsetstr.SetAs(selcharset);
    
			CustomGrid_DeleteListContent(gridhwnd, COLLATIONCOL);

			if(selcharsetstr.CompareI(STR_DEFAULT) == 0)
				InitCollationCol(gridhwnd);
   
			else if(charsetlen > 0)
			{
				//if a Charset is selected from the dropdown.. related collations will be filtered and displayed
				FilterCollationColumn(gridhwnd, row, charsetlen);
			}

			free(selcharset); 
		}
	}

	if(col == LENGTH)
	{
		CustomGrid_GetItemText(GetGrid(), row, DATATYPE, datatype);
		datatypestr.SetAs(datatype);
		if(stricmp(datatypestr.GetString(),"enum") == 0 || stricmp(datatypestr.GetString(),"set") == 0)
		{
			if(stricmp(datatypestr.GetString(),"enum") == 0)
				isenum = wyTrue;
			else 
				isenum = wyFalse;
			CustomGrid_SetColumnReadOnly(GetGrid(), row, LENGTH, wyTrue);

			lentxt = CustomGrid_GetItemTextLength(GetGrid(), row, LENGTH);

			value = AllocateBuffWChar(lentxt + 1);
			if(!value)
			{
				return FALSE;
			}
						
			CustomGrid_GetItemText(GetGrid(), row, LENGTH, value);
			values.SetAs(value);
			free(value);
			
			valuelist.Create(GetGrid(), &values, isenum);
			CustomGrid_SetText(GetGrid(), row, LENGTH, values.GetString());
		}
	}

	// If its the last row in the grid to be edited then we add a row automatically
	totalrows = CustomGrid_GetRowCount(gridhwnd);

	if(row == (totalrows - 1))
		CustomGrid_InsertRow(gridhwnd);

	return TRUE;
}

LRESULT
TableMakerCreateTable::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
	m_changed = wyTrue;

	// Validate stuff
	ValidateOnEndLabelEdit(wParam,lParam);

	return 1;
}

wyBool	
TableMakerCreateTable::ProcessOk()
{
	wyInt32             ret;
    wyBool              tablenamesuccess;           
    InputBoxGeneric     tablename;
    MDIWindow           *wnd;
	wyBool				retvalue;
	
	// We call save table to create the actual table and then ask if the user wants to create another table
	// If the user says then we reinitialize the grid otherwise we exit

    CustomGrid_ApplyChanges(GetGrid());

    retvalue = CreateTableValidation();
	
	if(retvalue == wyFalse)
		return wyFalse;

	tablenamesuccess = tablename.Create(GetDlgHwnd(), _("Create a new table"),
                                        _("Enter new table name"),
                                        _("Please provide table name"),
                                        m_tablename);

    if(tablenamesuccess == wyFalse)
        return wyFalse;

	// create create table string and ask user confirmation
    ret = CreateTable();

	if(ret)
	{
		ret = yog_message(GetGrid(), 
							_(L"Table created successfully. Do you want to add more tables?"), 
							pGlobals->m_appname.GetAsWideChar(), 
							MB_YESNO | MB_ICONQUESTION) ;

		switch ( ret )
		{
		case IDYES:
			ReInitialize();
			break;

		case IDNO:
            {
                VERIFY(wnd = GetQueryWindow());
			    wnd->m_pcqueryobject->m_seldatabase.SetAs(m_dbname.GetString());
			    wnd->m_pcqueryobject->m_seltable.SetAs(m_tablename.GetString());
			    yog_enddialog(GetDlgHwnd(),1);
            }
			break;
		}
	}

	return wyTrue;
}

wyBool
TableMakerCreateTable::CreateTableValidation()
{

	wyUInt32    count = 0, j = 0, i = 0; // just a counter
	wyUInt32	noofrows = 0;		// No of valide(data entered) rows in gride                
    wyUInt32    rowcount;           // number of rows in the grid
	PROWDATA	prevprowdata = NULL, curprowdata = NULL, prowdata = NULL;
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

		if((length == 0) && (datatypelength != 0)) //checks whether field name is empty and datatype field is entered
		{
			MessageBox(NULL, FIELDNAME_NOTSPECIFEID, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
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
				message.Sprintf(_("Default empty string ('') is not supported for the datatype (%s)"), datatypename.GetString());
				MessageBox(NULL, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
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
	
	for(;i<rowcount; i++)
	{
		// get all the text in a simplr character array
        // CustomGrid_GetItemRow allocates a 2-d chracter array, fills up the string data and sends.
        prowdata = CustomGrid_GetItemRow(GetGrid(), i);

		if(strlen(prowdata->pszData[0]) == 0)
		{
			continue;
		}
		noofrows++;
	}
	//if all the rows are empty and create table is clicked
	if(noofrows == 0)
    {
		MessageBox(m_hwnd, EMPTY_ROWS, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);  
		return wyFalse;
	}

	return wyTrue;
}

wyBool	
TableMakerCreateTable::ProcessCancel()
{
	wyBool ret;

	CustomGrid_ApplyChanges(GetGrid() );

    // If a user has changed anything then we just ask him for confirmation.
    // Otherwise we close the dialog.
	ret = IsChanged();
	if (ret != wyTrue)
    {
		if(m_ret == wyTrue)
			yog_enddialog(GetDlgHwnd(), 1);

		else
			yog_enddialog(GetDlgHwnd(), 0);

		return wyTrue;
    }

    // ask the user confirmation
    if(yog_message(GetGrid(), 
                   _(L"You will lose all changes. Do you want to quit?"), 
                   pGlobals->m_appname.GetAsWideChar(), 
                   MB_ICONQUESTION |MB_YESNO|MB_DEFBUTTON2|MB_APPLMODAL|MB_HELP) == IDYES) 
    
    {
		/// If atleast one table is created return 1 else return 0.
		if(m_ret == wyTrue)
			VERIFY(yog_enddialog(GetDlgHwnd(), 1));

		else
			VERIFY(yog_enddialog(GetDlgHwnd(), 0));
    }

	return wyTrue;
}

wyBool	
TableMakerCreateTable::ProcessDelete()
{
    // Method deletes the selected row from the grid
    wyInt32        ret;
    wyInt32        currow;
				
    // We exit if the number of row is < 1.
    if (CustomGrid_GetRowCount(GetGrid()) == 1)
        return wyTrue;

    // When a user deletes a row, we select the previous row so we keep the current row in
    // a variable before deleting and select the previous row there. Thus we do --currow :)
    ret = CustomGrid_GetCurSelection(GetGrid());
    currow = LOWORD(ret);

	CustomGrid_ApplyChanges(GetGrid());

	CustomGrid_DeleteRow(GetGrid(), currow);
	//CustomGrid_SetCurSelection(GetGrid(), max(0,--currow), 0);        // make sure that you are not going below 0
    ::SetFocus(GetGrid());

	return wyTrue;
}

wyBool	
TableMakerCreateTable::ProcessInsert()
{
    CustomGrid_ApplyChanges(GetGrid(), wyTrue);

    InsertRowInBetween();
	
    return wyTrue;
}

wyBool	
TableMakerCreateTable::ProcessAdvanceProperties()
{
	TableMakerAdvProperties	    ctap;
	
    ctap.Create(GetDlgHwnd(), NULL, NULL, m_tunnel, GetMySQL(), &m_advprop, wyFalse);

	return wyTrue;
}

wyBool
TableMakerCreateTable::ProcessWMNCDestroy()
{
    return wyTrue;
}

void  
TableMakerCreateTable::GetTitle(wyString& str)
{
	str.Sprintf(_(" New table in '%s'"), m_dbname.GetString());
}

void
TableMakerCreateTable::HandleHelp()
{
	ShowHelp("Create%20Table.htm");
}

void
TableMakerCreateTable::CreateInitRows()
{
	wyUInt32		i;					// just a counter
	wyUInt32		rowperpage;			// how many rows can be displayed in the page

	rowperpage = CustomGrid_RowPerPage (GetGrid());

	for(i=0; i< DEFAULTNUMROWS; i++)
		CustomGrid_InsertRow(GetGrid());

}

void
TableMakerCreateTable::ReInitialize()
{
	CustomGrid_DeleteAllRow(GetGrid());
	CustomGrid_DeleteAllColumn(GetGrid());
	InitDialogStep2();
    m_changed = wyFalse;
	m_autoincpresent = wyFalse;
    InitAdvProperties(&m_advprop);
 }
void
TableMakerCreateTable::InitAdvProperties(TableAdvPropValues	*advprop)
{
	advprop->m_changed = wyFalse;
	advprop->m_checksum.Clear();
	advprop->m_auto_incr.Clear();
	advprop->m_avg_row.Clear();
	advprop->m_comment.Clear();
	advprop->m_max_rows.Clear();
	advprop->m_min_rows.Clear();
	advprop->m_delay.Clear();
	advprop->m_rowformat.Clear();
	advprop->m_chunks.Clear();
	advprop->m_chunksize.Clear();
}

wyBool          
TableMakerCreateTable::IsChanged()
{
	return m_changed;
}

wyBool
TableMakerCreateTable::CreateQuery(wyString &query)
{
	TableMakerAdvProperties		ctav;
	wyString					tabledefn;                  // table column info
    wyString					primarydefn("PRIMARY KEY (");   
	wyString					autoincrdefn("UNIQUE KEY ");
    wyString					otherprop;

	// get the advanced properties values
    // need to change it to wyString
    ctav.GetAdvPropString(GetTunnel(), GetMySQL(), &m_advprop, otherprop);

	//to trim empty spaces from right, to avoid mysql errors
	m_tablename.RTrim();

    // now create the create table query
    tabledefn.AddSprintf("create table `%s`.`%s`( ", m_dbname.GetString(), m_tablename.GetString());
	
	if(CreateCompleteQuery(tabledefn, primarydefn, autoincrdefn) == wyFalse)
        return wyFalse;

	// now create the complete query and execute it.
	if(primarydefn.GetLength())
		tabledefn.AddSprintf("\n   %s", primarydefn.GetString()); 

	tabledefn.AddSprintf("\n )"); 

	if(otherprop.GetLength())
		tabledefn.AddSprintf(" %s ", otherprop.GetString()); 

	query.SetAs(tabledefn);
	return wyTrue;

}    

wyBool
TableMakerCreateTable::ShowPreview()
{
	QueryPreview	querypreview;
	wyString		query;
	wyInt32			retvalue;

	CustomGrid_ApplyChanges(m_hwndgrid);

	retvalue = CreateTableValidation();
	
	if(retvalue == wyFalse)
		return wyFalse;

	CreateQuery(query);
	querypreview.Create(m_hwnd,(wyChar *)query.GetString());

	return wyTrue;
}
