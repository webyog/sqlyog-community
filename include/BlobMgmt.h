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

#include <windows.h>
#include "FindAndReplace.h"

#ifndef __CBLOBINSERTUPDATE__
#define __CBLOBINSERTUPDATE__

/**
enum to specify what type of data is in the column.
*/
enum BLOBTYPE
{
	BLOB_TEXT=1,/**< BLOB_TEXT, Text value */  
	BLOB_IMAGE=2,/**< BLOB_IMAGE, Binary value */  
};

/*! \struct tagInsertUpdateBlob
    \brief holds data about a blob value in the insert update field
    \param wyChar*  m_data			holds the current data
    \param wyChar*  m_olddata		holds the original data while editing
    \param wyInt32  m_datasize		holds the current data size
    \param wyInt32  m_olddatasize	holds the original data size while editing
	\param wyBool   m_isnull		whether the data is null or not
	\param wyBool   m_isoldnull		whether the original data is null or not while editing
	\param wyBool   m_ischanged		whether the data is changed or not
*/
struct tagInsertUpdateBlob
{
	wyChar*		m_data;
	wyChar*		m_olddata;
	wyInt32     m_datasize;
	wyInt32     m_olddatasize;
	wyBool      m_isnull;
	wyBool      m_isoldnull;
	wyBool      m_ischanged;
	wyBool		m_isblob;
};

/*! Creates a type tagInsertUpdateBlob*/ 
typedef tagInsertUpdateBlob INSERTUPDATEBLOB, *PINSERTUPDATEBLOB;


class BlobMgmt
{
public:
    
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	BlobMgmt();

    /// Long message
    /**
    Free up all the allocated resources
    */
	~BlobMgmt();
	
    /// Creates the blob viewer windows
    /** 
    @param hwndparent		: IN parent window control
    @param pib				: IN PINSERTUPDATEBLOB structure
    @param edit				: IN editable or not, default value is editable
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				Create(HWND hwndparent, PINSERTUPDATEBLOB pib, wyBool edit = wyTrue);

	/// Function to implement the find and replace option
	/**
	@param  hwnd		: Handle to the dialog box
	@param isreplace	: IN Checks for replace option
	@returns wyTrue on success else wyFalse
	*/
	wyBool					FindOrReplace(HWND hwnd, wyBool isreplace);

	  /// Checks the data is binary or not
    /**
    @param data				: IN Data to check
    @param length			: IN data length
    @returns wyBool wyTrue if Binary, otherwise wyFalse
    */
	static wyBool				IsDataBinary(void * data, wyInt32 length);
    static wyBool				IsDataBinary2(void * data, wyInt32 length);

private:

    /// Common window procedure for the blob viewer
    /**
    @param hwnd				: Handle to the dialog box. 
    @param message			: Specifies the message. 
    @param wparam			: Specifies additional message-specific information. 
    @param lparam			: Specifies additional message-specific information. 

    @returns    1 or 0
    */
    static  INT_PTR	CALLBACK    DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Common window procedure for the blob viewer(Image)
    /**
    @param  hwnd			: Handle to the dialog box. 
    @param  message			: Specifies the message. 
    @param  wparam			: Specifies additional message-specific information. 
    @param  lparam			: Specifies additional message-specific information. 

    @returns 1 or 0
    */
	static	LRESULT	CALLBACK    ImageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Common window procedure for the blob viewer(Text)
    /**
    @param hwnd				: Handle to the dialog box. 
    @param message			: Specifies the message. 
    @param wparam			: Specifies additional message-specific information. 
    @param lparam			: Specifies additional message-specific information. 

    @returns 1 or 0
    */
	static	LRESULT	CALLBACK    TextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Initializes the blob viewer
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				InitDlgVales();	

	void				InitComboText();

    /// Handles the Wm_COMMAND on DlgProc
    /**
    @param wparam			: IN sent by the window procedure
    @param lparam			: IN sent by the window procedure
    @returns void
    */
    void                OnDlgProcWmCommand(WPARAM wparam, LPARAM lparam);

    /// Inserts items to tab control
    /**
    @param hwndtab: IN handle to tab control to insert tab
    @param pos: IN position where to insert
    @param captions: IN new tab caption
    @returns void
    */
    void                InsertTab(HWND hwndtab, wyInt32 pos, wyWChar *caption);

    /// Shows the data in the viewer
    /**
    @param setsave			: IN scintilla save mode
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				ShowData(wyBool setsave = wyTrue);

  

    /// Shows the binary data in the viewer
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				ShowBinary();

    /// Shows the Text data in the viewer
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
    wyBool				ShowText();

    /// Prepares the screen to display image
    /**
    @param hdc				: IN device handle
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				PrepareScreen(HDC hdc);

    /// Handles the tab columns selection change
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				OnTabSelChange();

    /// Function to open a data and set it as a buffer,
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				OpenFromFile();

    /// Function to save the selected data into a file.
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				SaveToFile();

    /// Processes the OK button press in the blob viewer
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				ProcessOK();

    /// Function is called when the user presses cancel button
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				ProcessCancel();

	//Sests the dialog when pops up
	/**
	@return void
	*/
	void				MoveDialog();

	//Resizes the blob viewer
	/**
	@param lparam			: Specifies additional message-specific information. 
	@return void
	*/
	void				Resize(LPARAM lParam);

    /// Handles enabling/disabling blob viewer
    /**
    @param disable			: IN wyTrue/wyFalse, default wyTrue
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				DisableAll(wyBool disable = wyTrue);

    /// Sets the font for editor window
    /**
    @param handed			: IN handle to the editor window
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool				SetEditFont(HWND hwndedit);
	
	/// Adds the Specified Text in Combo Box
    /**
    @param text		   	   : IN handle to the editor window
    @returns wyBool wyTrue if success, otherwise wyFalse
    */
	wyBool AddTextInCombo(const wyChar * text);
	
	void   SetComboBox(wyWChar *codepagename);
	wyBool ExamineData();
	void OnComboChange();
	void InitEncodingType();
	void ProcessComboSelection(wyString &buffstr);
	wyChar* Utf8toAnsi(const wyChar *utf8, wyInt32	len);
	wyChar* AnsitoUtf8(const wyChar *ansistr, wyInt32 len);
	wyChar* Ucs2toAnsi(const wyChar *ansistr, wyInt32 len);
	wyChar* Ucs2toUtf8(const wyChar *ansistr, wyInt32 len);
    wyWChar* AnsitoUcs2(const wyChar *ansistr, wyInt32 len);
	wyWChar* Utf8toUcs2(const wyChar *ansistr, wyInt32 len);

	/// display the Blob size
    /**
    @param size		   	   : IN Data size
    @returns void
    */
	void		ShowBlobSize(wyInt32 size);

    /// Paints the image in the window screen 
    /**
    @returns LRESULT 1 if success, otherwise 0
    */
	LRESULT				ShowImage();

    //Function perfoms FindNext on F3
    /**
    @param wnd          : IN scintilla window handle
    @returns void
    */
    void                OnAccelFindNext(HWND hwnd);

    /// Main window handle
	HWND				m_hwnddlg;

    /// Tab window handle
    HWND                m_hwndtab;

    /// Image window handle
    HWND                m_hwndimage;

    /// Edit window handle
    HWND                m_hwndedit;

    /// Setnull window handle
    HWND                m_hwndnull;

	/// ComboBox handle
	HWND				m_hwndcombo;

    /// Old data is null or not
	wyBool				m_oldnull;

    /// Editable or not
    wyBool              m_edit;
	
	/// Encoding Format is ANSI?	
	wyBool				m_isansi;
	
	////// Encoding Format is UTF-8?	
	wyBool				m_isutf8;
	
	/// Encoding Format is UCS-2?
	wyBool				m_isucs2;
	
	///BLOB data in utf-8 format is maintained here
	wyString			m_blobdata;

	///Changed Combo String
	wyString			m_changedcombotext;

	///Whether the encoding format is changed or not
	wyBool				m_isencodingchanged;

	// used to find the initial setnull check box state
	wyInt32				m_checkboxstate; 

    /// Old data pointer
	wyChar				*m_olddata;

    /// New data pointer
    wyChar             *m_newdata;

    /// Old data size
	wyInt32             m_olddatasize;
    
	/// New data size
    wyInt32             m_newdatasize;

	///Encoding type
	wyString			m_encodingtype;

    /// Editor font handle
	HFONT				m_hfont;

    /// Original paint window procedure
	WNDPROC				m_wporigpaintproc;
    
	/// Original text window procedure
    WNDPROC             m_wporigtextproc ;

    /// Current PINSERTUPDATEBLOB structure
	PINSERTUPDATEBLOB	m_piub;

	/// Is Blob or Text?
	wyBool				m_isblob;

	//Object for FindAndRepalce class
	FindAndReplace		*m_findreplace;
};

#endif
