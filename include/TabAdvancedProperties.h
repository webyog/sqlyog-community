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

struct TabAdvPropValues
{
    wyString    m_checksum;
    wyString    m_auto_incr;
    wyString    m_avg_row;
    wyString    m_comment;
    wyString    m_max_rows;
    wyString    m_min_rows;
    wyString    m_delay;
    wyString    m_rowformat;
    wyBool      m_changed;
};

class TableTabInterfaceTabMgmt;

class TabAdvancedProperties
{
public:

    TabAdvPropValues            m_tabadvprop;

    struct TabAdvPropValues     m_advprop;

    HWND                        m_hwndparent;

    HWND                        m_hwnd;

    CRITICAL_SECTION            m_cs;

    HWND                        m_hstaticcomment;

    HWND                        m_heditcomment;

    HWND                        m_hstatautoincr;

    HWND                        m_heditautoincr;

    HWND                        m_hstatavgrowlen;

    HWND                        m_heditavgrowlen;

    HWND                        m_hstatmaxrows;

    HWND                        m_heditmaxrows;

    HWND                        m_hstatminrows;

    HWND                        m_heditminrows;

    HWND                        m_hstatrowformat;

    HWND                        m_hcmbrowformat;

    HWND                        m_hstatdelaykeywrite;

    HWND                        m_hcmbdelaykeywrite;

    HWND                        m_hstatchecksum;

    HWND                        m_hcmbchecksum;

    HWND                        m_hlastfocusedwnd;

	HWND						m_hwndscroll;

    WNDPROC                     m_wporigcomment;

    WNDPROC                     m_wporigautoincr;

    WNDPROC                     m_wporigavgrowlen;

    WNDPROC                     m_wporigmaxrows;

    WNDPROC                     m_wporigminrows;

    WNDPROC                     m_wporigrowformat;

    WNDPROC                     m_wporigchecksum;

    WNDPROC                     m_wporigdelaykey;

    wyString                    m_origchecksum;

    wyString                    m_origautoincr;
    
    wyString                    m_origavgrow;

    wyString                    m_origcomment;
    
    wyString                    m_origmaxrows;
    
    wyString                    m_origminrows;
    
    wyString                    m_origdelay;
    
    wyString                    m_origrowformat;

    wyString                    m_prevval;

    MDIWindow*                  m_mdiwnd;

    wyBool                      m_ismysql41;
	
	wyBool                      m_ismysql553;//if mysql version is greater than 5.5.3

    TableTabInterfaceTabMgmt*   m_ptabmgmt;

	wyInt32						m_dispheight;

	wyInt32						m_scrollpos;

	wyInt32						m_prevscrollpos;

	RECT						m_rectparent;

    wyBool                      m_disableenchange;

    TabAdvancedProperties(HWND hwndparent, TableTabInterfaceTabMgmt *ptabmgmt);

    /// intializes m_mysql, m_tunnel and calls CreateGrid()
    /**
    @returns void
    */
    wyBool                      Create();

    /// Creates other windows
    /**
    @param  hwndparent          : IN    parent window for other windows
    @returns void
    */
    VOID                        CreateOtherWindows(HWND hwndparent);

    /// Resize function
    /**
    @returns void
    */
    VOID						Resize();

    /// Helper functions to initialize the dialog
    

    /// Fill checksum value options
    /**
	@returns void
    */
    void                        InitCheckSumValues(wyBool includedefault=wyTrue);

    /// Fill delay key write values
    /**
	@returns void
    */   
	void                        InitDelayKeyWriteValues(wyBool includedefault=wyTrue);

    /// Fill rowformat values
    /**
	@returns void
    */    
	void                        InitRowFormatValues(wyBool includedefault=wyTrue);

    /// Initializes All Windows
    /**
    @param  includedefault      : IN    whether to include "Default" or not
    @returns void
    */
    void                        InitAllWindows(wyBool includedefault=wyTrue);

    /// Appends other options to the query
    /**
    @param  query               : IN\OUT    query - wyString object    
    @returns void
    */
    void                        GenerateCreateQuery(wyString &query);

    /// 
    wyBool                      FetchInitValues();

    /// Appends other options to the query
    /**
    @param  includedefault      : IN    whether to include "Default" or not
    @returns wyTrue if successfully fetched and filled, else wyFalse
    */
    wyBool                      FillInitValues(wyBool   iscancelchanges = wyFalse);

    //Gets the maximum rows from myrow
    /**
    @param  textrow         : IN mysql row value
    @param  tofind          : IN Text to find
    @parasm retval          : OUT returning values
    @returns the integer form of the value
    */
    wyInt32                     GetValues(wyChar *textrow, wyChar *tofind, wyString &retval);

    /// Generates the query for Altered Properties (Options)
    /**
    @param  query           : IN\OUT    query - wyString object
    @param  querylenbuf     : OUT       query length 
    @returns void
    */
    //void                        GetAlteredProperties(wyString &query, wyUInt32& querylenbuf);

    //  Handled the Tab/Tab+Shift key to navigate from one control to another in the single Dialog
	static LRESULT CALLBACK     HandleTabKey(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    //  Procedure to handle Alt+1, etc. when Tabbed Interface is in a dialog-box(in SD).
    static LRESULT	CALLBACK	SysKeyDownWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    //..makes Windows Visible/Invisible
    /*
    @returns    void
    */
    wyInt32             OnTabSelChange();

    /// Cancels the modification and ReInitializes all the window values
    /*
    @param isaltertable         : IN    
    @returns    void
    */
    void                CancelChanges(wyBool    isaltertable=wyFalse);

    /// Binds all values to the Windows(makes the changes permanent) after table is "Saved" (created/altered)
    /*
    @param isaltertable     : IN    
    @returns    void
    */
    //void                BindOptionsValues(wyBool    isaltertable);

    //..To format the Adv. Prop query.
    wyBool              FormatString(wyString &query, wyInt32* ind);

    void                SetStaticWindowPosition(HWND hwnd, HWND hwndedit, wyInt32 lpos, wyInt32 width, wyInt32 height);

    wyBool              GenerateQuery(wyString &query);

    //void                GenerateCreateQuery(wyString &query);

    void                GenerateAlterQuery(wyString &query);

    wyBool              ReinitializeValues();

    void             GetMaxWidthOfStaticText(wyInt32* col1);

    //wyInt32             GetTextLenOfStaticWindow(HWND hwnd, RECT* recttext);

	static LRESULT CALLBACK     ScrollbarWndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void ScrollWindows(wyBool isdown, wyInt32 scrollpos = 0);

	static BOOL CALLBACK        ScrollWindowsDownProc(HWND hwnd, LPARAM lParam);

	static BOOL CALLBACK        ScrollWindowsUpProc(HWND hwnd, LPARAM lParam);

	static BOOL CALLBACK        ScrollWindowsByPosProc(HWND hwnd, LPARAM lParam);

	void OnMouseWheel(WPARAM wparam);

	void OnVScroll(WPARAM wparam);

	void OnTabSelChanging();
};