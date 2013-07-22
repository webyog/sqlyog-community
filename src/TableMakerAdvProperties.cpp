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


#include "TableMakerAdvProperties.h"
#include "FrameWindowHelper.h"
#include "CommonHelper.h"
#include "MySQLVersionHelper.h"
#include "ClientMySQLWrapper.h"
#include "GUIHelper.h"
#include "SQLmaker.h"

extern     PGLOBALS     pGlobals;

/*---------------------------------------------------------------------------------------
	Implementation of TAdvProp.
	Through this dialog the user can change some of the advance properties of a table.
---------------------------------------------------------------------------------------*/

TableMakerAdvProperties::TableMakerAdvProperties()
{

}

TableMakerAdvProperties::~TableMakerAdvProperties()
{
}

// The dialog box is started from here.

wyInt32
TableMakerAdvProperties::Create(HWND hwnd, 
                          const wyChar* db, const wyChar* table, Tunnel *tunnel, 
                          PMYSQL mysql, 
                          TableAdvPropValues* ptav, wyBool isalter)
{
    /* initialize member variables with values */
    if(db) 
      m_db.SetAs(db);
    if(table) 
      m_table.SetAs(table);

	m_mysql         = mysql;
    m_tunnel        = tunnel;
	m_hwndparent    = hwnd;
	m_advprop       = ptav;
	m_isalter       = isalter;

	//Post 8.01
	//RepaintTabModule();
    
	return DialogBoxParam(pGlobals->m_hinstance, 
                          MAKEINTRESOURCE(IDD_ALTERTABLEPROP), 
                          hwnd, 
                          (DLGPROC)TableMakerAdvProperties::DlgProc, 
                          (LPARAM)this); 
}

// The dialog procedure for the dialog for the underlying dialog

BOOL CALLBACK
TableMakerAdvProperties::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	TableMakerAdvProperties*  pctav = (TableMakerAdvProperties*)GetWindowLong(hwnd, GWL_USERDATA);
    

	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, UM_INITDLGVALUES, 0, 0);
			return FALSE;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			VERIFY(yog_enddialog( hwnd, 0));
			break;

		case IDOK:
			pctav->FillStructure();
			VERIFY(yog_enddialog(hwnd, 1));
			break;
        case IDC_TABCHARSET:
             if((HIWORD(wParam))== CBN_SELENDOK)
             {
                pctav->HandleTabCharset(hwnd);
             }
        }
		break;

	case UM_INITDLGVALUES:
		{
			pctav->m_hwnd = hwnd;
			pctav->InitDlgValues();
			pctav->FillInitialData();
		}
		break;
	}

	return 0;
}

void
TableMakerAdvProperties::HandleTabCharset(HWND hwnd)
{
    HWND        hwndcombo;
    wyInt32     ncursel, length, index;
    wyWChar     *charsetname = NULL;
    
    hwndcombo = GetDlgItem(hwnd, IDC_TABCHARSET);
    ncursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);
    length  = SendMessage(hwndcombo, CB_GETLBTEXTLEN, ncursel, NULL);
	
    charsetname    = AllocateBuffWChar(length + 1);

	SendMessage(hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)charsetname);
    
    if(wcsicmp(charsetname, TEXT(STR_DEFAULT)) != 0)
        this->ReInitRelatedCollations(hwnd, charsetname);

    if((index = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT(STR_DEFAULT))) == CB_ERR)
        SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)TEXT(STR_DEFAULT)); 
    
    free(charsetname);
}

void
TableMakerAdvProperties::ReInitRelatedCollations(HWND hwnd, wyWChar *charsetname)
{
    MDIWindow	*pcquerywnd = GetActiveWin();
    MYSQL_RES   *myres;
    MYSQL_ROW   myrow;
    wyWChar     *relcollation  = NULL;
    wyString    query, collationstr;
    wyInt32     ret, selcharsetlen  = 0;
    
    HWND    hwndcombo = GetDlgItem(hwnd, IDC_TABCOLLATION);
    
    query.SetAs("show collation");
    myres = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return;
	}

    VERIFY((hwndcombo, CB_RESETCONTENT, 0, 0));
        
    if(charsetname)
        selcharsetlen = wcslen(charsetname);

    while(myrow = pcquerywnd->m_tunnel->mysql_fetch_row(myres))
    {
        collationstr.SetAs(myrow[0]);
        ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)collationstr.GetAsWideChar());
	    if(ret != CB_ERR)
        {
            //delete the items which are not relevent
           if(wcsstr(collationstr.GetAsWideChar(), charsetname) == NULL)
                SendMessage(hwndcombo, CB_DELETESTRING, ret, 0);
        }
        else if((relcollation = wcsstr(collationstr.GetAsWideChar(), charsetname)) != NULL)
        {
            // Add the relevent items
            if(collationstr.GetCharAt(selcharsetlen) == '_')
                SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)collationstr.GetAsWideChar()); 
        }
    }
    pcquerywnd->m_tunnel->mysql_free_result(myres);
    SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)0, 0);
}

void
TableMakerAdvProperties::InitDlgValues()
{
    wyBool  ismysql41 = IsMySQL41(m_tunnel, m_mysql);
	
	// set the text limit of the edit boxes to the max field.
	SendMessage(GetDlgItem(m_hwnd, IDC_COMMENT), EM_LIMITTEXT, 60, 0);

    InitTableTypeValues();
    InitCheckSumValues();
    InitDelayKeyWriteValues();
    InitRowFormatValues();
    
    if(ismysql41 == wyFalse)
    {
        EnableWindow(GetDlgItem(m_hwnd, IDC_TABCHARSET), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_TABCOLLATION), FALSE);
    }
    else
    {
        InitCharacterSetCombo();
        InitCollationCombo();
    }
}

void
TableMakerAdvProperties::InitTableTypeValues()
{
    HWND        hwndcombo;
	wyInt32     index;
    wyString    strengine;
    wyWChar     *enginebuff, *tokpointer;

    GetTableEngineString(m_tunnel, m_mysql, strengine);
    
    enginebuff = AllocateBuffWChar(strengine.GetLength()+1);
    wcscpy(enginebuff, strengine.GetAsWideChar());
    VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TYPE));   

    tokpointer = wcstok(enginebuff, L";");

    while(tokpointer != NULL)
	{
        VERIFY((SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)tokpointer)) >= 0);

        tokpointer = wcstok(NULL, L";");
    }
    free(enginebuff);

	if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));

    return;
}

void
TableMakerAdvProperties::InitCheckSumValues()
{
    HWND        hwndofwindow;
	wyWChar	    checksum[][20] = {L"1", L"0", NULL};
    wyInt32     i=0;
	wyInt32		index;
    
    VERIFY(hwndofwindow = GetDlgItem(m_hwnd, IDC_CHECKSUM));	
    
    while(checksum[i][0])
    {
        VERIFY((SendMessage(hwndofwindow, CB_ADDSTRING, 0, (LPARAM)checksum[i])) >=0);
        i++;
    }
	    
	if((index = SendMessage(hwndofwindow , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
      SendMessage(hwndofwindow, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
}

void
TableMakerAdvProperties::InitDelayKeyWriteValues()
{
    HWND        hwndofwindow;
	wyWChar     delaykey[][20] = {L"1", L"0", NULL };
    wyInt32     i=0, index;

	VERIFY(hwndofwindow = GetDlgItem(m_hwnd, IDC_DELAYKEY));	
    while(delaykey[i][0])
    {
		VERIFY((SendMessage(hwndofwindow, CB_ADDSTRING, 0, (LPARAM)delaykey[i])) >=0 );
        i++;
    }

	if((index = SendMessage(hwndofwindow , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
      SendMessage(hwndofwindow, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));   
}

void
TableMakerAdvProperties::InitRowFormatValues()
{
	wyWChar  rowformat[][20] = {L"dynamic", L"fixed", L"compressed", NULL };
    HWND    hwndtowindow;
    wyInt32 i=0, index = 0;
    
    VERIFY(hwndtowindow = GetDlgItem(m_hwnd, IDC_ROWFORMAT));	
    while(rowformat[i][0])
    {
        VERIFY((SendMessage(hwndtowindow, CB_ADDSTRING, 0, (LPARAM)rowformat[i])) >=0);
        i++;
    }

	if((index = SendMessage(hwndtowindow , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
      SendMessage(hwndtowindow, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));   
}

// Function get information about the various options of the table and set it in the dialog

void
TableMakerAdvProperties::FillInitialData()
{
	HWND	    hwnd;
    wyInt32	    index;

	//table type
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_TYPE));
	index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_type.GetAsWideChar());
	if(index != CB_ERR)
		SendMessage(hwnd, CB_SETCURSEL, index, 0);
		
	//autoincrement data.
    VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_AUTOINCR));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_auto_incr.GetAsWideChar());

	// now rowformat.
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_ROWFORMAT));
	index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_rowformat.GetAsWideChar());
	if(index == CB_ERR)		
        SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, index, 0);

    // Charset Combo info
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_TABCHARSET));
    index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_charset.GetAsWideChar());
	if(index == CB_ERR)		
        SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, index, 0);

    // collation combo initialization
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_TABCOLLATION));
    index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_collation.GetAsWideChar());
	if(index == CB_ERR)		
        SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, index, 0);  

	// Now average row length
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_AVGROW));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_avg_row.GetAsWideChar());

	// now the comment.
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_COMMENT)); 
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_comment.GetAsWideChar());

	// max row for the table
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_MAXROW));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_max_rows.GetAsWideChar());

    // min row for the table
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_MINROW));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_min_rows.GetAsWideChar());

	/// Checksum
      VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_CHECKSUM));
	index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_checksum.GetString());
      if(index == CB_ERR)
		SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, index, 0);

	//the delay key write
	VERIFY(hwnd= GetDlgItem(m_hwnd, IDC_DELAYKEY));
	index = SendMessage(hwnd, CB_FINDSTRING, -1, (LPARAM)m_advprop->m_delay.GetAsWideChar());
	if(index == CB_ERR)
		SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, index, 0);

    	// the chunks and chunks size.
	VERIFY(hwnd = GetDlgItem(m_hwnd, IDC_CHUNKS));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_chunks.GetAsWideChar());

	VERIFY(hwnd = GetDlgItem (m_hwnd, IDC_CHUNKSIZE));
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)m_advprop->m_chunksize.GetAsWideChar());
}

// Function fills the table adv struture which is passed as parameter,
// so that the calling dialog Alter Table or Create Table can use it.

void
TableMakerAdvProperties::FillStructure ()
{
	wyInt32		    selindex;
	HWND	        hwndcombo;
    wyWChar         textbuf[512]={0};           // none of the value will go more then this and neway we send the max len to the api so it will not fail
    wyChar			*commentbuff = {0};
    wyBool          ismysql41 = IsMySQL41(m_tunnel, m_mysql);
	wyString        rowformat, enginestr;
	
	// now we start getting all the text.
	// 1. With the table type.
	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TYPE));
	VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR );
	SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);

	//if default is selected then it should take 'default'(with quotes)
	enginestr.SetAs(textbuf);

	if(enginestr.CompareI(STR_DEFAULT) == 0)
		m_advprop->m_type.SetAs("'Default'");
	else
		m_advprop->m_type.SetAs(textbuf);
	
	//2. With the checksum thing.
	VERIFY(hwndcombo  = GetDlgItem(m_hwnd, IDC_CHECKSUM));
	VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);
	SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    m_advprop->m_checksum.SetAs(textbuf);

    if(ismysql41 == wyTrue)
    {
        textbuf[0] = NULL;
        VERIFY(hwndcombo  = GetDlgItem(m_hwnd, IDC_TABCHARSET));
        VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);
        SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
        
        if(wcslen(textbuf) == 0)
            m_advprop->m_charset.SetAs(STR_DEFAULT);
        else
        m_advprop->m_charset.SetAs(textbuf);
        		
        textbuf[0] = NULL;
        VERIFY(hwndcombo  = GetDlgItem(m_hwnd, IDC_TABCOLLATION));
        VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);
        SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
        if(wcslen(textbuf) == 0)
            m_advprop->m_collation.SetAs(STR_DEFAULT);
        else
			m_advprop->m_collation.SetAs(textbuf);
    }

	// 3. The autoincr thing.
	SendMessage(GetDlgItem(m_hwnd, IDC_AUTOINCR), WM_GETTEXT, 15, (LPARAM)textbuf);
    m_advprop->m_auto_incr.SetAs(textbuf);

	// 4. The average row thing.
	SendMessage(GetDlgItem(m_hwnd, IDC_AVGROW), WM_GETTEXT, 15, (LPARAM)textbuf);
    m_advprop->m_avg_row.SetAs(textbuf);

	// 5. The comment thing.
	SendMessage(GetDlgItem(m_hwnd, IDC_COMMENT ), WM_GETTEXT, 511, (LPARAM)textbuf);
    m_advprop->m_comment.SetAs(textbuf);

    // Escape the comment string
	commentbuff = AllocateBuff((m_advprop->m_comment.GetLength() + 1) * 2);   // Considering allocation including escape characters
	m_tunnel->mysql_real_escape_string(*m_mysql, commentbuff, m_advprop->m_comment.GetString(), m_advprop->m_comment.GetLength());
	m_advprop->m_comment.SetAs(commentbuff);

	// 6. The max row thing.
    SendMessage(GetDlgItem(m_hwnd, IDC_MAXROW), WM_GETTEXT, 15, (LPARAM)textbuf);
    m_advprop->m_max_rows.SetAs(textbuf);

	// 7. The max row thing.
    SendMessage(GetDlgItem(m_hwnd, IDC_MINROW), WM_GETTEXT, 15, (LPARAM)textbuf);
	m_advprop->m_min_rows.SetAs(textbuf);

	// 9. The delay thing.
	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_DELAYKEY));
	VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);
	SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    m_advprop->m_delay.SetAs(textbuf);

	// 10. The row format thing.
	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_ROWFORMAT));
	VERIFY((selindex = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);
	SendMessage(hwndcombo, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    rowformat.SetAs(textbuf);
	// bug reported in http://www.webyog.com/forums//index.php?showtopic=3887 (solved in 6.52)
	if(rowformat.CompareI(STR_DEFAULT) == 0)
		m_advprop->m_rowformat.SetAs("Default");
	else
    m_advprop->m_rowformat.SetAs(textbuf);

	// 12. The chunks thing.
	SendMessage(GetDlgItem(m_hwnd, IDC_CHUNKS), WM_GETTEXT, 511, (LPARAM)textbuf);
    m_advprop->m_chunks.SetAs(textbuf);
	
	// 13. The chunks size thing.
	SendMessage(GetDlgItem(m_hwnd, IDC_CHUNKSIZE), WM_GETTEXT, 511, (LPARAM)textbuf);
    m_advprop->m_chunksize.SetAs(textbuf);

    m_advprop->m_changed = wyTrue;
	free(commentbuff);
}

// Function creates a text buffer with all the properties and sends newly allocated buffer.
wyBool
TableMakerAdvProperties::GetAdvPropString( Tunnel *tunnel, PMYSQL mysql, 
                                     TableAdvPropValues* ptav, wyString& str)
{
    wyBool ismysql41 = IsMySQL41(tunnel, mysql);

	if(ptav->m_changed == wyFalse)
		return wyFalse;

	/* 	nCount += sprintf( szQuery+nCount, " ,type=%s ", ptav->szType );
		This comma is not required, this is required in the alter stmt when this
		stmt is not the first one. So we are checking it in the query builder
		In the latest versions we need 'Engine' in place of 'Type'
	*/
    if(ptav->m_type.GetLength())
    {
	    if(ismysql41 == wyTrue)
		    str.AddSprintf(" Engine=%s ", ptav->m_type.GetString());
	    else
		    str.AddSprintf(" Type=%s ", ptav->m_type.GetString());
    }

	if(ptav->m_checksum.CompareI(STR_DEFAULT) != 0)
		str.AddSprintf("checksum=%s ", ptav->m_checksum.GetString());

	if(ptav->m_auto_incr.GetLength())
        str.AddSprintf("auto_increment=%s ", ptav->m_auto_incr.GetString());

	if(ptav->m_avg_row.GetLength())
		str.AddSprintf("avg_row_length=%s ", ptav->m_avg_row.GetString());

	if(ptav->m_comment.GetLength())
		str.AddSprintf("comment='%s' ", ptav->m_comment.GetString());
    else
        str.Add("comment='' ");

	if(ptav->m_max_rows.GetLength())
		str.AddSprintf("max_rows=%s ", ptav->m_max_rows.GetString());

	if(ptav->m_min_rows.GetLength())
		str.AddSprintf("min_rows=%s ", ptav->m_min_rows.GetString());

    	if(ptav->m_delay.CompareI(STR_DEFAULT) != 0)
        str.AddSprintf("delay_key_write=%s ", ptav->m_delay.GetString());

	if(ptav->m_rowformat.GetLength())
		str.AddSprintf("row_format=%s ", ptav->m_rowformat.GetString());

	if(ptav->m_chunks.GetLength())
		str.AddSprintf("raid_chunks=%s ", ptav->m_chunks.GetString());

    if(ptav->m_chunksize.GetLength())
        str.AddSprintf("raid_chunksize=%s ", ptav->m_chunksize.GetString());

    if(ismysql41 == wyTrue)
    {
        if(ptav->m_charset.GetLength() && ptav->m_charset.Compare(STR_DEFAULT) != 0)
            str.AddSprintf("charset=%s ", ptav->m_charset.GetString());

        if(ptav->m_collation.GetLength() && ptav->m_collation.Compare(STR_DEFAULT) != 0)
            str.AddSprintf("collate=%s ", ptav->m_collation.GetString());
    }

	return wyTrue;
}

void
TableMakerAdvProperties::CopyProperties(TableAdvPropValues *advprop, TableAdvPropValues *ptav)
{
    advprop->m_auto_incr.SetAs(ptav->m_auto_incr.GetString());
    advprop->m_avg_row.SetAs(ptav->m_avg_row.GetString());
    advprop->m_changed = ptav->m_changed;
    advprop->m_checksum.SetAs(ptav->m_checksum.GetString());
    advprop->m_chunks.SetAs(ptav->m_chunks.GetString());
    advprop->m_chunksize.SetAs(ptav->m_chunksize.GetString());
    advprop->m_comment.SetAs(ptav->m_comment.GetString());
    advprop->m_delay.SetAs(ptav->m_delay.GetString());
    advprop->m_max_rows.SetAs(ptav->m_max_rows.GetString());
    advprop->m_min_rows.SetAs(ptav->m_min_rows.GetString());
    advprop->m_rowformat.SetAs(ptav->m_rowformat.GetString());
    advprop->m_type.SetAs(ptav->m_type.GetString());
    return;
}

void
TableMakerAdvProperties::InitCharacterSetCombo()
{
    HWND        hwndcombo = GetDlgItem(m_hwnd, IDC_TABCHARSET);
    wyString    query, charsetstr;
    MYSQL_ROW   myrow;
    MYSQL_RES   *myres;
    MDIWindow   *wnd = GetActiveWin();
    wyInt32     index;
    
    query.SetAs("show charset");

    myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        charsetstr.SetAs(myrow[0]);
        index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)charsetstr.GetAsWideChar());
    }
    
    if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
}

void
TableMakerAdvProperties::InitCollationCombo()
{
    MDIWindow   *wnd = GetActiveWin();
    wyString    query, collationstr;
    MYSQL_ROW   myrow;
    MYSQL_RES   *myres;
    wyInt32     index;
    
    HWND        hwndcombo = GetDlgItem(m_hwnd, IDC_TABCOLLATION);
   
    query.SetAs("show collation");
    myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        collationstr.SetAs(myrow[0]);
        SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)collationstr.GetAsWideChar());
    }
    if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        index = SendMessage(hwndcombo, CB_SETCURSEL, index, (LPARAM)TEXT(STR_DEFAULT));
}
