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
#include "PreferenceBase.h"

#ifndef __PREFFREE__
#define	__PREFFREE__

class PreferencesCommunity:public PreferenceBase
{
public:	

    /// Apply changes to the file
    /**
    @returns void
    */
    void		Apply();

	///Restore Defaults for all the tabs
    /**
    @returns void
    */
	void		RestoreAllDefaults();

	/// Handle the WM_INITDIALOG on Autocomplete_pref dialog
	/**
	@param hwnd : IN Handle to tab
	@return void
	*/
	void        EnterprisePrefHandleWmInitDialog(HWND hwnd);

	/// Handle the WM_COMMAND on Autocomplete_pref dialog
	/**
	@param hwnd : IN Handle to tab
	@param wParam : IN WPARAM message parameter
	@return void
	*/
	void        EntPrefHandleWmCommand(HWND hwnd, WPARAM wParam);
    
	/// Handle the WM_INITDIALOG on Formatterpref dialog
	/**
	@param hwnd : IN Handle to tab
	@return void
	*/
	void        FormatterPrefHandleWmInitDialog(HWND hwnd);

	/// Handle the WM_COMMAND on Formatter_pref dialog
	/**
	@param hwnd : IN Handle to tab
	@param wParam : IN WPARAM message parameter
	@return void
	*/
	void        FormatterPrefHandleWmCommand(HWND hwnd, WPARAM wParam);

	/// Handle the WM_NOTIFY on Autocomplete dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
	void        ACPrefHandleWmNotify(HWND hwnd, LPARAM lparam);
	
	/// Handle the WM_NOTIFY on Formatterpref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        FormatterPrefHandleWmNotify(HWND hwnd, LPARAM lparam);
};

#endif