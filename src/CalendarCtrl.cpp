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

#include "CalendarCtrl.h"
#include "FrameWindowHelper.h"
#ifndef COMMUNITY
#include "formview.h"
#endif


/* ----------------------------------------------------------------------------------------
	Implementation of Calendar Control dialog box. 
	This dialog box is used when the user edits a 
	datetime, timestamp, date, or time field.
   --------------------------------------------------------------------------------------*/

CalendarCtrl::CalendarCtrl()
{
	m_hwnd = NULL;
	m_isForm = wyFalse;
	m_isDate = wyFalse;
	m_isResult = wyFalse;
}

CalendarCtrl::~CalendarCtrl()
{
}

#ifndef COMMUNITY
//Sets the parameters of the control in Formview
HWND
CalendarCtrl::CreateForm(htmlayout::dom::element hwndedit, FormView *pfv, wyBool isDate)
{
	m_edit = hwndedit;
	m_hwndparent = m_edit.get_element_hwnd(true);
	m_button = m_edit.next_sibling();
	
	m_pfv = pfv;
	
	if(pfv->m_pdv)
	{
		m_dv = pfv->m_pdv;
	}
	
	m_isDate = isDate;
	m_orgdata.SetAs(m_edit.text());
	m_isForm = wyTrue;
	m_rectCell = m_button.get_location();
	
	while(m_edit.get_ctl_type() != CTL_EDIT)
	{
		m_edit = m_edit.prev_sibling();
	}
	
	m_hwnd = Create();
	
	if(isDate)
	{
		ShowWindow(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),SW_HIDE);
	}
	
	PositionWindow(&m_rectCell);
	ShowWindow(m_hwnd,SW_SHOW);
    m_dv->m_hwndcal = m_hwnd;
    //m_dvb->m_hwndcal = m_hwnd;
	
	//set focus to the edit box
	m_edit.set_state(STATE_FOCUS);
	
	return m_hwnd;
}

#endif

//Sets the parameters of the control in Gridview
HWND
CalendarCtrl::CreateGrid(HWND hwndparent, wyWChar* olddat, DataViewBase *dvb, wyBool isDate, wyBool isResult)
{
	m_hwndparent= hwndparent;
	m_orgdata.SetAs(olddat);
	m_isDate = isDate;
	m_date.SetAs(olddat);
	m_dvb = dvb;
	m_isResult = isResult;

	m_hwnd = Create();
	
	if(isDate)
		ShowWindow(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),SW_HIDE);

	CustomGrid_GetSubItemRect(m_hwndparent, m_row, m_col, &m_rectCell);
 
	PositionWindow(&m_rectCell);
	ShowWindow(m_hwnd,SW_SHOW);
	
	return m_hwnd;
}

HWND
CalendarCtrl::CreateGrid(HWND hwndparent, DataView *dv, wyBool isDate, wyBool isResult)
{
	m_hwndparent= hwndparent;
    wyChar* temp = dv->m_data->m_rowarray->GetRowExAt(CustomGrid_GetCurSelRow(dv->m_hwndgrid))->m_row[CustomGrid_GetCurSelCol(dv->m_hwndgrid)];
	if(temp)
		m_orgdata.SetAs(temp);
	else
		m_orgdata.SetAs(L"(NULL)");
	m_isDate = isDate;
	m_date.SetAs(m_orgdata);
	m_dv	= dv;
	m_isResult = isResult;

	m_hwnd = Create();
	
	if(isDate)
		ShowWindow(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),SW_HIDE);

	CustomGrid_GetSubItemRect(m_hwndparent, m_row, m_col, &m_rectCell);
 
	PositionWindow(&m_rectCell);
	ShowWindow(m_hwnd,SW_SHOW);
	
	return m_hwnd;
}


//Function to create the dialogbox
HWND
CalendarCtrl::Create()
{
	 HWND hwnd = CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DATETIME), m_hwndparent, CalendarCtrl::CalendarCtrlProc, (LPARAM)this);
	 m_calproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1),GWLP_WNDPROC,(LONG_PTR)CalendarCtrl::CalendarProc);
	 m_timeproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),GWLP_WNDPROC,(LONG_PTR)CalendarCtrl::TimeProc);
	 SetWindowLongPtr(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1),GWLP_USERDATA,(LONG_PTR)this);
	 SetWindowLongPtr(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),GWLP_USERDATA,(LONG_PTR)this);
	 return hwnd;
}


// Calendar control Main Procedure

INT_PTR
CalendarCtrl::CalendarCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CalendarCtrl* pcc = (CalendarCtrl*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
	switch(message)
	{
		case WM_INITDIALOG:
			pcc = (CalendarCtrl*)lParam;
			pcc->m_hwnd = hwnd;
			SetWindowLongPtr(hwnd, GWLP_USERDATA,lParam);
			LocalizeWindow(hwnd);
			pcc->InitCalendarValues();
			break;

		case WM_LBUTTONDOWN:
			pcc->OnLButtonDown(lParam);
			break;

		case WM_MOUSEWHEEL:
			SendMessage(hwnd, UM_ENDDIALOG,0,0);
			break;

		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_ESCAPE:
					SendMessage(pcc->m_hwnd,WM_COMMAND,IDCANCEL,0);
					return 0;
				case VK_RETURN:
					SendMessage(pcc->m_hwnd,WM_COMMAND,IDOK,0);
					return 0;
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCURRDATE:
					pcc->OnClickNow(pcc->m_dv);
					break;

				case IDOK:
					pcc->OnClickOk();
					break;
		
				case IDCANCEL:
					pcc->OnClickCancel();
					break;
			}
			break;

		case UM_ENDDIALOG:
#ifndef COMMUNITY
			if(pcc->m_isForm)
			{
				pcc->m_button.set_state(0,STATE_DISABLED);
				DestroyWindow(pcc->m_hwnd);
				//pcc->m_dvb->m_hwndcal = NULL;
				pcc->m_edit.set_state(STATE_FOCUS);
			}
			else
#endif
			{
				if(pcc->m_hwnd)
					DestroyWindow(pcc->m_hwnd);
				//pcc->m_dvb->m_hwndcal = NULL;
			}
			if(lParam==1)
				delete pcc;
			return 0;

		case WM_NCDESTROY:
			break;
	}
	return 0;
}


//Subclassed Calender procedure to set the focus in the dialog box on click
wyInt32	CALLBACK CalendarCtrl::CalendarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CalendarCtrl* pcc = (CalendarCtrl*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
	switch(message)
	{
		case WM_LBUTTONDOWN:
			SendMessage(pcc->m_hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(pcc->m_hwnd,IDC_DATETIMEPICKER1), TRUE);
			SetFocus(GetDlgItem(pcc->m_hwnd,IDC_DATETIMEPICKER1));
			EnableWindow(GetDlgItem(pcc->m_hwnd,IDOK), TRUE);
			break;
	}
	return pcc->m_calproc(hwnd,message,wParam,lParam);
}


//Subclassed Time procedure to deal with escape and enter keys
wyInt32	CALLBACK CalendarCtrl::TimeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CalendarCtrl* pcc = (CalendarCtrl*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
	switch(message)
	{
		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_ESCAPE:
					SendMessage(pcc->m_hwnd,WM_COMMAND,IDCANCEL,0);
					return 0;

				case VK_RETURN:
					SendMessage(pcc->m_hwnd,message,wParam,lParam);
					return 0;
			}

	}

	return pcc->m_timeproc(hwnd,message,wParam,lParam);
}


// Convert the values from calendar to string
void
CalendarCtrl::ConvertCtrlValues()
{
    wyString    temp;
	m_date.Clear();
	SYSTEMTIME seltime;
	
	MonthCal_GetCurSel(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1), &seltime);
	m_date.AddSprintf("%04u-%02u-%02u",seltime.wYear,seltime.wMonth,seltime.wDay);
	
	if(!m_isDate)
	{
		DateTime_GetSystemtime(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1), &seltime);
		m_date.AddSprintf(" %02u:%02u:%02u",seltime.wHour,seltime.wMinute,seltime.wSecond);
	}
}

// convert the values from the grid into SYSTIME struct

void
CalendarCtrl::InitCalendarValues()
{
	wyInt32 iszerodateflag=0;
	wyString temp,temp2,settozero;
	m_row = CustomGrid_GetCurSelRow(m_hwndparent);
	m_col = CustomGrid_GetCurSelCol(m_hwndparent);
	wyChar tempchar[5]= "";
	wyString year,month,day;
	wyChar * tempstr = NULL;
	wyBool pretoshowserverdate;
	MYSQL			*mysql;
	Tunnel			*tunnel;
	wyString	    query("select NOW()");
	MYSQL_RES	    *res;
	MYSQL_ROW	    row;
	wyString		myrowstr;
	wyInt32			qret, checkint=1;

	//Get preference setting to show server date or system date
	pretoshowserverdate = GetPreferenceForNowButton();

	if (pretoshowserverdate && m_dv != NULL && m_dv->m_data != NULL && m_dv->m_data->m_pmdi != NULL && m_dv->m_data->m_pmdi->m_tunnel != NULL && m_dv->m_data->m_pmdi->m_mysql != NULL)
	{
		tunnel = m_dv->m_data->m_pmdi->m_tunnel;
		mysql = m_dv->m_data->m_pmdi->m_mysql;
		qret = HandleMySQLRealQuery(tunnel, mysql, query.GetString(), query.GetLength(), wyTrue, wyTrue, wyTrue);

		res = tunnel->mysql_store_result(mysql, wyTrue);
		/* we specifically ignore empty queries */
		if (res == NULL && mysql->affected_rows == -1)
		{
			return;
		}
		row = tunnel->mysql_fetch_row(res);
		myrowstr.SetAs(row[0]);
	}

// Year Section
	tempstr = m_orgdata.Substr(0,4);
	if(tempstr !=NULL)
	{	
		year.SetAs(tempstr);
		if(year.CompareI("0000")==0)
		{
			strcpy(tempchar,"1970");
			iszerodateflag=-1;
		}
		else {
				strcpy(tempchar, year.GetString());
				settozero.Add(tempchar);
				checkint = settozero.GetAsUInt32();
				if (pretoshowserverdate && checkint==0)
				{
					strcpy(tempchar, myrowstr.Substr(0, 4));
				}
		}
	}
	else
	{
		strcpy(tempchar,"0");
	}
	temp.Add(tempchar);
	m_datetime.wYear=temp.GetAsUInt32();
	temp.Clear();

// Month Section
	tempstr = m_orgdata.Substr(5,2);
	if(tempstr !=NULL)
	{	
		month.SetAs(tempstr);
		if(month.CompareI("00")==0)
		{
			strcpy(tempchar,"01");
			iszerodateflag=-1;
		}
		else {
				strcpy(tempchar, month.GetString());
				settozero.Add(tempchar);
				checkint = -1;
				checkint = settozero.GetAsUInt32();
				if (pretoshowserverdate && checkint == 0)
				{
				strcpy(tempchar, myrowstr.Substr(5, 2));
			}
		}
	}
	else
	{
		strcpy(tempchar,"0");
	}
	temp.Add(tempchar);
	m_datetime.wMonth=temp.GetAsUInt32();
	temp.Clear();
// Date Section
	
	tempstr = m_orgdata.Substr(8,2);
	if(tempstr !=NULL)
	{	
		day.SetAs(tempstr);
		if(day.CompareI("00")==0)
		{
			strcpy(tempchar,"01");
			iszerodateflag=-1;
		}
		else {
				strcpy(tempchar, day.GetString());
				settozero.Add(tempchar);
				checkint = -1;
				checkint = settozero.GetAsUInt32();
				if (pretoshowserverdate && checkint == 0)
				{
					strcpy(tempchar, myrowstr.Substr(8, 2));
				}
				
		}
	}
	else
	{
		if (pretoshowserverdate)
		{
			strcpy(tempchar, myrowstr.Substr(8, 2));
		}
		else {
			strcpy(tempchar, "0");
		}
	}
	temp.Add(tempchar);
	m_datetime.wDay=temp.GetAsUInt32();
	temp.Clear();

	
	if(m_orgdata.Substr(11,2)==NULL && !pretoshowserverdate)
		strcpy(tempchar,"0");
	else {
		if (m_orgdata.Substr(11, 2) == NULL)
		{
			strcpy(tempchar, myrowstr.Substr(11, 2));
		}
		else {
			strcpy(tempchar, m_orgdata.Substr(11, 2));
			settozero.Add(tempchar);
			checkint = -1;
			checkint = settozero.GetAsUInt32();
			if (pretoshowserverdate && checkint == 0)
			{
				strcpy(tempchar, myrowstr.Substr(11, 2));
			}
		}
			
	}

	temp.Add(tempchar);
	m_datetime.wHour=temp.GetAsUInt32();
	temp.Clear();
	
	
	if(m_orgdata.Substr(14,2)==NULL && !pretoshowserverdate)
		strcpy(tempchar,"0");
	else {
		if (m_orgdata.Substr(14, 2) == NULL)
		{
			strcpy(tempchar, myrowstr.Substr(14, 2));
		}
		else {
			strcpy(tempchar, m_orgdata.Substr(14, 2));
			settozero.Add(tempchar);
			checkint = -1;
			checkint = settozero.GetAsUInt32();
			if (pretoshowserverdate && checkint == 0)
			{
				strcpy(tempchar, myrowstr.Substr(14, 2));
			}
		}

			
	}
	temp.Add(tempchar);
	m_datetime.wMinute=temp.GetAsUInt32();
	temp.Clear();

			
	if(m_orgdata.Substr(17,2)==NULL && !pretoshowserverdate)
		strcpy(tempchar,"0");
	else {
		if (m_orgdata.Substr(17, 2) == NULL)
		{
			strcpy(tempchar, myrowstr.Substr(17, 2));
		}
		else {
			strcpy(tempchar, m_orgdata.Substr(17, 2));
			settozero.Add(tempchar);
			checkint = -1;
			checkint = settozero.GetAsUInt32();
			if (pretoshowserverdate && checkint == 0)
			{
				strcpy(tempchar, myrowstr.Substr(17, 2));
			}
		}
			
	}
	temp.Add(tempchar);
	m_datetime.wSecond=temp.GetAsUInt32();
	m_datetime.wMilliseconds=0;


	/* to check if the user has entered a date containg year as zero or month as zero or day as zero*/

	if(iszerodateflag==-1){
	EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
	}

	MonthCal_SetCurSel(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1), &m_datetime);
	if(!m_isDate)
	{
		DateTime_SetFormat(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1), L"HH:mm:ss");
		DateTime_SetSystemtime(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),GDT_VALID, &m_datetime);
	}
}


//Position the control
void
CalendarCtrl::PositionWindow(RECT* rect)
{
   
    RECT        temprect = {0};
	RECT prect = {0} ;
    wyInt32     width, height, hmargin = 0, vmargin = 0;
	RECT calrect,timerect,okrect,cancelrect,nowrect;
	GetClientRect(m_hwndparent, &prect);
	prect.right = prect.right - prect.left;
	prect.bottom = prect.bottom - prect.top;
	prect.left = 0;
	prect.top = 0;
    
    //Get all element handles
	GetClientRect(m_hwnd, &temprect);
	MonthCal_GetMinReqRect(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1),&calrect);  
	GetWindowRect(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),&timerect);
	GetWindowRect(GetDlgItem(m_hwnd,IDOK),&okrect);
	GetWindowRect(GetDlgItem(m_hwnd,IDCURRDATE),&nowrect);
	GetWindowRect(GetDlgItem(m_hwnd,IDCANCEL),&cancelrect);

    //calculate and modify the width and height based on the availabe space to best fit the window
	
	width = calrect.right - calrect.left;
	height = calrect.bottom - calrect.top + (okrect.bottom-okrect.top);
	hmargin = prect.right - rect->right;
	vmargin = prect.bottom - rect->top;
    
    if(height > vmargin)
	{
		if(rect->bottom - height > 0)
		{
			temprect.top = rect->bottom - height;
		}
		else if(rect->bottom + height/2 <= prect.bottom && rect->bottom - height/2 >= prect.top)
		{
			temprect.top = rect->bottom - height/2;
		}

		else
		{
			temprect.top = prect.top;
		}
	}
	else
	{
		temprect.top = rect->top;
	}
	
	
	if(width > hmargin)
    {
		if(rect->left - width > 0)
		{
			temprect.left = rect->left - width;
		}
		else
		{
			temprect.left = rect->left;
			if(temprect.top == rect->top)
			{
				temprect.top = rect->bottom;
			}
			else
			{
				if((rect->top - height)>0)
				{
					temprect.top = rect->top - height;
				}
				else
				{
					temprect.left = rect->left;
					if(temprect.top == rect->top)
					{
						temprect.top = rect->bottom;
					}
				}
			}
		}
	}
	else
		temprect.left = rect->right;

	//Postion all Controls

	MoveWindow(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1),
		-1,
		-1,
		width,
		calrect.bottom - calrect.top,
		TRUE);

	MoveWindow(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),
		0,
		(calrect.bottom - calrect.top)-1,
		width - (3*(okrect.right-okrect.left)+10),
		okrect.bottom-okrect.top+1,
		TRUE);

	MoveWindow(GetDlgItem(m_hwnd,IDCURRDATE),
		width - (3*(okrect.right-okrect.left)+10),
		(calrect.bottom - calrect.top)-2,
		(okrect.right-okrect.left)+1,
		okrect.bottom-okrect.top+1,
		TRUE);

	MoveWindow(GetDlgItem(m_hwnd,IDOK),
		width - (2*(okrect.right-okrect.left)+10),
		(calrect.bottom - calrect.top)-2,
		(okrect.right-okrect.left),
		okrect.bottom-okrect.top+1,
		TRUE);

	MoveWindow(GetDlgItem(m_hwnd,IDCANCEL),
		width - (okrect.right-okrect.left)-10,
		(calrect.bottom - calrect.top)-2,
		(okrect.right-okrect.left)+9,
		okrect.bottom-okrect.top+1,
		TRUE);

	//Position the window
#ifndef COMMUNITY
	if(m_isForm)
		MoveWindow(m_hwnd, temprect.left+4, temprect.top, width, height, TRUE);
	else
#endif
		MoveWindow(m_hwnd, temprect.left, temprect.top, width, height, TRUE);
	
}

// handle OK Click
void 
CalendarCtrl::OnClickOk()
{
    wyInt32 row, col;
     

    ConvertCtrlValues();
    SendMessage(m_hwnd,UM_ENDDIALOG,0,0);
	CustomGrid_ApplyChanges(m_hwndparent);

    row = CustomGrid_GetCurSelRow(m_dv->m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_dv->m_hwndgrid);
    m_dv->m_gridwndproc(m_dv->m_hwndgrid, GVN_BEGINLABELEDIT, row, col);
	CustomGrid_SetText(m_dv->m_hwndgrid, row, col, m_date.GetString());
	m_dv->m_gridwndproc(m_dv->m_hwndgrid, GVN_ENDLABELEDIT, MAKELONG(row, col), (LPARAM)m_date.GetString());
	
	delete this;
}


// handle NOW Click
void 
CalendarCtrl::OnClickNow(DataView *m_dv)
{
	SYSTEMTIME seltime;
	wyBool pretoshowserverdate;

	//Get preference setting to show server date or system date
	pretoshowserverdate = GetPreferenceForNowButton();

	if (pretoshowserverdate && m_dv != NULL) {
		
		MYSQL			*mysql;
		Tunnel			*tunnel;
		wyString	    query("select NOW()");
		MYSQL_RES	    *res;
		MYSQL_ROW	    row;
		wyString		myrowstr;
		wyInt32			qret;

		tunnel = m_dv->m_data->m_pmdi->m_tunnel;
		mysql = m_dv->m_data->m_pmdi->m_mysql;
		qret = HandleMySQLRealQuery(tunnel, mysql, query.GetString(), query.GetLength(), wyTrue, wyTrue, wyTrue);

		res = tunnel->mysql_store_result(mysql, wyTrue);
		/* we specifically ignore empty queries */
		if (res == NULL && mysql->affected_rows == -1)
		{
			return;
		}
		row = tunnel->mysql_fetch_row(res);
		myrowstr.SetAs(row[0]);

		seltime.wYear = atoi(myrowstr.Substr(0, 4));
		seltime.wMonth = atoi(myrowstr.Substr(5, 2));
		seltime.wDay = atoi(myrowstr.Substr(8, 2));
		seltime.wDayOfWeek = 0;
		seltime.wHour = atoi(myrowstr.Substr(11, 2));
		seltime.wMinute = atoi(myrowstr.Substr(14, 2));
		seltime.wSecond = atoi(myrowstr.Substr(17, 2));
		seltime.wMilliseconds = 0;

		// free the result space
		m_dv->m_data->m_pmdi->m_tunnel->mysql_free_result(res);

	}
	else
	{
		GetLocalTime(&seltime);
	}
	
	EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
	MonthCal_SetCurSel(GetDlgItem(m_hwnd,IDC_MONTHCALENDAR1), &seltime);
	DateTime_SetFormat(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1), L"HH:mm:ss");
	DateTime_SetSystemtime(GetDlgItem(m_hwnd,IDC_DATETIMEPICKER1),GDT_VALID, &seltime);
	
}


// handle Cancel Click
void 
CalendarCtrl::OnClickCancel()
{
	SendMessage(m_hwnd,UM_ENDDIALOG,0,0);

	if(!m_isForm)
    {
		CustomGrid_CancelChanges(m_hwndparent);
    }

	delete this;
}

// Get preference set by user
wyBool
CalendarCtrl::GetPreferenceForNowButton()
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH + 1] = { 0 };
	wyInt32     ret = 0;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "ShowServerDate", 1, dirstr.GetString());

	return(ret) ? (wyTrue) : (wyFalse);
}

// handle LButtonDown on parent (formview)
void 
CalendarCtrl::OnLButtonDown(LPARAM lParam)
{
#ifndef COMMUNITY
	int xPos, yPos;
	RECT redit;
#endif
	if(!m_isForm)
	{
		SendMessage(m_hwnd,UM_ENDDIALOG,0,0);
	}
#ifndef COMMUNITY
	else
	{	
		xPos = GET_X_LPARAM(lParam); 
		yPos = GET_Y_LPARAM(lParam); 
		
		//Edit Box Rectangle
		redit = m_edit.get_location();
		if(!(xPos > redit.left && xPos < redit.right && yPos > redit.top && yPos < redit.bottom) && 
			!(xPos > m_rectCell.left && xPos < m_rectCell.right && yPos > m_rectCell.top && yPos < m_rectCell.bottom))
			SendMessage(m_hwnd, UM_ENDDIALOG,0,0);
	}
#endif
}

#ifndef COMMUNITY
///Public function to invoke the calendar popup from formview
HWND DisplayCalendarForm(htmlayout::dom::element hwndedit, FormView *pfv , wyBool isDate)
{
	CalendarCtrl *c = new CalendarCtrl();
	return c->CreateForm(hwndedit, pfv, isDate);
}
#endif

HWND DisplayCalendarGrid(HWND hwndParent, DataView *dv, wyBool isDate, wyBool isResult)
{
	CalendarCtrl *c = new CalendarCtrl();	
	return c->CreateGrid(hwndParent, dv, isDate, isResult);
}
