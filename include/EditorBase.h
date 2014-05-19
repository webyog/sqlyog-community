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


#ifndef __EDITORBASE__
#define __EDITORBASE__

#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "CustTab.h"
#include "TabEditor.h"
#include "TabModule.h"
#include "Scintilla.h"
#include "FindAndReplace.h"


#define ISPRINTABLE(X)	((X>=48 && X<=57) || (X>=65 && X<=90) || (X>=186 && X<=192) || (X>=219 && X<=222) || (X==32))

/// Global Pointers
class MDIWindow;
class TabEditor;

/// Starts execution of query
/**
@param wnd			: IN/OUT Query window pointer
@param opt			: IN Execution options
@returns window HANDLE
*/
HWND	StartExecute(MDIWindow * wnd, EXECUTEOPTION opt);

/// Ends execution of query
/**
@param wnd			: IN/OUT Query window pointer
@param opt			: IN Execution options
@returns void
*/
VOID	EndExecute(MDIWindow * wnd, HWND hwnd, EXECUTEOPTION opt);

/// Handles options during execution of query
/**
@param wnd			: IN/OUT Query window pointer
@param opt			: IN Execution options
@returns void
*/
VOID    OnExecuteOptn(EXECUTEOPTION& opt, wyBool isstart);



class EditorBase
{

public:

	/// Editorbase constructor and destructor
	/**
    */
	EditorBase(HWND hwnd);
	virtual ~EditorBase();
    
	/// Sets the color for the editor base .
	/**
	@returns void
	*/
	VOID				SetColor();
	
	/// Changes the selected text to Uppercase
	/**
	@returns void
	*/
	VOID			    MakeSelUppercase();
	
	/// Changes the selected text to Lowercase
	/**
	@returns void
	*/
	VOID				MakeSelLowercase();
	
	/// The function makes selected text into comment.
    /**
	This function makes the whole line into comment
	even if half line is selected.
	@returns void
	*/
	VOID				CommentSel(wyBool isuncomment = wyFalse);
		
	/// This initializes m_isadvedit to wyTrue if it is advEdit
	/**
	@param val		: Enable/Disable advance editor
	@returns void
	*/
	VOID				SetAdvancedEditor(wyBool val);
	
	/// Function to display context menu of the edit box.
	/**
	@param				: IN Long message parameter
	@returns void
    */
	wyInt32				OnContextMenuHelper(LPARAM lParam);

	///Function to handle LeftButton Click
	/**
	@param wnd			: IN/OUT Query window pointer
	@param hwnd			: IN Window HANDLE
	@returns void
    */
	VOID				OnLButtonUp(MDIWindow *wnd,HWND hwnd);
	
	// Function to resize the edit box.
	/**
	@returns void
    */
	VOID				Resize();
	
	
    /// The following three function adds error message, result message or non result message
    /// in the buffer which is passed as parameter.
	/**
	@param tunnel			: IN Tunnel	pointer
	@param mysql			: IN Pointer to mysql pointer
	@param errorormsg		: OUT Error message 
	@param time taken		: IN Time taken
	@returns void
    */
	VOID				AddErrorMsg(Tunnel * tunnel, PMYSQL mysql, wyString& errorormsg, wyUInt32 timetaken);
	
	/// Adds non result message
	/**
	@param tunnel			: IN Tunnel	pointer
	@param mysql			: IN Pointer to mysql pointer
	@param errorormsg		: OUT Error message 
	@param timetaken		: IN Time taken
	@returns void
    */
	VOID				AddNonResultMsg(Tunnel * tunnel, PMYSQL mysql, wyString& errorormsg, wyUInt32 timetaken);
	

	/// Add the result message
	/**
	@param tunnel			: IN Tunnel	pointer
	@param mysql			: IN Pointer to mysql pointer
	@param errorormsg		: OUT Error message 
	@param timetaken		: IN Time taken
	@returns void
    */
	VOID				AddResultMsg(Tunnel * tunnel, MYSQL_RES* mysql, wyString& errorormsg, wyUInt32 timetaken);
	

	/// This function whenever the user wants to paste data. Basically before pasting we remove all the formatting data in the text so that 
	/// the text is inserted as only text. It is required because richedit copies all the data about font.
	/// So if a user is selecting query form diff source the formatting of the text becomes bad.
	/**
	@returns void
    */
	VOID				PasteData();
	

	/// Command handler for editor base
	/**
	@param hwnd				: IN Window HANDLE
	@param pCEditorBase		: IN/OUT Pointer to Editor Base class
	@param wparam			: IN Unsigned message pointer
	*/
	VOID                OnWmCommand(HWND hwnd, EditorBase* pCEditorBase, WPARAM wParam);
	
	/// Displays the result window
	/**
	@returns wyTrue
    */
	wyBool				ShowResultWindow();
	
	//This checking to identify whether they are 
	//comment or not
	/**
	@param hwndeditor		: HANDLE to editor window
	@returns wyTrue if comment else wyFalse
    */
	wyBool				IsComment(HWND hwndeditor, wyBool bfunc = wyFalse, wyInt32 pos = -1);
	
	/// Selects the first table to edit
	/**
	@returns wyTrue
    */
	wyBool				SelectFirstTableToEdit(wyBool isedit);
		
	/// Displays the tool tip message
	/**
	@param wnd			: IN MDIWindow object pointer
    */
	wyBool				ShowToolTip(MDIWindow	*wnd);
	
	/// Determines whether advance editor
	/**
	@returns wyTrue if advance editor else wyFalse
    */
	wyBool				GetAdvancedEditor();
	
	/// Sets the parent window pointer
	/**
	@param parentptr	: IN EditorTab object pointer
    */
	wyBool				SetParentPtr(TabEditor *  parentptr);
	
	/// Function allocates new buffer space and copies the next text into it and returns the address of the buffer.
	/// Before returning it changes all \r\n into \n coz when we lex color we get character poition with
	/// respect to only one new line feed. I don't know but this is how it works.
	/**
	@returns pointer to buffer allocated for query
    */
	void				GetCompleteText(wyString &query);
	void				GetCompleteTextByPost(wyString &query, MDIWindow *wnd);
	/// Allocates buffer for query text
	/**
	@returns pointer to allocated buffer
    */
	wyChar*				GetBufferForQueryText();
	
	
	/// Gets the window HANDLE
    /**
	@returns window handle
    */
	virtual HWND		GetHWND () { return (m_hwnd); };
	
	
	
	/// Function to execute the current query
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	virtual wyBool		ExecuteCurrentQuery(wyInt32 * stop, wyBool isedit = wyFalse){return wyFalse;};
	

    virtual wyBool      ExecuteExplainQuery(wyInt32 * stop, wyBool isExtended){return wyFalse;};
	
	/// Function to execute the all the query
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	/**
    */
	virtual wyBool		ExecuteAllQuery(wyInt32 * stop) { return wyFalse; };
	
	/// Function to execute the selected query
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	/**
    */
	virtual wyBool		ExecuteSelQuery(wyInt32 * stop) { return wyFalse; };

    /// Function to handle after query execution finish
	/**
	@param stop			: IN Stop condition
	@returns wyFalse
    */
	virtual wyBool		HandleQueryExecFinish(wyInt32 * stop, WPARAM wparam) {return wyFalse;};

	/// Window Procedure
    WNDPROC				wpOrigProc;
	
	/// Gets back parent pointer
	/**
	@returns Pointer to parent EditorTab 
    */
	//// Gets back parent pointer
	/**
	@returnes pointer to 'TabEditor'
	*/
	TabEditor *		GetParentPtr();

	/// Handles the keyup event of the window procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	*/
    wyInt32             OnWMKeyUp(HWND hwnd, WPARAM wparam);
	
	/// Handles the key-down event of the window procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param wnd          : IN con. window pointer
	@param ebase        : IN EditorBase pointer
	*/
	wyInt32				OnWMChar(HWND hwnd, WPARAM wparam, MDIWindow *wnd, EditorBase *ebase);

    ///this sets up all XPM values.
	/**
	@param hwndedit: IN HNADLE to editor window
	*/
    void                SetScintillaValues(HWND hwndedit);

	/// Callback procedure for editor base
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns 0
	*/
	static		LRESULT		CALLBACK	WndProc(HWND hwnd, UINT message, WPARAM pWPARAM, LPARAM pLPARAM);

	/// Destroys  all child windows
	/**
	@returns void
	*/
	void		DestroyWindows();
	
    /// Creates all the child windows 
    /**
    @param wnd : IN MDIWindow pointer
    @param hwnd : IN parent window handle
    @returns handle to the editor
    */
    HWND        CreateEditor(MDIWindow * wnd, HWND hwnd);

    // Function to implement context menu of the edit box.
    /**
    @param lparam       : IN Long message parameter
	@returns void
    */
	wyInt32				OnContextMenu(LPARAM lparam);

    ///sets up font
	/**
	@returns void
	*/
	void        SetFont();

    ///This will invoke the Thread to execute query
	/**
	@param  query :IN Query string
	@param  stop : IN Stop condition
	@param  wnd : IN MDI Window pointer
	@param  curline : IN Currentline
	@returns 1 on success
	*/
	wyInt32     ExecuteQueryThread(wyString query, wyInt32 *stop, MDIWindow * wnd, wyInt32& curline, wyBool isanalyze = wyTrue);
	
	// Function to set the 'TabEditor' pointer
	VOID	SetTabEditorptr(TabEditor *te);

	///It will set auto indentation in editor
	/**
	@param hwnd		: IN Editor handle
	@param wparam	: IN Unsigned message pointer
	@returnd wyTrue if success else wyFalse
	*/
	wyBool		SetAutoIndentation(HWND hwnd, WPARAM wparam);

	/// Field name
	static wyString		m_fields;
	
	/// File name 	
	wyString			m_filename;
	
	/// Advance editor ?
	wyBool				m_isadvedit;	

	/// Non character keys 
	wyBool				m_nonkey;
	
	/// Change in editor window  ?
	wyBool				m_edit;

	/// Is the content of editor window changed ?
	wyBool				m_save;

	//Flag set wyTrue when Save/Save As processed
	wyBool				m_isfilesave;

	/// Window HANDLE
	HWND				m_hwnd, m_hwndparent, m_hwndhelp;

	/// Font HANDLE
	HFONT				m_hfont;

	/// String to replace last found 
	FINDREPLACE			m_lastfind;
	
	/// HNADLE to item tree
	HTREEITEM			m_hitem;

    wyString            m_hitemname;

    /// Database Name
    wyString            m_dbname;

    /// Object Browser Node type;
    wyInt32             m_nodeimage;
	
	// pointer to 'TabEditor'
	TabEditor *		m_pctabeditor;

	//Object for FindAndRepalce class
	FindAndReplace  *m_findreplace;	

    ///Member variable says to discard any change notification from Scintilla
    wyBool              m_isdiscardchange;

    ///Stores style at and before the cursor position before invoking AC
    wyInt32             m_stylebeforeac;
    wyInt32             m_styleatac;

    ///Stores whether autocomplete is requested quoted identifier
    wyInt32             m_acwithquotedidentifier;
};

#endif
