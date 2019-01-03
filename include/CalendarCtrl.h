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


#include "FrameWindowHelper.h"
#include "htmlayout.h"

#ifndef _CALENDARCTRL_H_
#define _CALENDARCTRL_H_


class CCustGrid;
#ifndef COMMUNITY
class FormView;
#endif
class DataViewBase;
class DataView;

// User defined messages used in various procedures.

#define UM_ENDDIALOG			WM_USER+207

//Calendar Control
class CalendarCtrl
{

	public:
	/// Default constructor.
    /**
    Initializes the member variables with the default values.
    */
	CalendarCtrl();

	 /// Default destructor
    /**
    Free up all the allocated resources
    */
	~CalendarCtrl();

	/// The main dialog proc for the CalendarControl Box.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
		static INT_PTR	CALLBACK CalendarCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		/// The subclassed dialog proc for the Calendar.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
		static wyInt32	CALLBACK CalendarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		/// The subclassed dialog proc for the timePicker.
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 1 on success else 0
	*/
		static wyInt32	CALLBACK TimeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Sets the parameters of the calendar control in grid view.
	/*
	@param hwndParent	: IN Parent window HANDLE
	@param olddat		: IN Current cell data
	@param dvb			: IN pointer to dataviewbase
	@param isDate		: IN wyTrue if the field is DATE, wyFalse if the field is DATETIME or TIMESTAMP
	@param isResult		: IN wyTrue  if it is from the result tab
							 wyFalse if it is from table data tab
	@returns 0 on success, 1 otherwise
	*/
		HWND	CreateGrid(HWND hwndParent, wyWChar* olddat, DataViewBase *dvb, wyBool isDate, wyBool isResult);

		
	/// Sets the parameters of the calendar control in grid view.
	/*
	@param hwndParent	: IN Parent window HANDLE
	@param olddat		: IN Current cell data
	@param dvb			: IN pointer to dataviewbase
	@param isDate		: IN wyTrue if the field is DATE, wyFalse if the field is DATETIME or TIMESTAMP
	@param isResult		: IN wyTrue  if it is from the result tab
							 wyFalse if it is from table data tab
	@returns 0 on success, 1 otherwise
	*/
		HWND	CreateGrid(HWND hwndParent, DataView *dv, wyBool isDate, wyBool isResult);

	/// Sets the parameters of the calendar control in form view.
	/*
	@param hwndedit		: IN Corresponding edit control element
	@param pfv			: IN pointer to the form
	@param isDate		: IN wyTrue if the field is DATE, wyFalse if the field is DATETIME or TIMESTAMP
	@returns CalendarCtrl Handle
	*/
#ifndef COMMUNITY
		HWND	CreateForm(htmlayout::dom::element hwndedit, FormView *pfv, wyBool isDate);
#endif
	/// Creates the dialog box and subclasses the monthcalendarcontrol and datetimepicker
	/*
	@returns CalendarCtrl Handle
	*/

		HWND	Create();


	// Initializes the Calendar control values
		void	InitCalendarValues();

	//Positions the calendar control according to the space available
		void	PositionWindow(RECT* prect);
	
	//Gets the values from the control and converts it into a string 
		void	ConvertCtrlValues();
	
	//Handles click of the OK button
		void	OnClickOk();

	//Handles click of the OK button
		void	OnClickNow(DataView *m_dv);

	//Get preference to show server date and time when click on NOW button
		wyBool GetPreferenceForNowButton();

	//Handles click of the Cancel button
		void	OnClickCancel();

	//Handles WM_LBUTTONDOWN of the parent window
		void	OnLButtonDown(LPARAM lParam);

	//Handle to the dialog box
		HWND						m_hwnd;

	//Handle to the parent window
		HWND						m_hwndparent;
#ifndef COMMUNITY
	//Button element in the formview
		htmlayout::dom::element		m_button;

	//Editbox element in the formview			
		htmlayout::dom::element		m_edit;			
#endif
	//Rectangle of the cell(grid) or button(form)
		RECT						m_rectCell;

	//Pointer to DataViewBase
		DataViewBase				*m_dvb;

		//Pointer to DataView
		DataView					*m_dv;

#ifndef COMMUNITY
	//Pointer to the form
		FormView					*m_pfv;
#endif
	//to set the control date;	
		SYSTEMTIME					m_datetime;

	//String to input the date in the edit control
		wyString					m_date;
			
	//Edit cell row and column
		wyInt32						m_row;
		
		wyInt32						m_col;

	//Original cell date
		wyString					m_orgdata;

	//True if it is formview	
		wyBool						m_isForm;

	//True if it DATE field	
		wyBool						m_isDate;

	//True if result tab	
		wyBool						m_isResult;

	//Original datetimepicker WNDPROC
		WNDPROC						m_timeproc;

	//Original monthacalendar WNDPROC
		WNDPROC						m_calproc;
			
};
#ifndef COMMUNITY
///Public function to invoke the calendar from the grid
HWND DisplayCalendarForm(htmlayout::dom::element hwndedit, FormView *pfv, wyBool isDate);
#endif

HWND DisplayCalendarGrid(HWND hwndParent, DataView *dv, wyBool isDate, wyBool isResult);

#endif