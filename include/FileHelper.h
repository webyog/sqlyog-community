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

#ifndef __FILEPROC__
#define __FILEPROC__

typedef struct hookprochelper
{
    wyString    m_encoding;
    HWND        m_combohandle;
}hookprocinfo;  

/// Gets the filename for saving
/** 
@param hwnd			: IN Window HANDLE
@param filename		: IN File name
@param filter		: IN Checks for the type of export
@param bufsize		: IN Size of the buffer
@returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool InitFile(HWND hwnd, wyWChar *filename, wyInt32 filter, wyInt32 bufsize, wyUInt32 flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT);

/// Gets filename to open
/**
@param hwnd			: IN Window HANDLE
@param filename		: OUT File name
@param filter		: IN Checks for the type of export
@param bufsize		: IN Size of the buffer
@returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool InitOpenFile(HWND hwnd, wyWChar *filename, wyInt32 filter, wyInt32 bufsize);

/// Overloaded function for the InitOpenFile for handling wyString data type
/**
@param hwnd			: IN Window HANDLE
@param filename		: OUT File name
@param filter		: IN Checks for the type of export
@param bufsize		: IN Size of the buffer
@returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool InitOpenFile(HWND hwnd, wyString &filename, wyInt32 filter, wyInt32 bufsize);


/// Displays a dialog box enabling the user to select a folder
/**
@param hwnd			: IN Window HANDLE
@param dirname		: IN Directory name
@param title		: IN Title for the dialog box
@returns wyBool, wyTrue if success, otherwise wyFalse
*/
wyBool GetDirectoryFromUser(HWND hwnd, wyWChar *dirname, wyWChar *title);

#endif