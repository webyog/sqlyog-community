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

#include "Global.h"
#include "FrameWindowHelper.h"
#include "CustTab.h"

class TabFields;
class TabAdvancedProperties;
class TabIndexes;
class TabForeignKeys;
class TabPreview;
class TabCheck;



class TableTabInterfaceTabMgmt
{
public:
    ///Constructor with parent window as argument
    TableTabInterfaceTabMgmt(HWND hwndparent, TableTabInterface* ptabint);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~TableTabInterfaceTabMgmt();

    /// Creates the tab window control
    /**
    @param wnd				: IN the parent query window pointer
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool      Create();

    /// Sets the parent TabInterface Window pointer
    /*
    @@param TabTypes* : IN  the TabInterface Window Pointer to set
    @@returns wyBool    wyTrue if it is SUCCESS, else wyFalse
    */
    //wyBool      SetTabInterfacePtr(TabTypes *te);

    wyBool      CreateFieldsTab();

    wyBool      CreateAdvancedPropertiesTab();

    wyBool      CreateIndexesTab();

    wyBool      CreateForeignKeysTab();

	wyBool CreateCheckConstraintTab();

    wyBool      CreatePreviewTab();

    wyBool      CreateTableTab();

    void        Resize(HWND hwnd);

    void		OnTabSelChange(wyBool isonlyupdatestatus = wyFalse);

    wyInt32     OnTabSelChanging();

    /// window procedure for the tab control
	static LRESULT CALLBACK     WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool *pishandled);

    static LRESULT	CALLBACK	SubTabsCommonWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    static LRESULT	CALLBACK	ToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void        OnToolTipInfo(LPNMTTDISPINFO lpnmtt);
    
    wyBool      CreateCommonWindow();

    /// Resizes the tab control
    /**
    @returns void
    */
	void		Resize();

    /// Tab window handle
    HWND		                m_hwnd;

    HWND                        m_hcommonwnd;

    /// Tool window HANDLE
    HWND                        m_hwndtool;

    /// Parent window HANDLE
	HWND		                m_hwndparent;

    TableTabInterface*          m_tabinterfaceptr;

    TabFields*                  m_tabfields;

    TabAdvancedProperties*      m_tabadvprop;

    TabIndexes*                 m_tabindexes;

    TabForeignKeys*             m_tabfk;

	TabCheck*                 m_tabcheck;

    TabPreview*                 m_tabpreview;

    wyBool                      m_allsubtabsloaded;

	MDIWindow*					m_wnd;

    /// Handle to image list
	HIMAGELIST	                m_himglist;

    WNDPROC                     m_origtoolwndproc;

    wyBool                      m_isbuffereddraw;

    wyString                    m_tooltipstr;

    /// Selects a particular tab
    /**
    @param index		: IN tab index to select
    @returns void
    */
	void		SelectTab(wyInt32 index);

    wyBool      CreateOtherWindows(HWND hwnd);

    /// Creates the toolbar in the tabledata window
    /**
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
    wyBool      CreateToolBar(HWND hwnd);

    /// Adds buttons in the tabledata window toolbar.
    /**
     */
    void        AddToolButtons();

    void        ShowHideToolButtons(wyBool hide);

    ///Standard window procedure to enumerate child windows
    /**
    @param hwnd                 : IN handle to the window
    @param lParam               : IN user supplied parameter
    @returns TRUE on scucess else FALSE
    */
    static BOOL CALLBACK        ShowWindowsProc(HWND hwnd, LPARAM lParam);

    wyInt32                     GetActiveTabImage();

    wyBool                      OnWMCommand(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void                        OnClickAddRow();

    void                        OnClickDeleteRows();

    void                        OnClickMoveUp();

    void                        OnClickMoveDown();

    void                        OnSetFocus();

    wyBool                      OnSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam);
};