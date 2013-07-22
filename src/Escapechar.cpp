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
#include "FrameWindowHelper.h"
#include "CommonHelper.h"

// Constructor and destructor implementation of ESCAPECHAR class.
// the purpose of it is to read data from the file on creation
// and write it back when it is destroyed.

// as part of persistence feature.

ESCAPECHAR::ESCAPECHAR()
{
	m_isescaped = wyFalse;
	m_isfixed   = wyFalse;
	m_islineterminated = wyFalse;
	m_isnullreplaceby = wyFalse;
	m_isenclosed = wyFalse;
	m_isoptionally = wyFalse;
	m_isfieldsterm = wyFalse;
	m_iscolumns = wyFalse;
	m_isclipboard = wyFalse;
	
	memset(m_escape, 0, 10);
	memset(m_fescape, 0, 10);
	memset(m_lescape, 0, 10);
	memset(m_enclosed, 0, 10);
	memset(m_nullchar, 0, 10);
	
	m_towrite = wyTrue;

	//ReadFromFile(CSVSECTION);
}

ESCAPECHAR::~ESCAPECHAR()
{
	//if copy to clipboard is true then write to CSVESCAPING section else to EXPORTCSVESCAPING section
	if(m_isclipboard == wyTrue)
		WriteToFile(CSVSECTION);
	else
		WriteToFile(EXPORTCSVSECTION);
}

void
ESCAPECHAR::ReadFromFile(wyChar * escapesection)
{

	wyWChar		directory[MAX_PATH + 1], *lpfileport = 0;
	wyInt32		lastvalue;
	//wyChar		*mescape = {0}, *mfescape = {0}, *mlescape = {0}, *menclosed = {0};
	wyString	escapesectionstr, dirstr;
	wyString	escapestr, fescapestr, lescapestr, enclosedstr, nullcharstr;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	dirstr.SetAs(directory);
	escapesectionstr.SetAs(escapesection);

	wyIni::IniGetString(escapesectionstr.GetString(), "ESCAPEDBY", "\\\\", &escapestr, dirstr.GetString());
	wyIni::IniGetString(escapesectionstr.GetString(), "FESCAPE", "\\t", &fescapestr, dirstr.GetString());
	wyIni::IniGetString(escapesectionstr.GetString(), "LESCAPE", "\\n", &lescapestr, dirstr.GetString());
	wyIni::IniGetString(escapesectionstr.GetString(), "ENCLOSEDBY", "", &enclosedstr,  dirstr.GetString());
	wyIni::IniGetString(escapesectionstr.GetString(), "NULLREPLACEDBY", "\\N", &nullcharstr,  dirstr.GetString());
	
    
    strcpy(m_escape, escapestr.GetString());
    strcpy(m_fescape, fescapestr.GetString());
    strcpy(m_lescape, lescapestr.GetString());
    strcpy(m_enclosed, enclosedstr.GetString());
	strcpy(m_nullchar, nullcharstr.GetString());

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISESCAPED", 1, dirstr.GetString());

	if(lastvalue == 1)
		m_isescaped = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISFIXED", 0, dirstr.GetString());

	if(lastvalue == 1)
		m_isfixed = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "LINETERMINATED", 1, dirstr.GetString());

	if(lastvalue == 1)
		m_islineterminated = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISENCLOSED", 0, dirstr.GetString());

	if(lastvalue == 1)
		m_isenclosed = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISOPTIONALLY", 0, dirstr.GetString());

	if(lastvalue == 1)
		m_isoptionally = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "FIELDSTERM", 1, dirstr.GetString());

	if(lastvalue == 1)
		m_isfieldsterm = wyTrue;

	//include column names is enabled by default
	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISCOLUMNS", 1, dirstr.GetString());

	if(lastvalue == 1)
		m_iscolumns = wyTrue;

	lastvalue = wyIni::IniGetInt(escapesectionstr.GetString(), "ISNULLREPLACEBY", 1, dirstr.GetString());

	if(lastvalue == 1)
		m_isnullreplaceby = wyTrue;

	/*if(mescape)	
		free(mescape);
	
	if(mfescape)
		free(mfescape);
	
	if(mlescape)
		free(mlescape);
	
	if(menclosed)
		free(menclosed);*/
}

void
ESCAPECHAR::WriteToFile(wyChar * escapesection)
{	
	
	wyWChar	directory[MAX_PATH + 1];
	wyWChar	*lpfileport = 0;

	wyChar		newescape[10] = {0};
	wyChar		newfescape[10] = {0};
	wyChar		newlescape[10] = {0};
	wyChar		newenclosed[10] = {0};
	wyChar		newnullchar[10] = {0};
	
	wyString	escapesectionstr(escapesection);
	wyString	newescapestr; 
	wyString	newfescapestr;
	wyString	newlescapestr;
	wyString	newenclosedstr, dirstr;
	wyString	newnullreplacementcharstr;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini",MAX_PATH, directory, &lpfileport);

	if(m_towrite == wyFalse) 
    {
		RemoveEscapeChars(m_escape, newescape);
		RemoveEscapeChars(m_fescape, newfescape);
		RemoveEscapeChars(m_lescape, newlescape);
		RemoveEscapeChars(m_enclosed, newenclosed);
		RemoveEscapeChars(m_nullchar, newnullchar);
	} 
    else 
    {
		strcpy(newescape, m_escape);
		strcpy(newfescape, m_fescape);
		strcpy(newlescape, m_lescape);
		strcpy(newenclosed, m_enclosed);
		strcpy(newnullchar, m_nullchar);
	}
	
	newescapestr.SetAs(newescape);
	newfescapestr.SetAs(newfescape);
	newlescapestr.SetAs(newlescape);
	newenclosedstr.SetAs(newenclosed);
	newnullreplacementcharstr.SetAs(newnullchar);
	dirstr.SetAs(directory);

	wyIni::IniWriteString(escapesectionstr.GetString(), "ESCAPEDBY", newescapestr.GetString(), dirstr.GetString());
	wyIni::IniWriteString(escapesectionstr.GetString(), "FESCAPE", newfescapestr.GetString(), dirstr.GetString());
	wyIni::IniWriteString(escapesectionstr.GetString(), "LESCAPE", newlescapestr.GetString(), dirstr.GetString());
	wyIni::IniWriteString(escapesectionstr.GetString(), "ENCLOSEDBY", newenclosedstr.GetString(), dirstr.GetString());
	wyIni::IniWriteString(escapesectionstr.GetString(), "NULLREPLACEDBY", newnullreplacementcharstr.GetString(), dirstr.GetString());

	if(m_isescaped == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISESCAPED", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISESCAPED", 0, dirstr.GetString());
		
	if(m_isfixed == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISFIXED", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISFIXED", 0, dirstr.GetString());

	if(m_islineterminated == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "LINETERMINATED", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "LINETERMINATED", 0, dirstr.GetString());

	if(m_isenclosed == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISENCLOSED", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISENCLOSED", 0, dirstr.GetString());
	
	if(m_isoptionally == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISOPTIONALLY", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISOPTIONALLY", 0, dirstr.GetString());

	if(m_isfieldsterm == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "FIELDSTERM", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "FIELDSTERM", 0, dirstr.GetString());

	//to check include column names is enabled or disabled
	if(m_iscolumns == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISCOLUMNS", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISCOLUMNS", 0, dirstr.GetString());

	if(m_isnullreplaceby == wyTrue)
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISNULLREPLACEBY", 1, dirstr.GetString());
	else
		wyIni::IniWriteInt(escapesectionstr.GetString(), "ISNULLREPLACEBY", 0, dirstr.GetString());
}

void
ESCAPECHAR::RemoveEscapeChars(wyChar *in, wyChar *out)
{
	wyInt32	i = 0, j = 0;

	while(in[i] && j < 9)
	{
		switch(in[i])
		{
			case C_BACK_SLASH:
				out[j++] = '\\';
				//out[j++] = '\\';
				break;
			
			case '\r':
				out[j++] = '\\';
				out[j++] = 'r';
				break;
				
			case '\n':
				out[j++] = '\\';
				out[j++] = 'n';
				break;

			case '\t':
				out[j++] = '\\';
				out[j++] = 't';
				break;

			default:
				out[j++] = in[i];
				break;
		}

		i++;
		
	}
}