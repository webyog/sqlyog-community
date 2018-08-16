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

#include "TabAdvancedProperties.h"
#include "MDIWindow.h"
#include "Global.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"

extern PGLOBALS		pGlobals;

#define MAX_HEIGHT		320


TabAdvancedProperties::TabAdvancedProperties(HWND hwnd, TableTabInterfaceTabMgmt *ptabmgmt)
{
    m_hwnd                  =   hwnd;
    m_ptabmgmt  =   ptabmgmt;

    m_hlastfocusedwnd       =   NULL;
    m_mdiwnd                =   GetActiveWin();
    m_ismysql41             =   IsMySQL41(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
	m_ismysql553            =   IsMySQL553(m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);

    //..To achieve Tab/Shift+Tab keys functionality
    m_wporigcomment         =   NULL;

    m_wporigautoincr        =   NULL;
    m_wporigavgrowlen       =   NULL;
    m_wporigmaxrows         =   NULL;
    m_wporigminrows         =   NULL;
    m_wporigrowformat       =   NULL;
    m_wporigchecksum        =   NULL;
    m_wporigdelaykey        =   NULL;

    m_disableenchange       =   wyTrue;
	m_dispheight = 0;
	m_scrollpos = 0;
	m_prevscrollpos = 0;

    m_origchecksum.Clear();
    m_origautoincr.Clear();
    m_origavgrow.Clear();
    m_origcomment.Clear();
    m_origmaxrows.Clear();
    m_origminrows.Clear();
    m_origdelay.Clear();
    m_origrowformat.Clear();
}

void
TabAdvancedProperties::InitCheckSumValues(wyBool includedefault)
{
	wyWChar	    checksum[][20] = {L"1", L"0", NULL};
    wyInt32     i=0;
	wyInt32		index;
    
    if(includedefault)
    {
	    if((index = SendMessage(m_hcmbchecksum, CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
          SendMessage(m_hcmbchecksum, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
    }
    
    while(checksum[i][0])
    {
        VERIFY((SendMessage(m_hcmbchecksum, CB_ADDSTRING, 0, (LPARAM)checksum[i])) >=0);
        i++;
    }
}

void
TabAdvancedProperties::InitDelayKeyWriteValues(wyBool includedefault)
{
	wyWChar     delaykey[][20] = {L"1", L"0", NULL };
    wyInt32     i=0, index;

    if(includedefault)
    {
	    if((index = SendMessage(m_hcmbdelaykeywrite, CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
          SendMessage(m_hcmbdelaykeywrite, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));   
    }

    while(delaykey[i][0])
    {
		VERIFY((SendMessage(m_hcmbdelaykeywrite, CB_ADDSTRING, 0, (LPARAM)delaykey[i])) >=0 );
        i++;
    }
}

void
TabAdvancedProperties::InitRowFormatValues(wyBool includedefault)
{
	wyWChar  rowformat[][20] = {L"dynamic", L"fixed", L"compressed", L"compact", L"redundant", NULL };
    wyInt32 i=0, index = 0;
    
    if(includedefault)
    {
	    if((index = SendMessage(m_hcmbrowformat , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
          SendMessage(m_hcmbrowformat, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));   
    }

    while(rowformat[i][0])
    {
        VERIFY((SendMessage(m_hcmbrowformat, CB_ADDSTRING, 0, (LPARAM)rowformat[i])) >=0);
        i++;
    }
}

VOID
TabAdvancedProperties::CreateOtherWindows(HWND hwndparent)
{
    wyUInt32        style   = WS_CHILD | WS_VISIBLE;
    
	//...Creating Comment Windows (Static & Edit)
    VERIFY(m_hstaticcomment = CreateWindowEx(0, WC_STATIC, _(TEXT("Comment")), 
                                        style, 0,0, 0,0,	hwndparent, (HMENU)0, 
										 (HINSTANCE)pGlobals->m_hinstance, NULL));
    
    if(m_hstaticcomment)
    {
        ShowWindow(m_hstaticcomment, SW_HIDE);
        UpdateWindow(m_hstaticcomment);
    }

    m_heditcomment  =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _(TEXT("")),
        style | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP | WS_GROUP | ES_WANTRETURN, 0, 0, 0, 0, hwndparent, (HMENU) IDC_COMMENT, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_heditcomment)
    {
        ShowWindow(m_heditcomment, SW_HIDE);
        UpdateWindow(m_heditcomment);
        m_hlastfocusedwnd = m_heditcomment;
    }
    m_wporigcomment = (WNDPROC)SetWindowLongPtr(m_heditcomment, GWLP_WNDPROC, (LONG_PTR)HandleTabKey);
	SetWindowLongPtr(m_heditcomment, GWLP_USERDATA, (LONG_PTR)this);
    m_hlastfocusedwnd = m_heditcomment;

    //...Creating Auto-Increment Windows (Static & Edit)
    m_hstatautoincr =   CreateWindowEx(0, WC_STATIC, _(TEXT("Auto increment")), 
                                        style , 0,0, 0,0,	hwndparent, (HMENU) 0, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_hstatautoincr)
    {
        ShowWindow(m_hstatautoincr, SW_HIDE);
        UpdateWindow(m_hstatautoincr);
    }

    m_heditautoincr =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _(TEXT("")), 
                                        style | ES_NUMBER  | WS_TABSTOP | WS_GROUP, 0,0, 0,0,	hwndparent, (HMENU) IDC_AUTOINCR, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_heditautoincr)
    {
        ShowWindow(m_heditautoincr, SW_HIDE);
        UpdateWindow(m_heditautoincr);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_heditautoincr;
    }

    //origproc = (WNDPROC)SetWindowLongPtr(m_heditautoincr, GWLP_WNDPROC, (LONG)LastFocus);
	//SetWindowLongPtr(m_heditautoincr, GWLP_USERDATA, (LONG)this);
    //SetWindowLongPtr(m_heditautoincr, GWLP_WNDPROC, (LONG)origproc);

    //...Creating Avg Row Length Windows (Static & Edit)
    m_hstatavgrowlen =   CreateWindowEx(0, WC_STATIC, _(TEXT("Avg. row length")), 
                                        style, 0,0, 0,0,	hwndparent, (HMENU) 0, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_hstatavgrowlen)
    {
        ShowWindow(m_hstatavgrowlen, SW_HIDE);
        UpdateWindow(m_hstatavgrowlen);
    }

    m_heditavgrowlen =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _(TEXT("")), 
                                        style | ES_NUMBER  | WS_TABSTOP | WS_GROUP, 0,0, 0,0,	hwndparent, (HMENU) IDC_AVGROWLEN, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_heditavgrowlen)
    {
        ShowWindow(m_heditavgrowlen, SW_HIDE);
        UpdateWindow(m_heditavgrowlen);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_heditavgrowlen;
    }
    
    //...Creating Max Rows Windows (Static & Edit)
    m_hstatmaxrows =   CreateWindowEx(0, WC_STATIC, _(TEXT("Maximum rows")), 
                                        style, 0,0, 0,0,	hwndparent, (HMENU) 0, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_hstatmaxrows)
    {
        ShowWindow(m_hstatmaxrows, SW_HIDE);
        UpdateWindow(m_hstatmaxrows);
    }

    m_heditmaxrows =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _(TEXT("")), 
                                style | ES_NUMBER | WS_TABSTOP | WS_GROUP, 0,0, 0,0,	hwndparent, (HMENU) IDC_MAXROWS, 
                                (HINSTANCE)pGlobals->m_hinstance, NULL);

    if(m_heditmaxrows)
    {
        ShowWindow(m_heditmaxrows, SW_HIDE);
        UpdateWindow(m_heditmaxrows);
        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_heditmaxrows;
    }
    
    //...Creating Min Rows Windows (Static & Edit)
    m_hstatminrows =   CreateWindowEx(0, WC_STATIC, _(TEXT("Minimum rows")), 
                                style, 0,0, 0,0,	hwndparent, (HMENU) 0, 
                                (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatminrows)
    {
        ShowWindow(m_hstatminrows, SW_HIDE);
        UpdateWindow(m_hstatminrows);
    }

    m_heditminrows =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _(TEXT("")), 
                                        style | ES_NUMBER | WS_TABSTOP | WS_GROUP, 0,0, 0,0,	hwndparent, (HMENU) IDC_MINROW, 
										(HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_heditminrows)
    {
        ShowWindow(m_heditminrows, SW_HIDE);
        UpdateWindow(m_heditminrows);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_heditminrows;
    }

    //...Creating Row Format Windows (Static & Combo)
    m_hstatrowformat =   CreateWindowEx(0, WC_STATIC, _(TEXT("Row format")), 
                                style, 0, 0, 0, 0,	hwndparent, (HMENU) 0, 
                                (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatrowformat)
    {
        ShowWindow(m_hstatrowformat, SW_HIDE);
        UpdateWindow(m_hstatrowformat);
    }

    m_hcmbrowformat =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | WS_TABSTOP, 0, 0, 0, 0, hwndparent, (HMENU) IDC_ROWFORMAT, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbrowformat)
    {
        ShowWindow(m_hcmbrowformat, SW_HIDE);
        UpdateWindow(m_hcmbrowformat);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_hcmbrowformat;
    }

    //...Creating Checksum Windows (Checkbox)
    m_hstatchecksum =   CreateWindowEx(0, WC_STATIC, _(TEXT("Checksum")), 
                                        style, 0, 0, 0, 0,	hwndparent, (HMENU) 0, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatchecksum)
    {
        ShowWindow(m_hstatchecksum, SW_HIDE);
        UpdateWindow(m_hstatchecksum);
    }

    m_hcmbchecksum =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")), 
                                        style | CBS_DROPDOWNLIST | WS_TABSTOP, 0, 0, 0, 0, hwndparent, (HMENU) IDC_CHECKSUM, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbchecksum)
    {
        ShowWindow(m_hcmbchecksum, SW_HIDE);
        UpdateWindow(m_hcmbchecksum);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_hcmbchecksum;
    }

    //...Creating Delay Key Windows (Static & Combo)
    m_hstatdelaykeywrite =   CreateWindowEx(0, WC_STATIC, _(TEXT("Delay key write")), 
                                        style, 0, 0, 0, 0,	hwndparent, (HMENU) 0, 
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hstatdelaykeywrite)
    {
        ShowWindow(m_hstatdelaykeywrite, SW_HIDE);
        UpdateWindow(m_hstatdelaykeywrite);
    }

    m_hcmbdelaykeywrite =   CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _(TEXT("")),
                                        style | CBS_DROPDOWNLIST | WS_TABSTOP, 0, 0, 0, 0, hwndparent, (HMENU) IDC_DELAYKEY,
                                        (HINSTANCE)pGlobals->m_hinstance, NULL);
    if(m_hcmbdelaykeywrite)
    {
        ShowWindow(m_hcmbdelaykeywrite, SW_HIDE);
        UpdateWindow(m_hcmbdelaykeywrite);

        if(!m_hlastfocusedwnd)
            m_hlastfocusedwnd = m_hcmbdelaykeywrite;
    }
    
    if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
    {
        m_wporigautoincr = (WNDPROC)SetWindowLongPtr(m_heditautoincr, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_heditautoincr, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigavgrowlen = (WNDPROC)SetWindowLongPtr(m_heditavgrowlen, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_heditavgrowlen, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigmaxrows = (WNDPROC)SetWindowLongPtr(m_heditmaxrows, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_heditmaxrows, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigminrows = (WNDPROC)SetWindowLongPtr(m_heditminrows, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_heditminrows, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigrowformat = (WNDPROC)SetWindowLongPtr(m_hcmbrowformat, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbrowformat, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigchecksum = (WNDPROC)SetWindowLongPtr(m_hcmbchecksum, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbchecksum, GWLP_USERDATA, (LONG_PTR)this);

        m_wporigdelaykey = (WNDPROC)SetWindowLongPtr(m_hcmbdelaykeywrite, GWLP_WNDPROC, (LONG_PTR)TabAdvancedProperties::SysKeyDownWndProc);
        SetWindowLongPtr(m_hcmbdelaykeywrite, GWLP_USERDATA, (LONG_PTR)this);
    }
    
    m_hwndscroll =  CreateWindowEx( 
            0, WC_SCROLLBAR, NULL, WS_CHILD | WS_VISIBLE | SBS_VERT | SBS_RIGHTALIGN, 
            0, 0, 0, 0, hwndparent, (HMENU) NULL, pGlobals->m_hinstance, NULL);

    ShowWindow(m_hwndscroll, SW_HIDE);

    /*m_wporigscrollbar = (WNDPROC)SetWindowLongPtr(m_hwndscroll, GWLP_WNDPROC, (LONG)ScrollbarWndproc);*/
	SetWindowLongPtr(m_hwndscroll, GWLP_USERDATA, (LONG_PTR)this);

	 //Sets the Windows' font
    SendMessage(m_hstaticcomment, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_heditcomment, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatautoincr, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_heditautoincr, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatavgrowlen, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_heditavgrowlen, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatmaxrows, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_heditmaxrows, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatminrows, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_heditminrows, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatrowformat, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbrowformat, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatdelaykeywrite, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbdelaykeywrite, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hstatchecksum, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hcmbchecksum, (UINT)WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

LRESULT CALLBACK
TabAdvancedProperties::SysKeyDownWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TabAdvancedProperties   *tabadv = (TabAdvancedProperties*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    WNDPROC             wndproc = NULL;

    if(!tabadv)
        return 1;

    switch(message)
    {
    case WM_SYSKEYDOWN:
        {
            if(!tabadv->m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(CustomTab_GetCurSel(tabadv->m_ptabmgmt->m_hwnd), wParam, lParam))
                return 0;
        }
        break;
    }
    
    if(hwnd == tabadv->m_heditautoincr)
        wndproc = tabadv->m_wporigautoincr;
    else if(hwnd == tabadv->m_heditavgrowlen)
        wndproc = tabadv->m_wporigavgrowlen;
    else if(hwnd == tabadv->m_heditminrows)
        wndproc = tabadv->m_wporigminrows;
    else if(hwnd == tabadv->m_heditmaxrows)
        wndproc = tabadv->m_wporigmaxrows;
    else if(hwnd == tabadv->m_hcmbrowformat)
        wndproc = tabadv->m_wporigrowformat;
    else if(hwnd == tabadv->m_hcmbchecksum)
        wndproc = tabadv->m_wporigchecksum;
    else if(hwnd == tabadv->m_hcmbdelaykeywrite)
        wndproc = tabadv->m_wporigdelaykey;

    if(!wndproc)
        return 1;

    return CallWindowProc(wndproc, hwnd, message, wParam, lParam);
}


void
TabAdvancedProperties::InitAllWindows(wyBool includedefault)
{
	// set the text limit of the edit boxes to the max field.
	if(m_ismysql553)
	SendMessage(GetDlgItem(m_hwnd, IDC_COMMENT), EM_LIMITTEXT, 2048, 0);
	else 
     SendMessage(GetDlgItem(m_hwnd, IDC_COMMENT), EM_LIMITTEXT, 60, 0);

    InitCheckSumValues(includedefault);
    InitDelayKeyWriteValues(includedefault);
    InitRowFormatValues(includedefault);
}

wyBool
TabAdvancedProperties::ReinitializeValues()
{
    if(!FetchInitValues())
        return wyFalse;

    SendMessage(m_hcmbrowformat, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hcmbchecksum, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hcmbdelaykeywrite, CB_RESETCONTENT, 0, 0);

    InitAllWindows(wyFalse);

    FillInitValues();

    return wyTrue;
}

wyBool
TabAdvancedProperties::FetchInitValues()
{
    wyInt32		commentindex, index = 0, val, collationindex, charindex;
    wyString	query, collation, criteria;
	wyString	retval,myrowstr;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyInt32		pos = 0, newpos = 0;
	wyBool		flag = wyFalse;
    wyChar      *charset = NULL;
    wyString    tempstr;
    
    myres = m_ptabmgmt->m_tabinterfaceptr->m_myrestablestatus;
    if(!myres)
        return wyFalse;

    m_mdiwnd->m_tunnel->mysql_data_seek(myres, 0);

    commentindex = GetFieldIndex(myres, "comment", m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);

	myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres);
	
	if(!myrow || !myrow[0] || !myrow[1])
	{	
        yog_message(m_hwnd, _(L"Could not read data for the table!"), pGlobals->m_appname.GetAsWideChar(), 
								MB_OK | MB_ICONINFORMATION);

		return wyFalse;			
	}

    if(m_ismysql41 == wyTrue)
	{
		collationindex = GetFieldIndex(myres, "collation", m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
    
		if(collationindex != -1 && myrow[collationindex] != 0)
			collation.SetAs(myrow[collationindex], m_ismysql41);

        m_ptabmgmt->m_tabinterfaceptr->m_origcollate.SetAs(collation);

        tempstr.SetAs(collation);
		criteria.SetAs("_");

		//we extract the charset by collation info of the table
		//for eg, x_y is the collation then before "_" is the charset
		charindex = tempstr.Find(criteria.GetString(), 0);
		charset = (wyChar *)tempstr.GetString();
		
		if(charindex != -1)
		{
			charset[charindex ] = 0;
		}
        m_ptabmgmt->m_tabinterfaceptr->m_origcharset.SetAs(charset);
	}

    commentindex = GetFieldIndex(myres, "auto_increment", m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);
    if(commentindex != -1 && myrow[commentindex] != 0)
    {
        tempstr.SetAs(myrow[commentindex]);
        m_origautoincr.SetAs(myrow[commentindex], m_ismysql41);
    }

    index = GetFieldIndex(myres, "Create_options", m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql);

    /// Fetch type engine type value
    if(myrow[1])
    {
        tempstr.SetAs(myrow[1]);
        m_ptabmgmt->m_tabinterfaceptr->m_origtableengine.SetAs(myrow[1]);
    }
    
	/// Code added to strip off ";Innodb free" in comment string	
	myrowstr.SetAs(myrow[GetFieldIndex(myres, "comment", m_mdiwnd->m_tunnel, &m_mdiwnd->m_mysql)]);
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
	
	
    if(myrowstr.GetLength() >= newpos && flag == wyTrue && (tempstr.CompareI("innodb") == 0))
		myrowstr.Strip(myrowstr.GetLength() - (newpos));
	
	/// Gets the comment
    m_origcomment.SetAs(myrowstr.GetString());
    
	/// Get Max row
    if((val =  GetValues(myrow[index], "max_rows", retval)) > 0)
        m_origmaxrows.SetAs(retval.GetString());

    /// Get the min rows
    if((val =  GetValues(myrow[index], "min_rows", retval)) > 0)
        m_origminrows.SetAs(retval.GetString());

    /// Get average row length
    if((val =  GetValues(myrow[index], "avg_row_length", retval)) >= 0)
        m_origavgrow.SetAs(retval.GetString());

    /// Get checksum 
    if((val =  GetValues(myrow[index], "checksum", retval)) >= 0)
        m_origchecksum.SetAs(retval.GetString());
    else
        m_origchecksum.SetAs("0");

    /// Get delay key write
    if((val =  GetValues(myrow[index], "delay_key_write", retval)) >= 0)
        m_origdelay.SetAs(retval.GetString());
    else
        m_origdelay.SetAs("0");

    /// Get the row format
    if((val =  GetValues(myrow[index], "row_format", retval)) >= 0)
        m_origrowformat.SetAs(retval.GetString());
    else
        m_origrowformat.SetAs("");

    return wyTrue;
}

wyBool
TabAdvancedProperties::FillInitValues(wyBool iscancelchanges)
{
    wyInt32		index = 0;

    m_disableenchange = wyTrue;

    if(!m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        if(!iscancelchanges)
        {
            SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbtabletype, CB_SELECTSTRING, index, (LPARAM) TEXT(STR_DEFAULT));
            if(m_ismysql41 == wyTrue)
            {
                SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbcharset, CB_SELECTSTRING, -1, (LPARAM) TEXT(STR_DEFAULT));
                SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbcollation, CB_SELECTSTRING, -1, (LPARAM) TEXT(STR_DEFAULT));
            }
        }
        SendMessage(m_hcmbdelaykeywrite, CB_SELECTSTRING, -1, (LPARAM) TEXT(STR_DEFAULT)); 
        SendMessage(m_hcmbrowformat, CB_SELECTSTRING, -1, (LPARAM) TEXT(STR_DEFAULT)); 
        SendMessage(m_hcmbchecksum, CB_SELECTSTRING, -1, (LPARAM) TEXT(STR_DEFAULT)); 
    }
    else
    {
        if(!iscancelchanges)
        {
            SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbtabletype, CB_SELECTSTRING, index, (LPARAM)m_ptabmgmt->m_tabinterfaceptr->m_origtableengine.GetAsWideChar());
            if(m_ismysql41 == wyTrue)
            {
                SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbcharset, CB_SELECTSTRING, -1, (LPARAM) m_ptabmgmt->m_tabinterfaceptr->m_origcharset.GetAsWideChar());
                m_ptabmgmt->m_tabinterfaceptr->HandleTabCharset();
                SendMessage(m_ptabmgmt->m_tabinterfaceptr->m_hcmbcollation, CB_SELECTSTRING, -1, (LPARAM) m_ptabmgmt->m_tabinterfaceptr->m_origcollate.GetAsWideChar());
            }
        }
        if(m_origdelay.GetLength())
            SendMessage(m_hcmbdelaykeywrite, CB_SELECTSTRING, -1, (LPARAM) m_origdelay.GetAsWideChar());
        else
            SendMessage(m_hcmbdelaykeywrite, CB_SETCURSEL, -1, (LPARAM) 0);

        if(m_origrowformat.GetLength())
            SendMessage(m_hcmbrowformat, CB_SELECTSTRING, -1, (LPARAM) m_origrowformat.GetAsWideChar());
        else
            SendMessage(m_hcmbrowformat, CB_SETCURSEL, -1, (LPARAM)NULL);

        if(m_origchecksum.GetLength())
            SendMessage(m_hcmbchecksum, CB_SELECTSTRING, -1, (LPARAM) m_origchecksum.GetAsWideChar());
        else
            SendMessage(m_hcmbchecksum, CB_SETCURSEL, -1, (LPARAM) 0);

    }
    
    SendMessage(m_heditcomment, WM_SETTEXT, index, (LPARAM) m_origcomment.GetAsWideChar());
    SendMessage(m_heditautoincr, WM_SETTEXT, index, (LPARAM) m_origautoincr.GetAsWideChar());
    
    SendMessage(m_heditmaxrows, WM_SETTEXT, 0, (LPARAM) m_origmaxrows.GetAsWideChar());
    SendMessage(m_heditminrows, WM_SETTEXT, 0, (LPARAM) m_origminrows.GetAsWideChar());
    SendMessage(m_heditavgrowlen, WM_SETTEXT, 0, (LPARAM) m_origavgrow.GetAsWideChar());
    
    m_disableenchange = wyFalse;

    return wyTrue;
}

wyInt32 
TabAdvancedProperties::GetValues(wyChar *textrow, wyChar *tofind, wyString &retval)
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
TabAdvancedProperties::Create()
{
    /// Creating controls for advanced properties
    CreateOtherWindows(m_hwnd);

    /// Initislizing all controls
    InitAllWindows(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable ? wyFalse : wyTrue);

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        /// Fetching Advanced Properties
        if(!FetchInitValues())
            return wyFalse;

        /// Setting all fetched values to the controlls
        FillInitValues();
    }

    /// Unsetting flag
    m_disableenchange = wyFalse;
    return wyTrue;
}

void
TabAdvancedProperties::GetMaxWidthOfStaticText(wyInt32* col1)
{
    wyInt32     size = 0;
    wyInt32     max  = 0;

    size = GetTextLenOfStaticWindow(m_hstaticcomment);
    if(size > max)
        max = size;

    size = GetTextLenOfStaticWindow(m_hstatautoincr);
    if(size > max)
        max = size;

    size = GetTextLenOfStaticWindow(m_hstatavgrowlen);
    if(size > max)
        max = size;

    size = GetTextLenOfStaticWindow(m_hstatrowformat);
    if(size > max)
        max = size;
	
	size = GetTextLenOfStaticWindow(m_hstatchecksum);
    if(size > max)
        max = size;
	
	size = GetTextLenOfStaticWindow(m_hstatmaxrows);
    if(size > max)
        max = size;

    size = GetTextLenOfStaticWindow(m_hstatminrows);
    if(size > max)
        max = size;

    size = GetTextLenOfStaticWindow(m_hstatdelaykeywrite);
    if(size > max)
        max = size;
	
	*col1 = max;

}

VOID
TabAdvancedProperties::Resize()
{
    RECT			rcwnd, rcmain, rctemp = {0};
    wyInt32         minxpos, minypos;
    wyInt32         staticwndwidthcol1 = 0;
    wyInt32         dist_static_edit = 8;   //..Distance from static to edit
	wyInt32			textheight = 0;

    VERIFY(GetWindowRect(m_hwnd, &rcmain));
    VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcmain, 2));
    
    GetWindowRect(m_hstaticcomment, &rctemp);
    MapWindowPoints(NULL, m_hstaticcomment, (LPPOINT) &rctemp, 2);
    
    minxpos = rctemp.left + 13;
    minypos = rctemp.top + 20;
    GetMaxWidthOfStaticText(&staticwndwidthcol1);
	textheight = GetStaticTextHeight(m_hstaticcomment);


    MoveWindow(m_hstaticcomment     , minxpos       , minypos + 4 , staticwndwidthcol1   , 20,   TRUE);
    MoveWindow(m_heditcomment       , staticwndwidthcol1 + dist_static_edit + minxpos, minypos    , 200 , 50,   TRUE);

	MoveWindow(m_hstatautoincr     , minxpos       , minypos + 69 , staticwndwidthcol1, 20,   TRUE);
    MoveWindow(m_heditautoincr      , staticwndwidthcol1 + dist_static_edit + minxpos, minypos + 65  , 150   , 20,   TRUE);

    MoveWindow(m_hstatavgrowlen     , minxpos       , minypos + 104 , staticwndwidthcol1  , 20,  TRUE);
    MoveWindow(m_heditavgrowlen     , staticwndwidthcol1 + dist_static_edit + minxpos, minypos + 100  , 150  , 20,  TRUE);
    
	MoveWindow(m_hstatmaxrows       , minxpos , minypos + 139, staticwndwidthcol1   , 20,    TRUE);
    MoveWindow(m_heditmaxrows       , minxpos + dist_static_edit + staticwndwidthcol1 , minypos + 135  , 150   , 20,    TRUE);
    
    MoveWindow(m_hstatminrows       , minxpos , minypos + 174 , staticwndwidthcol1   , 20,    TRUE);
    MoveWindow(m_heditminrows       , minxpos + dist_static_edit + staticwndwidthcol1, minypos + 170 , 150   , 20,    TRUE);
    
	MoveWindow(m_hstatrowformat     , minxpos       , minypos + 209 , staticwndwidthcol1  , 20,    TRUE);
    MoveWindow(m_hcmbrowformat      , staticwndwidthcol1 + dist_static_edit  + minxpos, minypos + 205 , 150   , 20,    TRUE);
        
    MoveWindow(m_hstatchecksum      , minxpos       , minypos + 244 , staticwndwidthcol1 , 20,   TRUE);
    MoveWindow(m_hcmbchecksum       , staticwndwidthcol1 + dist_static_edit + minxpos, minypos + 240 , 150   , 20,    TRUE);

    MoveWindow(m_hstatdelaykeywrite , minxpos , minypos + 279 , staticwndwidthcol1   , 20,    TRUE);
    MoveWindow(m_hcmbdelaykeywrite  , minxpos + dist_static_edit + staticwndwidthcol1, minypos + 275 , 150   , 20,    TRUE);
    
	GetSystemMetrics(SM_CYVTHUMB);
	
	MoveWindow(m_hwndscroll, rcmain.right - GetSystemMetrics(SM_CYVTHUMB), rcmain.left, GetSystemMetrics(SM_CYVTHUMB), rcmain.bottom, TRUE);

	SetScrollRange(m_hwndscroll, SB_CTL, 0, MAX_HEIGHT, TRUE);

	if((rcmain.bottom - rcmain.top) >= MAX_HEIGHT)
	{
		ShowWindow(m_hwndscroll, SW_HIDE);
	}
	else 
	{
		SCROLLINFO	scinfo= {0};
		GetClientRect(m_hwnd, &m_rectparent);
		m_rectparent.right = m_rectparent.right - GetSystemMetrics(SM_CYVTHUMB);
		m_dispheight = m_rectparent.bottom - m_rectparent.top;
		if(m_ptabmgmt->GetActiveTabImage() == IDI_TABLEOPTIONS)
		{
			m_scrollpos = GetScrollPos(m_hwndscroll, SB_CTL);
		}

		if(m_dispheight < MAX_HEIGHT)
		{
			scinfo.cbSize =  sizeof(SCROLLINFO);
			scinfo.fMask = SIF_ALL;

            //..set the scroll bar position for the vertical scroll box.
			scinfo.nPos = m_scrollpos;
			scinfo.nMin = 0;
			scinfo.nMax = MAX_HEIGHT + 15;
			scinfo.nPage = m_dispheight;
			SetScrollInfo(m_hwndscroll, SB_CTL, &scinfo, TRUE);	
			if(m_scrollpos)
			{
				ScrollWindows(wyTrue, (0 - m_scrollpos));
			}
			if(m_ptabmgmt->GetActiveTabImage() == IDI_TABLEOPTIONS)
				ShowWindow(m_hwndscroll, SW_SHOW);   
			
		}
	}
	
    if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
    {
        //..To avoid painting issue
        GetClientRect(m_ptabmgmt->m_hcommonwnd, &rcwnd);
        InvalidateRect(m_ptabmgmt->m_hcommonwnd, &rcwnd, FALSE);
    }
}

void
TabAdvancedProperties::SetStaticWindowPosition(HWND hwnd, HWND hwndedit, wyInt32 lpos, wyInt32 width, wyInt32 height)
{
    RECT            rctxt = {0}, rcedit = {0};
    HDC             hdc;
    wyWChar         lblcaption[SIZE_256];

    GetWindowText(hwnd, lblcaption, SIZE_256-1);
    hdc = GetDC(hwnd);
    DrawText(hdc, lblcaption, wcslen(lblcaption), &rctxt, DT_CALCRECT);
    
    GetWindowRect(hwndedit, &rcedit);
    MapWindowRect(NULL, m_hwnd, &rcedit);
    
    SetWindowPos(hwnd, NULL, lpos, rcedit.top + ((rcedit.bottom - rcedit.top) - (rctxt.bottom - rctxt.top)) / 2, width   , height, SWP_NOZORDER);
    
    ReleaseDC(hwnd, hdc);
}

void
TabAdvancedProperties::GenerateCreateQuery(wyString &query)
{
    wyInt32		    selindex, ret;
    wyWChar         textbuf[2049]={0};           // none of the value will go more then this and neway we send the max len to the api so it will not fail
    wyChar*         commentbuff = {0};
    wyString        enginestr, rowformat, tempstr;
    wyString        advpropval;
    wyInt32         counter = 0;
    
    ret = SendMessage(m_hcmbchecksum , CB_GETCURSEL, 0, 0);

    if(ret != CB_ERR)
    {
        SendMessage(m_hcmbchecksum , CB_GETLBTEXT, ret, (LPARAM)textbuf);
        tempstr.SetAs(textbuf);
        if(tempstr.Compare(STR_DEFAULT) != 0 && tempstr.GetLength())
        {
            FormatString(advpropval, &counter);
            advpropval.AddSprintf("checksum=%s ", tempstr.GetString());
        }
    }
    
    // 3. The autoincr thing.
    SendMessage(m_heditautoincr, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength())
    {
        FormatString(advpropval, &counter);
        advpropval.AddSprintf("auto_increment=%s ", tempstr.GetString());
    }

    // 4. The average row thing.
	SendMessage(m_heditavgrowlen, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength())
    {
        FormatString(advpropval, &counter);
		advpropval.AddSprintf("avg_row_length=%s ", tempstr.GetString());
    }

    // 5. The comment.
	SendMessage(m_heditcomment, WM_GETTEXT, 2049, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    
    // Escape the comment string
	commentbuff = AllocateBuff((tempstr.GetLength() + 1) * 2);   // Considering allocation including escape characters
	m_mdiwnd->m_tunnel->mysql_real_escape_string(m_mdiwnd->m_mysql, commentbuff, tempstr.GetString(), tempstr.GetLength());
	tempstr.SetAs(commentbuff);
    
    if(commentbuff)
        free(commentbuff);

    if(tempstr.GetLength())
    {
        FormatString(advpropval, &counter);
		advpropval.AddSprintf("comment='%s' ", tempstr.GetString());
    }

    // 6. The max row thing.
    SendMessage(m_heditmaxrows, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength())
    {
        FormatString(advpropval, &counter);
		advpropval.AddSprintf("max_rows=%s ", tempstr.GetString());
    }

    // 7. The max row thing.
    SendMessage(m_heditminrows, WM_GETTEXT, 15, (LPARAM)textbuf);
	tempstr.SetAs(textbuf);
    if(tempstr.GetLength())
    {
        FormatString(advpropval, &counter);
		advpropval.AddSprintf("min_rows=%s ", tempstr.GetString());
    }


    // 8. The delay thing.
    VERIFY((selindex = SendMessage(m_hcmbdelaykeywrite, CB_GETCURSEL, 0, 0)) != CB_ERR);
	SendMessage(m_hcmbdelaykeywrite, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.CompareI(STR_DEFAULT) != 0)
    {
        FormatString(advpropval, &counter);
        advpropval.AddSprintf("delay_key_write=%s ", tempstr.GetString());
    }


    // 10. The row format thing.
	
	VERIFY((selindex = SendMessage(m_hcmbrowformat, CB_GETCURSEL, 0, 0)) != CB_ERR);
	SendMessage(m_hcmbrowformat, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
	// bug reported in http://www.webyog.com/forums//index.php?showtopic=3887 (solved in 6.52)
	if(tempstr.CompareI(STR_DEFAULT) != 0)
    {
        FormatString(advpropval, &counter);
        advpropval.AddSprintf("row_format=%s ", tempstr.GetString());
    }

    if(advpropval.GetLength())
        query.AddSprintf("%s", advpropval.GetString());
}

wyBool
TabAdvancedProperties::FormatString(wyString &query, wyInt32* ind)
{
    wyBool      ret = wyTrue;
    wyString    newline("\r\n");

    if((*ind) == 3)
        (*ind) = 0;

    if((*ind) % 3 == 0)
        query.Add(newline.GetString());

    (*ind)++;

    return ret;
}
void 
TabAdvancedProperties::GenerateAlterQuery(wyString &query)
{
    wyInt32		    selindex, ret;
    wyWChar         textbuf[2049]={0};           // none of the value will go more then this and neway we send the max len to the api so it will not fail
    wyChar*         commentbuff = {0};//, commentbuff2 = {0};
    wyString        enginestr, rowformat, tempstr, newline("\r\n   ");
    wyString        advpropval;
    //wyBool          ismysql41 = IsMySQL41(m_tunnel, m_mysql);
    wyString        dbname, tablename;
    wyInt32         tmpcount = 0;


    dbname.SetAs(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_dbname);
    tablename.SetAs(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_origtblname);

    ret = SendMessage(m_hcmbchecksum, CB_GETCURSEL, 0, 0);
    if(ret != CB_ERR)
    {
        SendMessage(m_hcmbchecksum, CB_GETLBTEXT, ret, (LPARAM)textbuf);
        tempstr.SetAs(textbuf);
        if(tempstr.GetLength() && tempstr.Compare(m_origchecksum) != 0 && tempstr.Compare(STR_DEFAULT) != 0)
        {
            FormatString(advpropval, &tmpcount);
            advpropval.AddSprintf("checksum=%s, ", tempstr.GetString());
        }
    }
    
    // 3. The autoincr thing.
    SendMessage(m_heditautoincr, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength() && tempstr.CompareI(m_origautoincr) != 0)
    {
        FormatString(advpropval, &tmpcount);
        advpropval.AddSprintf("auto_increment=%s, ", tempstr.GetString());
    }
    
    // 4. The average row thing.
	SendMessage(m_heditavgrowlen, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength() && tempstr.CompareI(m_origavgrow) != 0)
    {
        FormatString(advpropval, &tmpcount);
        advpropval.AddSprintf("avg_row_length=%s, ", tempstr.GetString());
    }
    else if(tempstr.GetLength() && m_origavgrow.GetLength())
    {
        if(tempstr.GetAsInt32() != m_origavgrow.GetAsInt32())
        {
            FormatString(advpropval, &tmpcount);
            advpropval.AddSprintf("avg_row_length=%s, ", tempstr.GetString());
        }
    }

    // 5. The comment thing.
	SendMessage(m_heditcomment, WM_GETTEXT, 2049, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    
    // Escape the comment string
	commentbuff = AllocateBuff((tempstr.GetLength() + 1) * 2);   // Considering allocation including escape characters
	m_mdiwnd->m_tunnel->mysql_real_escape_string(m_mdiwnd->m_mysql, commentbuff, tempstr.GetString(), tempstr.GetLength());
	tempstr.SetAs(commentbuff);
    
    wyString escapedstr;
    //delete commentbuff;       ///Uncomment afterwards
	free(commentbuff);
    commentbuff = NULL; //{0};
    
    commentbuff = AllocateBuff((m_origcomment.GetLength() + 1) * 2);   // Considering allocation including escape characters
	m_mdiwnd->m_tunnel->mysql_real_escape_string(m_mdiwnd->m_mysql, commentbuff, m_origcomment.GetString(), m_origcomment.GetLength());
	escapedstr.SetAs(commentbuff);
	free(commentbuff);

    if(escapedstr.CompareI(tempstr) != 0)
    {
        FormatString(advpropval, &tmpcount);
        advpropval.AddSprintf("comment='%s', ", tempstr.GetString());
    }


    // 6. The max row thing.
    SendMessage(m_heditmaxrows, WM_GETTEXT, 15, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);

    if(tempstr.GetLength() && tempstr.CompareI(m_origmaxrows) != 0)
    {
        FormatString(advpropval, &tmpcount);
        advpropval.AddSprintf("max_rows=%s, ", tempstr.GetString());
    }

    // 7. The min row thing.
    SendMessage(m_heditminrows, WM_GETTEXT, 15, (LPARAM)textbuf);
	tempstr.SetAs(textbuf);
    if(tempstr.GetLength() && tempstr.CompareI(m_origminrows)!= 0)
    {
        FormatString(advpropval, &tmpcount);
		advpropval.AddSprintf("min_rows=%s, ", tempstr.GetString());
    }

    // 8. The delay thing.
    selindex = SendMessage(m_hcmbdelaykeywrite, CB_GETCURSEL, 0, 0);
    if(selindex != CB_ERR)
    {
	SendMessage(m_hcmbdelaykeywrite, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
    if(tempstr.GetLength() && tempstr.CompareI(m_origdelay) != 0)
    {
        if(tempstr.CompareI(STR_DEFAULT) != 0)
        {
            FormatString(advpropval, &tmpcount);
            advpropval.AddSprintf("delay_key_write=%s, ", tempstr.GetString());
        }
    }
    }
    // 10. The row format thing.
	selindex = SendMessage(m_hcmbrowformat, CB_GETCURSEL, 0, 0);
    
    if(selindex != CB_ERR)
    {
	SendMessage(m_hcmbrowformat, CB_GETLBTEXT, selindex, (LPARAM)textbuf);
    tempstr.SetAs(textbuf);
	// bug reported in http://www.webyog.com/forums//index.php?showtopic=3887 (solved in 6.52)
    if(tempstr.GetLength() && tempstr.CompareI(m_origrowformat) != 0)
    {
        if(tempstr.CompareI(STR_DEFAULT) != 0)
        {
            FormatString(advpropval, &tmpcount);
            advpropval.AddSprintf("row_format=%s, ", tempstr.GetString());
        }
    }
    }
    
    advpropval.RTrim();

    advpropval.GetCharAt(advpropval.GetLength()-1) == ',' ? advpropval.Strip(1) : NULL;

    if(advpropval.GetLength())
        query.AddSprintf("%s", advpropval.GetString());
        
    return;
}

LRESULT CALLBACK 
TabAdvancedProperties::HandleTabKey(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TabAdvancedProperties   *tabadvprop = (TabAdvancedProperties*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch(message)
	{
    case WM_SYSKEYDOWN:
        {
            if(!tabadvprop->m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(CustomTab_GetCurSel(tabadvprop->m_ptabmgmt->m_hwnd), wParam, lParam))
                return 0;
        }
        break;

    case WM_GETDLGCODE:

        if(wParam == VK_ESCAPE)
        {
            /// Sending the UM_CLOSEDLG message to the tabbed-interface wndproc if the tabbed-interface is on dialog (SD), 
            if(tabadvprop->m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
                PostMessage(tabadvprop->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, UM_CLOSEDLG, wParam, lParam);
            return 0;
        }

        if(lParam)
        {
			return DLGC_WANTALLKEYS;
        }
        break;
    }

    return CallWindowProc(tabadvprop->m_wporigcomment, hwnd, message, wParam, lParam);
}

wyInt32 
TabAdvancedProperties::OnTabSelChange()
{
    HWND   hwndarr[25] = {m_hstaticcomment, m_heditcomment, m_hstatautoincr, m_heditautoincr,
                          m_hstatavgrowlen, m_heditavgrowlen, m_hstatmaxrows, m_heditmaxrows,
                          m_hstatminrows, m_heditminrows, m_hstatrowformat, m_hcmbrowformat,
						  m_hstatdelaykeywrite, m_hcmbdelaykeywrite, m_hstatchecksum,  m_hcmbchecksum,
                          NULL};
    
    //..Focus issue when user changes from Advanced to Preview Tab and comes back
    //..stored temporarily, because, ShowWindowsProc calls ShowWindow() api and that further sends EN_SETFOCUS message to commonwndproc with preview handle as WPARAM
    HWND    templastfocus = m_hlastfocusedwnd;

    EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);

	SCROLLINFO	scinfo= {0};
	GetClientRect(m_hwnd, &m_rectparent);
	m_rectparent.right = m_rectparent.right - GetSystemMetrics(SM_CYVTHUMB);
	m_dispheight = m_rectparent.bottom - m_rectparent.top;
	if(m_dispheight < MAX_HEIGHT)
	{
		scinfo.cbSize =  sizeof(SCROLLINFO);
		scinfo.fMask = SIF_ALL;

        // set the scroll bar position for the vertical scroll box.
		scinfo.nPos = m_scrollpos;
		scinfo.nMin = 0;
		scinfo.nMax = MAX_HEIGHT + 15;
		scinfo.nPage = m_dispheight;
		SetScrollInfo(m_hwndscroll, SB_CTL, &scinfo, FALSE);	
		/*if(m_scrollpos)
			ScrollWindows(wyTrue, m_scrollpos);*/
		SetScrollPos(m_hwndscroll, SB_CTL, m_scrollpos, TRUE);
		ShowWindow(m_hwndscroll, SW_SHOW);   
	}
    
    //..Restoring back the lastfocused window handle
    m_hlastfocusedwnd = templastfocus;

    PostMessage(m_ptabmgmt->m_hcommonwnd, UM_SETFOCUS, (WPARAM)m_hlastfocusedwnd, NULL);
    return 1;
}

void
TabAdvancedProperties::CancelChanges(wyBool isaltertable)
{
    FillInitValues(wyTrue);
}

wyBool 
TabAdvancedProperties::GenerateQuery(wyString &query)
{
    wyBool  retval = wyTrue;

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
        GenerateAlterQuery(query);
    else
        GenerateCreateQuery(query);

    return retval;
}

void
TabAdvancedProperties::ScrollWindows(wyBool isdown, wyInt32 pos)
{
	HWND   hwndarr[25] = {m_hwnd, m_hwndscroll, m_hstaticcomment, m_heditcomment, m_hstatautoincr, m_heditautoincr,
                          m_hstatavgrowlen, m_heditavgrowlen, m_hstatmaxrows, m_heditmaxrows,
                          m_hstatminrows, m_heditminrows, m_hstatrowformat, m_hcmbrowformat,
						  m_hstatdelaykeywrite, m_hcmbdelaykeywrite, m_hstatchecksum,  m_hcmbchecksum,  
                          m_ptabmgmt->m_tabinterfaceptr->m_hbtnsave, m_ptabmgmt->m_tabinterfaceptr->m_hbtncancelchanges,
                          NULL};
	wyInt32 temppos = 0;
	if(m_scrollpos)
		temppos = m_scrollpos;
	if(pos!=0)
	{
		m_scrollpos = pos;
		EnumChildWindows(m_hwnd, TabAdvancedProperties::ScrollWindowsByPosProc, (LPARAM)hwndarr);
		m_scrollpos = temppos;
	}
	else
	{
	if(isdown)
		EnumChildWindows(m_hwnd, TabAdvancedProperties::ScrollWindowsDownProc, (LPARAM)hwndarr);
	else
		EnumChildWindows(m_hwnd, TabAdvancedProperties::ScrollWindowsUpProc, (LPARAM)hwndarr);
	}
	UpdateWindow(m_hwndscroll);
    InvalidateRect(m_hwnd, NULL, TRUE);
	UpdateWindow(m_hwnd);
}

BOOL CALLBACK 
TabAdvancedProperties::ScrollWindowsDownProc(HWND hwnd, LPARAM lParam)
{
    HWND*       hwndarr;
    wyUInt32    ind = 2; //first hwnd is the parent hwnd, 2nd hwnd is the scrollbar
	RECT rect, rect2;
	hwndarr = NULL;
    hwndarr = (HWND*)lParam;
	
    if(!hwndarr)
		return TRUE;

    while(hwndarr[ind])
    {
        if(hwnd == hwndarr[ind])
        {
			GetWindowRect(hwnd, &rect);
			GetClientRect(hwndarr[0], &rect2);
			MapWindowRect(NULL, hwndarr[0] ,&rect);
			MoveWindow(hwnd, rect.left, rect.top-10, rect.right-rect.left, rect.bottom - rect.top, TRUE);
			UpdateWindow(hwnd);
			return TRUE;
        }
        ind++;
    }
    //ShowWindow(hwnd, SW_HIDE);
    return TRUE;
}

BOOL CALLBACK 
TabAdvancedProperties::ScrollWindowsUpProc(HWND hwnd, LPARAM lParam)
{
    HWND*       hwndarr;
     wyUInt32    ind = 2; //first hwnd is the parent hwnd, 2nd hwnd is the scrollbar
	RECT rect, rect2;
    hwndarr = NULL;
    hwndarr = (HWND*)lParam;
	
	if(!hwndarr)
		return TRUE;

    while(hwndarr[ind])
    {
        if(hwnd == hwndarr[ind])
        {
			GetWindowRect(hwnd, &rect);
			GetClientRect(hwndarr[0], &rect2);
			MapWindowRect(NULL, hwndarr[0] ,&rect);
			MoveWindow(hwnd, rect.left, rect.top+10, rect.right-rect.left, rect.bottom - rect.top, TRUE);
			UpdateWindow(hwnd);
            return TRUE;
        }
        ind++;
    }
    //ShowWindow(hwnd, SW_HIDE);
    return TRUE;
}

BOOL CALLBACK 
	TabAdvancedProperties::ScrollWindowsByPosProc(HWND hwnd, LPARAM lParam)
{
    HWND*       hwndarr;
    wyUInt32    ind = 2;
	RECT rect, rect2;

    hwndarr = NULL;
    hwndarr = (HWND*)lParam;
    if(!hwndarr)
		return TRUE;

	TabAdvancedProperties   *tabadvprop = (TabAdvancedProperties*) GetWindowLongPtr(hwndarr[1], GWLP_USERDATA);

    while(hwndarr[ind])
    {
        if(hwnd == hwndarr[ind])
        {
			GetWindowRect(hwnd, &rect);
			GetClientRect(hwndarr[0], &rect2);
			MapWindowRect(NULL, hwndarr[0] ,&rect);
			//poschange = tabadvprop->m_prevscrollpos - tabadvprop->m_scrollpos;
			MoveWindow(hwnd, rect.left, rect.top + tabadvprop->m_scrollpos, rect.right-rect.left, rect.bottom - rect.top, TRUE);
			UpdateWindow(hwnd);
            return TRUE;
        }
        ind++;
    }
    //ShowWindow(hwnd, SW_HIDE);
    return TRUE;
}
void
TabAdvancedProperties::OnMouseWheel(WPARAM wParam)
{
	wyInt32 CurPos, newpos;
	CurPos = GetScrollPos(m_hwndscroll, SB_CTL);
	if((short) HIWORD(wParam)> 0) 
	{
		if (CurPos > 10)
		{
			CurPos = CurPos-10;
			ScrollWindows(wyFalse);
		}
		else if(CurPos > 0)
		{
			ScrollWindows(wyFalse, (CurPos));
			CurPos = 0;
		}
	}
	else
	{
		if (CurPos < (MAX_HEIGHT - m_dispheight - 10))
		{
			CurPos = CurPos+10;
			ScrollWindows(wyTrue);
		}
		else if(CurPos < (MAX_HEIGHT - m_dispheight))
		{
			newpos = (MAX_HEIGHT - m_dispheight);
			ScrollWindows(wyFalse, (CurPos - newpos));
			CurPos = newpos;
		}
	}
	SetScrollPos(m_hwndscroll, SB_CTL, CurPos, TRUE);
}

void
TabAdvancedProperties::OnVScroll(WPARAM wParam)
{
	wyInt32 	CurPos, scrollpos, newpos;
	CurPos = GetScrollPos(m_hwndscroll, SB_CTL);

	switch (LOWORD(wParam))
    {
		case SB_LINEUP:
			if (CurPos > 10)
            {
				CurPos = CurPos-10;
				ScrollWindows(wyFalse);
			}
			else if(CurPos > 0)
			{
				ScrollWindows(wyFalse, (CurPos));
				CurPos = 0;
			}
			break;
			
		case SB_THUMBTRACK:
        	scrollpos = HIWORD(wParam);
			if(scrollpos == CurPos)
				break;
			
			newpos = CurPos - scrollpos;
			ScrollWindows(wyTrue, newpos);
			CurPos = scrollpos;
			break;

		case SB_LINEDOWN:
			if (CurPos < ((MAX_HEIGHT + 15) - m_dispheight - 10))
			{
				CurPos = CurPos+10;
				ScrollWindows(wyTrue);
			}
			else if(CurPos < ((MAX_HEIGHT + 15) - m_dispheight))
			{
				newpos = ((MAX_HEIGHT + 15) - m_dispheight);
				ScrollWindows(wyFalse, (CurPos - newpos));
				CurPos = newpos;
			}
			break;

		case SB_PAGEDOWN:
			newpos = CurPos + m_dispheight;
			newpos = newpos < (MAX_HEIGHT + 15 - m_dispheight) ? newpos:((MAX_HEIGHT + 15) - m_dispheight);
			ScrollWindows(wyTrue,  CurPos - newpos);
			CurPos = newpos;
			break;

		case SB_PAGEUP:
			newpos = CurPos - m_dispheight;
			newpos = newpos > 0 ? newpos:0;
			ScrollWindows(wyTrue,  CurPos - newpos);
			CurPos = newpos;
			break;
	}
	SetScrollPos(m_hwndscroll, SB_CTL, CurPos, TRUE);
}

void 
TabAdvancedProperties::OnTabSelChanging()
{
	//Save scroll position (if any);
	m_scrollpos = GetScrollPos(m_hwndscroll, SB_CTL);
    m_hlastfocusedwnd = GetFocus();
}