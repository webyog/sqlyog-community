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

#ifndef __ANNOUNCEMENTS_H__
#define __ANNOUNCEMENTS_H__

#include "Include.h"
#include "Global.h"
#include "htmlayout.h"

class Announcements;
class Announcements
{
public:
	//Constructor and destructor
	Announcements();
	~Announcements();

    //On exit the upgrade check thread
	/**
	@return void
	*/
	//VOID		ExitAnnouncementsThread();

	///Handkes the upgrade check
	/**
	@return void
	*/
	static BOOL                 HandleEvents(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params);
	wyBool						HandleAnnouncementsCheck(HWND hwnd);
	void						Resize(HWND hwnd, wyBool isstart = wyFalse);
	static LRESULT	CALLBACK	AnnounceWndMainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT	CALLBACK	AnnounceWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);	

	HWND		m_hwnd;		
	HWND		m_hwndHTML;	
	wyInt32     m_htmlw;
	wyInt32     m_htmlh;
	wyInt32     m_htmlx;
	wyInt32     m_htmly;
	wyInt32     m_annw;
	wyInt32     m_annh;
	wyInt32     m_annx;
	wyInt32     m_anny;

};

#endif