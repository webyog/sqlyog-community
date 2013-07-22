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

#ifndef _AUTO_COMPLETE_INTERFACE_H_
#define _AUTO_COMPLETE_INTERFACE_H_
#include "MDIWindow.h"
#include "DataType.h"

class CAutoComplete;

class AutoCompleteInterface
{
public:

    AutoCompleteInterface();
    ~AutoCompleteInterface();

	/// Initializes the auto complete
	/**
	@param wnd		: IN Pointer to MDI class object
	@returns wyTrue on success else wyFalse
	*/
    wyBool              HandlerInitAutoComplete(MDIWindow * wnd);
    
	/// Handler to stored objects
	/**
	@param wnd		: IN Pointer to MDI class object
	@param rebuild	: IN Rebuild ?
	returns void
	*/
    void	            HandlerStoreObjects(MDIWindow *wnd, wyBool brebuild);

	/// Initiates the tag building process
	/**
	@param wnd		: IN Pointer to MDI class object
	returns void
	*/
    void	            HandlerBuildTags(MDIWindow *wnd);

	/// Function is called when a key is pressed on the keyboard
	/**
	@param hwnd		: IN HANDLE to a window
	@param eb		: IN Pointer to editor base class object
	@param			: IN Unsigned message pointer
	@returns 1 on success else 0
	*/
    wyInt32             HandlerOnWMChar(HWND hwnd, EditorBase *eb, WPARAM wparam);

	/// Function is called when  down key is pressed on the keyboard
	/**
	@param hwnd		: IN HANDLE to a window
	@param eb		: IN Pointer to editor base class object
	@param			: IN Unsigned message pointer
	@returns 1 on success else 0
	*/
    wyInt32             HandlerOnWMKeyDown(HWND hwnd, EditorBase *eb, WPARAM wparam);

	/// Function is called when up key is pressed on the keyboard
	/**
	@param hwnd		: IN HANDLE to a window
	@param eb		: IN Pointer to editor base class object
	@param			: IN Unsigned message pointer
	@returns 1 on success else 0
	*/
    wyInt32             HandlerOnWMKeyUp(HWND hwnd, EditorBase *eb, WPARAM wparam);

	/// Adds messages to message queue
	/**
	@param wnd		: IN Pointer to MDI class object
	@param query	: IN Query string
	returns wyTrue if success, else wyFalse
	*/
    wyBool              HandlerAddToMessageQueue(MDIWindow*	wnd, wyChar *query);

    /// Processes messages in the message queue
	/**
	@param wnd		: IN Pointer to MDI class object
	@param query	: IN Query string
	returns void
	*/
    void              HandlerProcessMessageQueue(MDIWindow *wnd);

	/// Shows tool tips 
	/**
	@param wnd		: IN Pointer to MDI class object
	returns wyTrue if success, else wyFalse
	*/
    wyBool              HandlerShowToolTip(MDIWindow *wnd);

	/// Rebuilds keywords when autocomplete thread exits
	/**
	@param wnd		: IN Pointer to MDI class object
	@param lparam	: IN Long message pointer
	*/
    void                HandlerOnAutoThreadExit(MDIWindow *wnd, LPARAM lparam);

	//Update the 'AutocompleteTagbuilded' flag to '1' once the build tag is completed
    VOID				UpdateTagBuildFlag();
    
    wyBool                OnACNotification(WPARAM wparam, LPARAM lparam);
    
	/// Auto complete pointer
	CAutoComplete       *m_autocomplete;


};
#endif
