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

# include "FrameWindowHelper.h"

class CommunityRibbon;

typedef struct commtyheader
{
	CommunityRibbon		*m_comribbon; //Class pointer
}COMTYHDR;

class CommunityRibbon
{
public:
	//Construct and destructor
	CommunityRibbon();
	~CommunityRibbon();

	///Creates the ribbon window
	/**
	@param hwndparent : IN parent window handle
	@return wyTrue on success else return wyFalse
	*/
	wyBool CreateRibbon(HWND hwndparent);

	/// window procedure for the community ribbon
	/**
	@param hwnd			: IN tab window handle
    @param wparam		: IN windowprocedure WPARAM
    @param lparam		: IN windowprocedure LPARAM
    @returns wyInt32 , always 0.
	*/
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    
	//Handles the communty tab header
	/**
	@return void
	*/
	void	HandleCommunityHeader();

	///Handes for setting community text
	/**
	@param : IN struct passed to this thread
	*/
	static	unsigned __stdcall		HandleChangeTabText(LPVOID param);

	///Enumerates the connection windows
	/**
	@param comhdr : IN struct pointer holds arguments paased to thrread function
	@return void
	*/
	VOID	EnumConWindows(COMTYHDR *comhdr);

	/// The callback function helps to apply changes  to all windows 
	/**
	@param hwnd			: IN frame window HANDLE
	@param lparam		: IN Long message parameter
	*/
	static	wyInt32 CALLBACK	EnumChildProc(HWND hwnd, LPARAM lParam);

	///Generates the random index of the String ID to be displayed in ribbon
	/**
	@return index
	*/
	static	wyInt32			GetRandomIndex();

	//Thread handle
	HANDLE		m_thread;

	// Handle to ribbon
	HWND		m_hwnd;

	//Font of the window
	HFONT		m_hfont;
	
	//procedure fo r
	WNDPROC		m_wndproc;
};

