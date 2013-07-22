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

#include "Global.h"
#include "TabTypes.h"
#include "FrameWindow.h"

extern	PGLOBALS		pGlobals;

TabTypes::TabTypes(HWND hwnd)
{
	m_hwnd			= NULL;
	m_hwndparent	= hwnd;
	m_pcetsplitter	= NULL;
	m_peditorbase	= NULL;
	m_pctabmgmt		= NULL;
	m_istextresult  = wyFalse;
	m_iseditwnd	= wyTrue;
	m_isresultwnd	= wyTrue;	
}

TabTypes::~TabTypes()
{
}

wyBool	
TabTypes::SetParentPtr(TabModule * parentptr)
{
	m_parentptr = parentptr;
	
	return wyTrue;
}

TabModule *	
TabTypes::GetParentPtr()
{
	return m_parentptr;
}

HWND
TabTypes::GetHwnd()
{ 
	return m_hwnd;
}

void 
TabTypes::SetBufferedDrawing(wyBool isset)
{

}

void        
TabTypes::OnGetChildWindows(wyInt32 tabcount, LPARAM lparam) 
{ 

}
