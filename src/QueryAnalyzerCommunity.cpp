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
#include "QueryAnalyzerCommunity.h"

QueryAnalyzerCommunity::QueryAnalyzerCommunity(HWND hwndparent, MDIWindow* wnd) : QueryAnalyzerBase(wnd, hwndparent)
{
	m_isdefaultpqatab = wyTrue;
}

QueryAnalyzerCommunity::~QueryAnalyzerCommunity()
{
}

wyBool
QueryAnalyzerCommunity::CreateControls(wyChar *query, wyInt32 index, wyBool istunnel, wyBool isdefaulttab, wyBool islimitadded)
{
	//MDIWindow	*wnd = NULL;
	wyString	htmlbuff;
	wyBool		isanyresult = wyTrue;

	m_isdefaultpqatab = wyTrue;
				
	m_querytext.SetAs(query);

	//VERIFY(wnd = GetActiveWin());

	if(isdefaulttab == wyFalse)
    {
		return wyFalse;
    }

	//Parent window for all controls in Analyzer tab
	CreateMainWindow();
	
	//Initiaize the HTML code and add styles;
	isanyresult = InitHtmlBuffer(istunnel, isdefaulttab);
	
	if(CreateHtmlEditor(&m_htmlformatstr) == wyFalse)
		return wyFalse;
		
	/*Create and insert 'Analyzer tab' to custom tab.
	It comes just after the 1st 'SELECT Result set' Result tab*/	
	CreateAnalyzerTab(index);
		
	return wyTrue;
}

wyBool
QueryAnalyzerCommunity::InitHtmlBuffer(wyBool istunnel, wyBool isdeftab)
{
	m_htmlformatstr.SetAs("<html><body>");
	m_htmlformatstr.Add("<style type=\"text/css\">");
		
	m_htmlformatstr.Add(CSS_WARNING);
	
	m_htmlformatstr.Add("</style>");		
	
		m_htmlformatstr.Add("<div class = \"tabcaptionstyle\">");
		m_htmlformatstr.AddSprintf("<b>%s</b></div", QA_ULTIMATE);
		m_htmlformatstr.Add("</body></html>");
			
	return wyTrue;
}
