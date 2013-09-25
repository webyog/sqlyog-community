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

#ifndef __PREF__
#define	__PREF__

#define			EDITFONT		 "EditFont"
#define			DATAFONT		 "DataFont"
#define			HISTORYFONT		 "HistoryFont"
#define         OBFONT           "OBFont"
#define			BLOBFONT		 "BlobFont"
#define			TIMEOUT			 "TimeOut"
#define         GENERALPREF_PAGE    0
#define         AC_PAGE             1
#define         FONT_PAGE           2
#define			FORMATTER_PAGE		3
#define			OTHERS_PAGE         4



// Style type for Tree View Items
#define EDITOR                      1
#define EDITOR_DEFAULT              2
#define EDITOR_COMMENTS             3
#define EDITOR_STRINGS              4
#define EDITOR_FUNCTIONS            5
#define EDITOR_KEYWORDS             6
#define EDITOR_NUMBER               7
#define EDITOR_OPERATOR             8
#define EDITOR_HIDDENCOMMAND        9
#define EDITOR_LINENUMBERMARGIN     10
#define EDITOR_FOLDINGMARGIN        11
#define EDITOR_TEXTSELECTION        12
#define EDITOR_BRACESMATCH          13
#define EDITOR_BRACESUNMATCH        14
#define CANVAS                      15
#define CANVAS_LINE                 16
#define CANVAS_TEXT                 17
#define MTI                         18
#define MTI_SELECTION               19


// Mask to enable Foreground and Background buttons for customising colors
#define IF_FOREGROUND_MASK           1
#define IF_BACKGROUND_MASK           2


// structure to store tree view item's data
struct TREEVIEWDATA
{
    int mask;                   // Mask can be IF_FOREGROUND_MASK
    COLORREF *fgColor;          // Pointer to Foreground Color
    COLORREF *bgColor;          // Pointer to Background color
    int style;                  // Style to identify type of trre view item.
};


#define COMMMUNITY_POWERTOOLS _(L"Power Tools Options (Professional/Enterprise/Ultimate only) ")
#define COMMUNITY_FORMATTER _(L"SQL Formatter Options (Enterprise/Ultimate only) ")
#define FORMATTER_PREVIEW		"SELECT\n  t1.column1 AS c1,\n  t1.column2 AS c2\nFROM table1 t1,\n  table2 t2\n\
WHERE t1.column1 = t2.column1\nGROUP BY c1, c2\nORDER BY c1;"


class PreferenceBase
{
public:	

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	PreferenceBase();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	virtual ~PreferenceBase();

    /// Creates the preferences dialog box
    /**
	@param startpage : IN index of the tab to be selected
    @returns void
    */
	void        Create(wyInt32 startpage = 0); 

    /// Window procedure for the general preferences window
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static	INT_PTR CALLBACK	GeneralPrefDlgProc(HWND hwnd, UINT messahe, WPARAM wParam, LPARAM lParam);
    
	/// Window procedure for the Autocomplete preferences
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
	static	INT_PTR CALLBACK	ACPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Window procedure for the Autocomplete preferences
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
	static	INT_PTR CALLBACK	FormatterPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
	/// Window procedure for the font preferences
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
	static	INT_PTR CALLBACK	FontPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Window procedure for the others preferences window
	/**
	@param hwnd			: IN Windows HANDLE
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static	INT_PTR CALLBACK	OthersPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
	/// The callback function helps to apply changes  to all windows 
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	*/
	static	wyInt32 CALLBACK	EnumChildProc(HWND hwnd, LPARAM lParam);

    /// Apply changes to the file
    /**
    @returns void
    */
    virtual void		Apply()=0;

	/// Handle the WM_INITDIALOG on Autocomplete_pref dialog
	/**
    @param hwnd: IN window handle
    @returns void
    */
	virtual void        EnterprisePrefHandleWmInitDialog(HWND hwnd) = 0;

	/// Handle the WM_COMMAND on Autocomplete_pref dialog
	/**
    @param hwnd		: IN window handle
	@param wparam	: IN Unsigned message parameter
    @returns void
    */
    virtual void        EntPrefHandleWmCommand(HWND hwnd, WPARAM wParam) = 0;

	/// Handle the WM_INITDIALOG on Formatterpref dialog
	/**
	@param hwnd : IN window handle
	@return void
	*/
    virtual void        FormatterPrefHandleWmInitDialog(HWND hwnd) = 0;

	/// Handle the WM_NOTIFY on Autocomplete dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
	virtual void        ACPrefHandleWmNotify(HWND hwnd, LPARAM lparam)=0;
	
	/// Handle the WM_NOTIFY on Formatterpref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    virtual void        FormatterPrefHandleWmNotify(HWND hwnd, LPARAM lparam)=0;
	
	/// Handle the WM_COMMAND on Formatter_pref dialog
	/**
	@param hwnd : IN Handle to tab
	@param wParam : IN WPARAM message parameter
	@return void
	*/
    virtual void        FormatterPrefHandleWmCommand(HWND hwnd, WPARAM wParam) = 0;
    
	/// Saves the General preferences values
    /**
    @param hwndbase		: IN Base window handle
    @returns wyBool, wyTrue if SUCCESS, otherwise wyFalse
    */
    wyBool      SaveGeneralPreferences(HWND hwndbase, wyInt32 page);
    
    /// Saves the Font preferences values
    /**
    @param hwndbase: IN Base window handle
    @returns wyBool, wyTrue if SUCCESS, otherwise wyFalse
    */
    wyBool      SaveFontPreferences(HWND hwndbase, wyInt32 page);

	/// Saves the Others preferences values
    /**
    @param hwndbase		: IN Base window handle
    @returns wyBool, wyTrue if SUCCESS, otherwise wyFalse
    */
    wyBool      SaveOthersPreferences(HWND hwndbase, wyInt32 page);

	///Set the toolbar icon size on save preference change
	/**
	@returns wyBool, wyTrue if SUCCESS, otherwise wyFalse
	*/
	wyBool		SetToolBarIconSize();

    /// Writes the Bool type profile string
    /**
    @param hwnd			: IN parent window handle
    @param appname		: IN section name
    @param keyname		: IN key attribute string
    @param id			: IN control id
    @returns wyBool, what value is written
    */
    wyBool      SetBoolProfileString(HWND hwnd, wyWChar *appname, wyWChar *keyname, wyInt32 id);

    /// Writes the wyInt32 type profile string
    /**
    @param hwnd			: IN parent window handle
    @param appname		: IN section name
    @param keyname		: IN key attribute string
    @param id			: IN control id
    @returns wyInt32, what value is written
    */
    wyInt32     SetIntProfileString(HWND hwnd, wyWChar *appname, wyWChar *keyname, wyInt32 id);

    /// Sets the font preferences details
    /**
    @param hwnd			: IN handle to the window
    @param appname		: IN Sectionname
    @param font			: IN LPLOGFONT pointer
    @returns void
    */
    void        SetFontPrefDetails(HWND hwnd, wyWChar * appname, LPLOGFONT font);

    /// Sets the color preferences values
    /**
    @returns void
    */
    void        SetColorPrefDetails();

	/// Sets the init general preferences
	/**
	@returns void
	*/
	void		InitGeneralPrefValues();

	/// Disable /Enable RowLimit
    /**
    @param hwnd			: IN window handle
    @param enable		: IN wyTrue/wyFalse
	@returns void
    */
    void        EnableRowLimit(HWND hwnd, wyInt32 enable);
   
	/// disable/enable BulkInsert
    /**
    @param hwnd			: IN window handle
    @param enable		: IN wyTrue/wyFalse
	@returns void
    */
    void        EnableBulkInsert(HWND hwnd, wyInt32 enable);

    /// Gets the general preferences size values
    /**
    @param hwnd			: IN window handle
	@returns void
    */
    void        GetGeneralPrefSizeValues(HWND hwnd);

    /// Initializes the Font preferences values
    /**
    returns void
    */
	void		InitFontPrefValues();

    ///Sets the font details
    /**
    @returns void
    */
	void		SetFontDetails();

    /// Gets the font details from .ini
    /**
    @param appname		: IN section name
    @param font			: OUT font info
    @returns void
    */
    void        GetFontDetails(wyWChar *appname, PLOGFONT font, wyChar* deffontface = NULL);

    ///Gets the font size according to the current device context
    /**
    @param font			: IN Font details
    @returns wyInt32, fontsize
    */
    wyInt32 	GetFontSize(PLOGFONT font);

    /// Helps to show the details on the window
    /**
    returns void
    */
	void		PrintFontDetails();

    /// Helps 
	// void		ShowColorDetails(HWND);
    /// Sets the color details
    /**
    @returns void
    */
	//void		SetColorDetails();

    ///function initializes the choosefont structure and opens up the choosefont dialog
    /**
    @param lf			: OUT font info
	returns wyTrue if font chosen else wyFalse
    */
	wyBool		ChFont(PLOGFONT lf);

	/// Sets the init others preferences
	/**
	@returns void
	*/
	void		InitOthersPrefValues();

	/// Gets the others preferences size values
    /**
    @param hwnd			: IN window handle
	@returns void
    */
    void        GetOthersPrefSizeValues(HWND hwnd);

    /// Handel the WM_INITDIALOG on gen_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        GenPrefHandleWmInitDialog(HWND hwnd, LPARAM lparam);
    
	/// Handle the WM_INITDIALOG on font_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@returns void
	*/
    void        FontPrefHandleWmInitDialog(HWND hwnd);

	/// Handle the WM_INITDIALOG on others_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OthersPrefHandleWmInitDialog(HWND hwnd, LPARAM lparam);

    /// Handle the WM_COMMAND on gen_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        GenPrefHandleWmCommand(HWND hwnd, WPARAM wparam);
    
	/// Handle the WM_COMMAND on font_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        FontPrefHandleWmCommand(HWND hwnd, WPARAM wparam);

	/// Handle the WM_COMMAND on others_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OthersPrefHandleWmCommand(HWND hwnd, WPARAM wparam);
    
	/// Handle the WM_NOTIFY on gen_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        GenPrefHandleWmNotify(HWND hwnd, LPARAM lparam);
    
	/// Handle the WM_NOTIFY on font_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        FontPrefHandleWmNotify(HWND hwnd, LPARAM lparam);

	/// Handle the WM_NOTIFY on others_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void        OthersPrefHandleWmNotify(HWND hwnd, LPARAM lparam);
    
	/// Handle the WM_CTLCOLORSTATIC on font_pref dialog
	/**
	@param hwnd			: IN Windows HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    wyInt32     FontPrefHandleWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Setting default General preferences values when we on click RestoreAllTabs button
    /**
    @param hwndbase		: IN Base window handle
	@param page			: IN page number
    @returns void
    */
	void		SetGenPrefDefaultAllTabValues(HWND hwndbase, wyInt32 page);
	
	/// Setting default General preferences values
    /**
    @param hwnd		: IN window handle
	@returns void
    */
	void		SetGenPrefDefaultValues(HWND hwnd);

	/// Sets the general preferences size values
    /**
    @param hwnd			: IN window handle
    @returns void
	*/
	void		SetGeneralPrefDefaultSizeValues(HWND hwnd);

	/// Setting default Others preferences values when we on click RestoreAllTabs button
    /**
    @param hwndbase		: IN Base window handle
	@param page			: IN page number
    @returns void
    */
	void		SetOthersPrefDefaultAllTabValues(HWND hwndbase, wyInt32 page);

	/// Setting default Others preferences values 
    /**
    @param hwnd		: IN window handle
	@returns void
    */
	void		SetOthersPrefDefaultValues(HWND hwndbase);

	/// Sets the others preferences size values
    /**
    @param hwnd			: IN window handle
    @returns void
	*/
	void		SetOthersPrefDefaultLimitValues(HWND hwnd);

	/// Setting default Font preferences values when we click on RestoreAllTabs button
    /**
    @param hwndbase		: IN Base window handle
	@param page			: IN page number
    @returns void
    */
	void		SetFontPrefDefAllTabValues(HWND hwndbase, wyInt32 page);

	/// Setting default Font preferences values
    /**
    @param hwnd			: IN window handle
	@returns void
    */
	void		SetDefaultFontDetails(HWND hwnd);

	/// Sets the default Font preferences color values
    /**
    @param hwnd			: IN window handle
    @returns void
	*/
	void        SetDefaultColorDetails(HWND hwnd=NULL);

	/// Fill default Font for the window
	/**
	@param logf     : IN Font details
	@param hwnd     : IN Window HANDLE
	@returns void
	*/
	void		FillDefaultEditFont(PLOGFONT	logf, HWND hwnd);
	
	/// Saves the General preferences with default values
    /**
    @returns void
    */	
	void		SaveDefaultGeneralPreferences();

	/// Saves the Other preferences with default values
    /**
    @returns void
    */
	void		SaveDefaultOthersPreferences();

	/// Saves the Editor preferences with default values
    /**
    @returns void
    */
	void		SaveDefaultEditorPreferences();
    
	 /// Saves the Font preferences with Default values
    /**
    @param appname		: IN section name
    @returns void
    */
	void		SaveDefaultFontPreferences(wyWChar * appname);
	
	///Restore Defaults for all the tabs
    /**
    @returns void
    */
	virtual void			RestoreAllDefaults() = 0;

	///Fills CaseCombo in Formatter Pref dialog
	/**
    @param hwndcombo	: IN ComboBox handle
	@returns void
    */
	void					FillCaseCombo(HWND hwndcombo);

	///Fills IconSize ComboBox in Others Pref dialog
	/**
    @param hwndcombo	: IN ComboBox handle
	@returns void
    */
    void					FillSizeCombo(HWND hwndcombo);

    void                    FillThemeCombo(HWND hwndcombo);

	 /// Enable or Disable the given list of IDs
    /**
    @param hwnd         : IN Window HANDLE
    @param array        : IN array if ids
    @param arraycount   : IN Number of ids
    @param enable       : IN Flag for showing or hiding
    */
    static  void			EnableOrDisable(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable);

	/// Sets preview editor properties
    /**
	@param hwnd		: IN Preview window handle
    @returns void
    */	
	void					SetPreviewEditor(HWND hwnd);

    //For Toggling Foreground and Background Buttons behavious
    void SetFrgndBkgndBtn(HWND hwnd,BOOL ,BOOL);


    void ChangeBkFrColor(HWND hwnd,COLORREF frGnd, COLORREF bkGnd);


    void SetColorsInStaticControl(HWND hwnd);
    
    /// Called to destroy dialog tree view item's data
    void OnDestroy(HWND hwnd);
        
	/// Ini path
	wyWChar		m_directory[MAX_PATH + 1];
    
	/// Window HANDLE
	HWND		m_hwnd;

    /// Editor font info
	LOGFONT		m_editfont;
    
	/// Data font info
	LOGFONT		m_datafont;

	/// Contains history and object tab font information
	LOGFONT		m_historyfont;

    /// Contains Object Browser font information
    LOGFONT     m_obfont;

	/// Contains blob viewer font information
	LOGFONT		m_blobfont;

    // to hold current background and foreground color of the static control for showing preview
    COLORREF m_rgbbgcolor;
    COLORREF m_rgbfgcolor;

	// holds folding margin texture color. It holds value White for default otherwise same as folding margin color
    COLORREF m_rgbtexturecolor;

    /// Window HANDLE for font tab
	HWND		m_hwndfonttab;

    /// Handle to Treeview
    HWND        m_htv;

	//Whether RestoreAllTabs button is clicked or not
	wyBool		m_isrestorealldefaults;

	//keywords for scintilla editor
	wyString	m_keywordstring;

	//functions for scintilla editor
	wyString	m_functionstring;

	//Default start page by default
	wyInt32		m_startpage;

    wyBool      m_isthemechanged;

    wyBool      m_istabledataunderquery;

    wyBool      m_isinfotabunderquery;

    wyBool      m_ishistoryunderquery;

	wyBool		m_ispreferenceapplied;
};

#endif
