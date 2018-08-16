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

#include <shlobj.h>

#include "FrameWindowHelper.h"
#include "Global.h"
#include "ExportMultiFormat.h"
#include "FileHelper.h"

extern	PGLOBALS		pGlobals;

// Function implent general file opening fucntions.
// It is used to get names of the for various file handling things.

wyBool  
InitFile(HWND hwnd, wyWChar* filename, wyInt32 filter, wyInt32 bufsize, wyUInt32 flags)
{
	OPENFILENAME    openfilename;

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = hwnd;
	openfilename.hInstance         = pGlobals->m_hinstance;
	openfilename.lpstrFilter       =(filter == CSVINDEX)?(CSV):
									(filter == XMLINDEX)?(XML): 	
									(filter == SCHEMAXMLINDEX)?(SCHEMAXML): 
									(filter == BMPINDEX)?(BMPFILE): 
									(filter == HTMINDEX)?(HTML):
									(filter == SQLINDEX)?(SQL):
									(filter == LOGINDEX)?(LOG):
                                    (filter == QUERYXMLINDEX)?(QUERYXML):
                                    (filter == CONMANINDEX)?(CONMANFILE):
									(filter == SESSIONINDEX)?(SESSIONFILE):
									(filter == TEXTINDEX)?(TEXTFILE):(NULL);
                                    
	openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = filename;
	openfilename.nMaxFile          = bufsize;
	openfilename.lpstrTitle        = _(L"Save As");
	openfilename.lpstrDefExt       =(filter == CSVINDEX)?(L"csv"):
									(filter == XMLINDEX)?(L"xml"): 	
									(filter == SCHEMAXMLINDEX)?(L"schemaxml"):
									(filter == BMPINDEX)?(L"bmp"):
									(filter == HTMINDEX)?(L"html"):
									(filter == SQLINDEX)?(L"sql"):
									(filter == LOGINDEX)?(L"log"):
                                    (filter == QUERYXMLINDEX)?(L"queryxml"):
                                    (filter == CONMANINDEX)?(L"sycs"):
									(filter == SESSIONINDEX)?(L"ysav"):
									(filter == TEXTINDEX)?(L"txt"):(NULL);
                                    
	openfilename.Flags             = flags;

	if(GetSaveFileName(&openfilename))
		return wyTrue;
	else
	   return wyFalse;

}

wyBool 
InitOpenFile(HWND hwnd, wyString &filename, wyInt32 filter, wyInt32 bufsize, wyBool ismainwnd)
{
    wyWChar     file[MAX_PATH] = {0};
    wyBool      ret;

    ret = InitOpenFile(hwnd, file, filter, bufsize);
    filename.SetAs(file);

    return ret;
}

wyBool 
InitOpenFile(HWND hwnd, wyWChar *filename, wyInt32 filter, wyInt32 bufsize, wyBool ismainwnd)
{
	OPENFILENAME openfilename;

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = hwnd;
	openfilename.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
	openfilename.lpstrFilter       =(filter == CSVINDEX)?(CSV):
									(filter == XMLINDEX)?(XML): 	
									(filter == SCHEMAXMLINDEX)?(SCHEMAXML):
									(filter == HTMINDEX)?(HTML):
									(filter == SQLINDEX)?(SQL):
									(filter == ZIPINDEX)?(ZIP):
									(filter == TEXTINDEX)?(TEXTFILE):
									(filter == ACCESSINDEX)?(ACCESSFILE):
                                    (filter == QUERYXMLINDEX)?(QUERYXML):
									(filter == SQLYOGFILEINDEX)?(SQLYOG_FILES):
									(filter == EXCELINDEX)?(EXCELFILE):
									(filter == SESSIONINDEX)?(SESSIONFILE):
                                    (filter == PPKINDEX)?(PPKFILE):(NULL);
	
	openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = filename;
	openfilename.nMaxFile          = bufsize;
	openfilename.lpstrTitle        = _(L"Open File");
	openfilename.lpstrDefExt       =(filter == CSVINDEX)?(L"csv"):
									(filter == XMLINDEX)?(L"xml"): 	
									(filter == SCHEMAXMLINDEX)?(L"schemaxml"): 
									(filter == HTMINDEX)?(L"html"):
									(filter == SQLINDEX)?(L"sql"):
									(filter == ZIPINDEX)?(L"zip"):
									(filter == TEXTINDEX)?(L"txt"):
									(filter == ACCESSINDEX)?(L"mdb"):
                                    (filter == QUERYXMLINDEX)?(L"queryxml"):
									(filter == SESSIONINDEX)?(L"ysav"):
									(filter == EXCELINDEX)?(L"xls"):(NULL);

	if(ismainwnd == wyTrue)
		openfilename.Flags             = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	else
		openfilename.Flags             = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	
	if(GetOpenFileName(&openfilename))
		return wyTrue;

	else
        return wyFalse;
}

wyBool 
GetDirectoryFromUser(HWND hwnd, wyWChar *dirname, wyWChar *title)
{
	LPMALLOC		lpmalloc;
	BROWSEINFO		brwinf;
	LPITEMIDLIST	lpidl;

	memset(&brwinf, 0, sizeof(brwinf));

	if(SHGetMalloc(&lpmalloc)!= NOERROR)
		return wyFalse;

	brwinf.hwndOwner		= hwnd;
	brwinf.pidlRoot			= NULL;
	brwinf.pszDisplayName	= dirname;
	brwinf.lpszTitle		= title;
	brwinf.ulFlags			= BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE;

    //Displays a dialog box enabling the user to select a Shell folder
	lpidl = SHBrowseForFolder(&brwinf);

	if(!lpidl)
	{
		lpmalloc->Release();
		return wyFalse;
	}

    //Converts an item identifier list to a file system path
	SHGetPathFromIDList(lpidl, dirname);

	lpmalloc->Free(lpidl);
	lpmalloc->Release();

	return wyTrue;
}
