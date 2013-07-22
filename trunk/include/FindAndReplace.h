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
#include "Global.h"
#include "MDIWindow.h"
#include "Scintilla.h"

#ifndef _FINDREPLACE
#define _FINDREPLACE

#define FINISHED_ONECYCLE	_(L" Finished searching the document")
#define BEGINNING_REACHED	_(L" Beginning of the document is reached. Do you want to continue searching from the end?")
#define END_REACHED			_(L" End of the document is reached. Do you want to continue searching from the beginning?")
#define TEXT_NOTFOUND		_(L" Finished searching the document. The search item not found")

class FindAndReplace
{

public:

	/// FindAndReplace constructor and destructor
	/**
    */
	FindAndReplace(HWND hwnd);

	~FindAndReplace();

	/// Finds text in 
	/**
	@param hwnd         : IN Window Handler
	@param lParam		: IN Long message parameter
	@return void
	*/
	void				FindNextText(HWND hwnd, LPARAM lParam);

	/// The Function is called when the user presses the FIND NEXT button	
	/**
	@param lpfr			: IN/OUT FILEREPLACE class pointer
	@returns void
	*/
	void				OnFindNext(HWND hwnd, LPFINDREPLACE& lpfr);
	
	/// Finds text in a scintilla control one by one	
	/**
	@param hwnd                 : IN Window Handler
	@param findWhat             : IN Text to find
	@param reversedirection     : IN Reverse direction
	@param wholeword            : IN To check for the whole word or not
	@param matchcase            : IN To match case or not
	@returns 1 on success else 0
	*/
	wyInt32				FindNext(wyWChar * findWhat, wyInt32 reversedirection, wyInt32 wholeword, wyInt32 matchcase);

	///Finds text postion 
	/**
	@hwnd						:IN Window Handler
	@findwhatstr				:IN Text to find
	@param startposition		:IN Starting position
	@param endposition			:IN Ending position
	@reurn 1 on sucess else -1
	*/
	wyInt32				FindTextPostion(wyString * findwhatstr, wyInt32 flag, wyInt32 startposition, wyInt32 endposition);

	/// Function to show a yog_message if FIND string is not found.
	/**
	if the to find text is not found it will display dlg box
	with the message to find text is not found
	@param to find		: IN wyString text which is not found
	@returns void
    */
	void				NotFoundMsg(wyString tofind);

	/// Function Displayes a Message Box to find next if end or beginning of the document is reached
	/** @param IsBeginning	: true upward search false search from down
	*/
	wyInt32				FindNextconfirmationMessage(wyBool  IsBeginning);	

	/// Function to implement FIND and FIND REPLACE & REPLACE ALL functionality.
	/**
	This function is called whenever there is a message
	from the find replace dialog box.
	@param lParam		: IN Long message parameter
	@returns void
	*/
	wyBool				FindReplace(HWND hwnd, LPARAM lParam, wyBool val = wyFalse);

	// Function to implement the find replace mechanism in the software.
	
	/// Replaces all instances of replacewhat with replacewith in the editor
	/**
	@param replacewhat	: OUT String to replace
	@param repalcewith	: IN String to replace with
	@param wholeworld	: IN Whole word ?
	@param matchcase	: IN Match case ?
	*/
	wyInt32				ReplaceAll(wyString& replacewhat, wyString& replacewith, wyUInt32 wholeworld, wyUInt32 matchcase);

	/// Replaces one instances of replacewhat with replacewith in the editor
	/**
	@param posfind		: IN Position last match found
	@param endposition	: IN/OUT End position after changing the string
	@param replacewhat	: OUT String to replace
	@param repalcewith	: IN  String to replace with
	@param wholeworld	: IN Whole word ?
	@param matchcase	: IN Match case ?
	*/
	wyInt32             Replace(wyInt32 posfind, wyInt32 endposition, wyString &replacewhat, wyString &replacewith, wyUInt32 wholeworld, wyUInt32 matchcase);
	
	/// Things to do before the dialog ends
	/**
	@param lpfr			: IN/OUT FILEREPLACE class pointer
	@returns void
    */
	VOID                OnEndDlg(LPFINDREPLACE& lpfr);

		
	/// The Function is called when the user presses the REPLACE button	
	/**
	@param lpfr			: IN/OUT FILEREPLACE class pointer
	@returns void
    */
	VOID                OnReplaceButton(LPFINDREPLACE& lpfr);

	/// The Function is called when the user presses the REPLACE ALL button	
	/**
	@param lpfr			: IN/OUT FILEREPLACE class pointer
	@returns void
    */
	VOID                OnReplaceAll(LPFINDREPLACE& lpfr);

	///	Find and replace the text once and then stops for user to respond 
	/**
	@param replacewhat	: IN Text to repalce
	@param replacewith	: IN Text to replace with
	@returns void
    */
	VOID				ReplaceOnce(wyWChar *replacewhat, wyWChar *replacewith);

	/// Saves the selection from psrc to pdest
	/**
	@param pdest		: OUT  Destination
	@param psrc			: IN  Source
	@returns void
	*/
	VOID				CopyFindData(FINDREPLACE * pdest, FINDREPLACE * psrc);

	/// Gets the starting and the ending position of a selected text
	/**
	@returns the range of selection
	*/
	CharacterRange	    GetSelection();

    ///Subclassed window procedure for Find/Replace dialog
    /**
    @param HWND                 : IN handle to the window
    @param message              : IN message to be processed
    @param wparam               : IN message parameter
    @param lparam               : IN message parameter
    @returns value after processing
    */
    static LRESULT	CALLBACK    FindWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    wyInt32         ShowMessage(wyWChar* message, wyInt32 flag);

	/// Flag raised if text is found in FIND REPLACE execution
	wyInt32             m_havefound;

	/// String to replace last found 
	FINDREPLACE			m_lastfind;

	//handle fro scintilla editor
	HWND				m_hwndedit;

	//first found position
	wyInt32				m_firstfound;

	//count
	wyInt32				m_count; 

	//if one cycle is completed or not
	wyBool				m_cyclecompleted;

	//Starting positon 
	wyInt32				m_startpos;

	//found or not
	wyBool				m_found;
	wyBool				m_flag;
	
	//search text
	wyString			m_findwhattext;

    wyInt32             m_findflag;
};

#endif
