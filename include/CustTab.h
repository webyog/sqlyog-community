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

/*****************************************************************************
Rewritten by : Vishal P.R

This is the Version 2 of Custom Tab control. The drwing logic has been 
rewritten completly to enable improved scrolling and tab behaviors.
The external interfaces are kept as it is to make it compatible with the 
exisitng SQLyog code base, however some minor changes has been done to improve 
the perfomance and a few redundant members are removed.
*****************************************************************************/

#ifndef _CUST_TAB_H_
#define _CUST_TAB_H_

#include <windows.h>
#include <commctrl.h>
#include "datatype.h"
#include "wyString.h"

typedef LRESULT(CALLBACK* CTBWNDPROC)(HWND, UINT, WPARAM, LPARAM, wyBool* pishandled);

#define TBM_SETLONGDATA         WM_USER+1
#define CTCN_SELCHANGING        WM_USER+112
#define CTCN_SELCHANGE          WM_USER+113 
#define CTCN_TABCLOSING         WM_USER+114 
#define CTCN_TABCLOSED          WM_USER+115 
#define	CTCN_WMCOMMAND          WM_USER+116
#define	CTCN_WMDESTROY          WM_USER+117
#define CTCN_WMCTLCOLORSTATIC   WM_USER+118
#define CTCN_ONCONTEXTMENU      WM_USER+119
#define CTCN_LBUTTONDBLCLK      WM_USER+122
#define CTCN_ICONDROPDOWNNOTIFY WM_USER+124
#define CTCN_GETCHILDWINDOWS    WM_USER+126
#define CTCN_PLUSBUTTONCLICK    WM_USER+128
#define CTCN_PAINTTIMERSTART    WM_USER+130
#define CTCN_PAINTTIMEREND      WM_USER+132
#define CTCN_TABRENAME		    WM_USER+133
#define CTCN_DROPDOWNBUTTONCLICK    WM_USER+134

#define CTBIF_TEXT              1
#define CTBIF_IMAGE             2
#define CTBIF_LPARAM            4
#define CTBIF_CMENU             8
#define CTBIF_COLOR             16
#define CTBIF_TOOLTIP           32


#define CTCF_TABBG1                     0x1
#define CTCF_TABBG2                     0x2
#define CTCF_SELTABFG1                  0x4
#define CTCF_SELTABFG2                  0x8
#define CTCF_SELTABTEXT                 0x10
#define CTCF_TABTEXT                    0x20
#define CTCF_ACTIVESEP                  0x40
#define CTCF_INACTIVESEP                0x80
#define CTCF_CLOSEBUTTON                0x100
#define CTCF_TABFG                      0x200
#define CTCF_DRAGARROW                  0x400
#define CTCF_TABCONTROLS                0x800
#define CTCF_HIGHLIGHTSEP               0x1000
#define CTCF_BORDER                     0x2000
#define CTCF_HOTTABFG1                  0x4000
#define CTCF_HOTTABFG2                  0x8000
#define CTCF_LINK                       0x10000    
#define CTCF_BOTTOMLINE                 0x20000
#define CTCF_ALLEXECPT_CTCF_BOTTOMLINE  ((-1) & (~CTCF_BOTTOMLINE))

typedef struct{
    wyUInt32    m_mask;
    COLORREF    m_tabbg1;
    COLORREF    m_tabbg2;
    COLORREF    m_tabfg;
    COLORREF    m_seltabfg1;
    COLORREF    m_seltabfg2;
    COLORREF    m_seltabtext;
    COLORREF    m_tabtext;
    COLORREF    m_activesep;
    COLORREF    m_inactivesep;
    COLORREF    m_closebutton;
    COLORREF    m_dragarrow;
    COLORREF    m_bottomline;
    COLORREF    m_tabcontrols;
    COLORREF    m_highlightsep;
    COLORREF    m_hottabfg1;
    COLORREF    m_hottabfg2;
    COLORREF    m_linkcolor;
    wyBool      m_border;
}TABCOLORINFO, *LPTABCOLORINFO;


//structur for tab notifications
typedef struct{
    //basic windows notification structure
    NMHDR   hdr;

    //number of elements in array. Used for CTCN_GETCHILDWINDOWS
    //the tab going to be selected. Used for CTCN_SELCHANGING
    wyInt32 count;

    //array of windows. Used for CTCN_GETCHILDWINDOWS
    HWND*   phwnd;
    
    //set this value to non zero to acknowledge the notification used for CTCN_GETCHILDWINDOWS
    //set this value to non zero to send CTCN_SELCHANGE after CTCN_TABCLOSED
    wyInt32 retvalue;

    //current cursor position, in screen cordinates
    POINT   curpos;

    //any additional information for the notification
    LPARAM  lparam;
}NMCTC, *LPNMCTC;

/*! \struct tab_item_information
    \brief Used to store tab item information
    \param wyUInt32	m_mask whether the tab contain text/Image/param
                            It can be any one of the following or a combination,
                            CTBIF_TEXT, CTBIF_IMAGE, CTBIF_LPARAM, CTBIF_CMENU
    \param LPSTR m_psztext The tab caption
	\param wyInt32 m_cchtextmax The maximum caption length
	\param wyInt32 m_iimage The image id
	\param wyInt32 m_lparam The long value that we can keep
*/
struct tab_item_information 
{
	wyUInt32	m_mask; 
    LPSTR	    m_psztext;
    wyInt32	    m_cchtextmax;
    wyInt32		m_iimage;
    LPARAM		m_lparam;
	COLORREF	m_color;//Color of tab
	COLORREF	m_fgcolor;//Text color of tab
	LPSTR		m_tooltiptext;//tooltip text
	
};

/*! Creates a type tab_item_information*/ 
typedef tab_item_information CTCITEM,*LPCTCITEM;

struct tab_item_information_ex;
typedef tab_item_information_ex CTCITEMEX,*LPCTCITEMEX;


class CCustTab
{
public:
	/// Default constructor
    /**
    @param hwnd: IN handle to the Grid
    */
	                                CCustTab(HWND hwnd);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	                                ~CCustTab	();

       /// Custom Tab window procedure
	static	LRESULT	CALLBACK        CustomTabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Inserts a tab item in to the tab control
    /**
    @param position		            : IN new tab item position
    @param pitem		            : IN LPCTCITEM structure containing tab item details
    @returns wyInt32 it will return position if it is success, otherwise -1
    */
    wyInt32                         InsertItem(wyInt32 position, LPCTCITEM pitem);

    /// Sets the tab item details
    /**
    Used to change the tab details
    @param position		            : IN the position of the tab to change
    @param pitem		            : IN LPCTCITEM structure containing tab item details
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool                          SetItem(wyInt32 position, LPCTCITEM pitem);

	/// Change color of a particular tab item
    /**
    Used to change the tab details
    @param position		            : IN the position of the tab to change
    @param pitem		            : IN LPCTCITEM structure containing tab item details
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool		                    SetItemColor(wyInt32 position, LPCTCITEM pitem);

    /// Gets that long value associated with the tab control
    /**
    @return LPARAM value associated with the tab control
    */
    LPARAM                          GetLongData();

    /// Sets that long value to the tab control
    /**
    @param lparam		: IN value to set the tab control
    */
	LRESULT                         SetLongData(LPARAM lparam);

    /// Sets along value to the tab
    /**
    Sets along value to the particular tab.
    @param tabindex		            : IN index of the tab to set.
    @param  lParam		            : IN the long value to set
    */
	LPARAM                          SetItemLongValue(wyInt32 tabindex, LPARAM lParam);

    /// Sets the cursel selection to the particular tab
    /**
    @param tabindex		            : IN tab index to be selected
	@param wparam                   : IN the long value to set
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool                          SetCurSel(wyInt32 tabindex, WPARAM wparam = NULL);

    /// Removes a particular tab
    /**
    @param tabindex		            : IN tab index to remove
    @param isexplicit               : IN whether the tab was closed with an explicti close operation
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */

	//Added notdeletefromstruct Param: wyTrue denotes the tab doesn't need to remove from the drop down structure, wyFalse: remove tab details from drop down
	wyBool                          DeleteItem(wyInt32 tabindex, wyBool isexplicit = wyFalse, wyBool notdeletefromstruct = wyFalse);

    /// Remove all tabs
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool                          RemoveAllItem();

    /// Gets the details of a particular tab
    /**
    @param tabindex		            : IN index of the tab.
    @param pitem		            : OUT Structure that contains the tab details
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool                          GetItem(wyInt32 tabindex, LPCTCITEM pitem);
	
	const wyChar*  						GetItemTitle(wyInt32 tabindex, wyString *title);
	const wyChar*  						GetItemTooltip(wyInt32 tabindex, wyString *Tooltip);
    ///Gets the total tab count
    /**
    @returns wyInt32 Number of tabs
    */
    wyInt32                         GetItemCount();

    /// Sets the minimum tab width
    /**
    @param minwidth		: IN minimum width of the tabs
    @returns wyInt32 previous minimum value.
    */
	wyInt32                         SetMinTabWidth(wyInt32 minwidth);

    /// Gets currently selected tab index
    /**
    @returns wyInt32 Currently selected tab index
    */
	wyInt32                         GetCurFocus();

    /// Gets visible tab count
    /**
    @returns wyInt32 Visible tab count
    */
	wyInt32                         GetVisibleTabCount();

    /// Changes tab closable state
    /**
    We can enable/disable the close tab buttons for the tab control
    @param flag			            : IN it can be wyTrue/wyFalse. If it is wyTrue we can 
    close the tabs in the tab control, Otherwise not.
    */
	void                            SetClosable(wyBool flag, wyInt32 mintabcount, wyBool ispaint);

	 /// Set tab length state
    /**
    @param flag			            : IN it can be wyTrue/wyFalse. If it is wyTrue we can 
    */
	void                            SetFixedLengthProp(wyBool val);

	/// Set tab drag propery
    /**
    @param flag			            : IN it can be wyTrue/wyFalse. If it is wyTrue we can 
    @param hwnddragwin              : IN the boundary window
    */
	void                            SetDragProperties(HWND hwnddragwin,  wyBool val);	

    /// Ensures a particular tab item visible
    /**
    @param tabindex		            : IN Index of the tab item
    @param ispaintonlyheader        : IN whether to paint only tab header
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool                          EnsureVisible(wyInt32 tabindex, wyBool isanimate = wyTrue);

    /// Sets the tab control ID
    /**
    @param id			            : IN tab control id to set
    */
    void                            SetId(wyInt32 id);

    /// Checks the context menu is enabled or not
    /**
    @param index                    : IN tab index
    @returns wyBool, wyTrue if context menu enabled, otherwise wyFalse
    */
    wyBool	                        IsMenuEnabled(wyInt32 index);

	/// removes the resources allocated
	/**
	@returns void
	*/
	void		                    DestroyResources();

    ///event handler for WM_SIZE, but is used in other places also, since we need to caluclate the totla tab size for scrolling purpose
    /**
    @returns void
    */
    void                            OnWMSize();

    ///Function paints the tab
    /**
    @param isonlyheader             : IN whether to paint only header
    @returns void
    */
    void                            PaintTab(wyBool ispaintonlyheader = wyFalse);

    ///Function enable/disable the plus button
    /**
    @param enable                   : IN enable/disable 
    @returns void
    */
    void                            EnableAddButton(wyBool enable);

    /// Function to handle all the painting activities
    /**
    @returns LRESULT
    */
	LRESULT                         OnPaint();

    void		                    SetBlockEventsFlag(wyBool val);

    void                            SetVisibleTabBoundary();

    void                            SetPaintTimer();

    void                            SetIconList(HIMAGELIST hiconlist);

    wyBool                          StartIconAnimation(wyInt32 tabindex, wyInt32 delay);

    wyBool                          StopIconAnimation(wyInt32 tabindex, wyBool isrepaint);

    wyInt32                         GetTabHeight();

    void                            SetColorInfo(LPTABCOLORINFO pcolorinfo);

    void                            GetColorInfo(LPTABCOLORINFO pcolorinfo);

    /// handle to tooltip associated with tab
	HWND            m_hwndtooltip;

	/// Stucture containing tooltip info
	TOOLINFO		m_toolinfo;

    //tooltip string
	wyString        m_tooltipstr;

    wyBool		    m_isblock;

    ///whether to use double buffering to draw the tab
    static wyBool   m_isdrawwithdoublebuffering;

private:

    /// Function to get the return code.
    /**
    This function is used to get the return code from the custom tab if the control is on a dialog.
    @param lparam		            : IN LPARAM from the window procedure
    @returns LRESULT DLGC_WANTMESSAGE if lparam valid, otherwise 0;
    */
	LRESULT                         OnGetDLGCode(LPARAM lparam);

  
    /// Handles WM_NOTIFY for the tab control
    /**
    @param lParam                   : IN window procedure LPARAM
    returns LRESULT 1
    */
    LRESULT                         OnWmNotify(LPARAM lParam);

	/// This function is called whenever the system sends notification requiring tool tip text for the tab
    /**
    @param hwnd			            : IN handle to the control
	@param lpnmtdi		            : IN Tool tip display information
    @returns void
    */
	void					        ToolTipInfo(HWND hwnd, LPNMTTDISPINFO lpnmtdi);

    ///WM_LBUTTONDOWN event handler
    /**
    @returns LRESULT
    */
    LRESULT                         OnLButtonDown(WPARAM wPram, LPARAM lParam);

    ///WM_LBUTTONUP event handler
    /**
    @returns LRESULT
    */
    LRESULT                         OnLButtonUp(WPARAM wPram, LPARAM lParam);
    
    /// Handles WM_MOUSEMOVE for the tab control
    /**
	@param hwnd			            : IN handle to the control
    @param wParam		            : IN window procedure WPARAM
    @param lParam		            : IN window procedure LPARAM
    @returns LRESULT 1
    */
	LRESULT                         OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam);

	/// Handles Double click for the tab control
    /**
	@param hwnd			            : IN handle to the control
    @param wParam		            : IN window procedure WPARAM
    @param lParam		            : IN window procedure LPARAM
    @returns LRESULT 1
    */
	LRESULT                         OnLButtonDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
	/// Genetrates the signals like mouse leave etc
	/**
	@return void
	*/
    void                            GetMouseMovement();

	///Handle Tabs when leave the mouse
	/**
	@return void
	*/
	void		                    OnMouseLeave();
    
    /// Handles  WM_CREATE for the tab control
    /**
    @param lParam		            : IN window procedure LPARAM
    @returns LRESULT 1
    */
	LRESULT                         OnCreate(LPARAM lParam);
    
    /// Creates the fonts for the tab control
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool                          CreateFonts();
    
    /// Frees a tab item structure
    /**
    @param pitem		            : IN LPCTCITEM structure to free
    @returns wyBool, wyTrue if success, otherwise wyFalse.
    */
	wyBool                          FreeItem(LPCTCITEMEX pitem, wyInt32 tabindex = -1);

	/// Reset sequence numbers for different tabs
	void							ResetGlobalSequenceNumbers(wyString tabname);

    
    /// Checks the Cursor position is over any of the tabs
    /**
    @param pnt			            : IN the current mouse position
    @param pisoverclosebutton       : OUT whether the mouse is over close button
    @returns the tab index, -1 if not
    */
	wyInt32                         OverTabs(POINT* pt, wyBool* pisoverclosebutton);

    ///Checks whether the pointer is over tab controls, such as scroll button and plus button
    /**
    @params pt                      : IN ursor point
    @params pisoverbutton           : OUT wherher the mouse is over the plus button
    @returns the control id, -1 if not
    */
    wyInt32                         OverTabControls(POINT* pt);
    
    /// Retrieves a particular tab item structure
    /**
    @param tabindex		            : IN tab item index
    @returns LPCTCITEM of the tab item
    */
	LPCTCITEM                       GetSubItemStruct(wyInt32 tabindex);
    
	/// Function is used to set gradient array for selected tab
	/**
	@param hdcmem		            : IN handle to device context
	@param prect		            : IN client area
	@param tabcount		            : IN number of tabs
	@param ptcursor		            : IN cursor position
	@returns void
	*/
	void                            DrawCloseControls(HDC hdcmem, PRECT rect, wyInt32 tabcount, PPOINT ptcursor);

    

	/// Function is used to set gradient array for selected tab
	/**
	@param hdcmem		            : IN handle to device context
	@param prect		            : IN client area
	@param vertex		            : IN array of colors for gradient
	@returns void
	*/
	void	                        SetGradientToTabRibbon(HDC hdcmem, PRECT rectwin, LPTRIVERTEX vertex);

	/// Function is used to set gradient array for selected tab
	/**
	@param pitem		            : IN Tab item details pointer
	@param prect		            : IN client area
	@param vertex		            : IN array of colors for gradient
	@returns void
	*/
	void	                        SetGradientArrayOnFocus(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex);
    
	/// Function is used to set gradient array for mouse over tab
	/**
	@param pitem		            : IN Tab item details pointer
	@param prect		            : IN client area
	@param vertex		            : IN array of colors for gradient
	@returns void
	*/
	void	                        SetGradientArrayOnMouseOver(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex, wyBool isselected = wyFalse);
	
	/// Function is used to set gradient array for unselected tab
	/**
	@param pitem		            : IN Tab item details pointer
	@param prect		            : IN client area
	@param vertex		            : IN array of colors for gradient
	@returns void
	*/
	void	                        SetGradientArrayOnUnFocus(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex);
    
    /// Draw tab close button
    /**
    @param hdcmem		            : IN handle to device context
	@param prect		            : IN client area
    @return void
    */
	void                            DrawCloseButton(HDC hdcMem, PRECT prectclose, PPOINT pt);
    
    /// Draws the close button border
    /**
    @param hdcmem		            : IN handle to device context
	@param prect		            : IN client area
    @return void
    */
	void                            DrawCloseButtonBorder(HDC hdcMem, PRECT prectclose);
    
    
    /// Shows the tab context menu
    /**
    @param hwnd			            : IN tab control handle
    @param lParam		            : IN window procedure LPARAM
    @returns LRESULT, 1 if success, otherwise 0
    */

    ///Event handlers for WM_RBUTTONDOWN, WM_RBUTTONUP, WM_MBUTTONDOWN and WM_MBUTTONUP
    /**
    @returns LRESULT
    */
	LRESULT                         OnRButtonUp(HWND hwnd, LPARAM lParam);
    LRESULT                         OnRButtonDown(HWND hwnd, LPARAM lParam);
    LRESULT                         OnMButtonUp(HWND hwnd, LPARAM lParam);
    LRESULT                         OnMButtonDown(HWND hwnd, LPARAM lParam);
    
    /// Sends a message to the user
    /**
    @param message		            : IN message to be send
    @returns wyInt32, depends on the message send
    */
    wyInt32                         SendTabMessage(wyInt32 message, LPNMCTC lpnmctc = NULL, WPARAM wparam = NULL);

	
	///Sets gradient when mouse move over tab.
	/**
	@param hdc			            : IN handle to device context
	@param pnt		                : IN point structure
	@return void
	*/
	void		                    SetMouseOverGradient(HDC hdc, PPOINT pnt);


	///Sets the Gradient to the tab
	/**
	@param hdcmem	                : IN handle to device context
	@param vertex	                : IN Pointer to an array of TRIVERTEX structures that each define a triangle vertex
	@return void
	*/
	void		                    SetGradient(HDC hdcmem, TRIVERTEX *vertex); 

	///On drag start
	/**
	@param pnt		                : IN point structure
	@param rectwin		            : IN rect structure
	@return void
	*/
	void                            OnDragStart(PPOINT pnt, PRECT rectwin);

    ///Get the realtime rect of a particular tab
    /**
    @param tabindex                 : IN index opf the tab
    @param prect                    : OUT tab rect
    @returns wyTrue on scucess else wyFalse
    */
    wyBool                          GetTabRect(wyInt32 tabindex, RECT* prect);

    ///Helper function which draws the contents using double buffering
    /**
    @param  hdc                     : IN device context
    @returns void
    */
    void                            DrawWithDoubleBuffering(HDC hdc);

    ///Helper function which draws the contents without double buffering
    /**
    @param  hdc                     : IN device context
    @returns void
    */
    void                            DrawWithoutDoubleBuffering(HDC hdc);

	
	///On End of drag operation
	/**
	@return void
	*/
	void		                    OnDragEnd();

    ///While dragging
    /**
    @param wparam                   : IN message parameter
    @param pnt                      : IN cursor position
    @returns wyTrue if the operation is inside the tab header, else wyFalse
    */
    wyBool                          OnDragMove(WPARAM wparam, PPOINT pnt);
    

    ///Function gets the tab drop target tab index
    /**
    @param pnt                      : IN cursor position
    @returns the tab id
    */
    wyInt32                         GetDragArrowTab(PPOINT pnt);
    
	//Draw drag tab image
	/**
	@param rect		                : IN rect structure
	@return void
	*/
	void                            DrawDragTabImage(PRECT rect);


	//Free the drag resouces
	/**
	@return void
	*/
	wyInt32                         FreeDragResources();

    ///Function draws the tabs
    /**
    @param hdc                      : IN device context
    @returns void
    */
    void                            DrawTabs(HDC hdc);

    void                            DrawTab(HDC hdc, wyInt32 tabindex, RECT* pdrawrect, RECT* porigrect, POINT* pt, RECT* pselectedrect);

    ///Function whenever scrolling is performed
    /**
    @param wparam                   : IN scroll control identifier
    @returns void
    */
    void                            OnHScroll(WPARAM wparam, LPARAM lparam);

    ///Function draws fixed length round rectangle tab
    /**
    @param hdc                      : IN device context
    @param prect                    : IN bounding rectangle
    @param tabindex                 : IN tab index
    @returns the size of the tab
    */
    wyInt32                         DrawFixedLengthTab(HDC hdc, RECT * prect, wyInt32 tabindex, wyBool ismouseovertab);

    ///Function draws varying length rectangular tab
    /**
    @param hdc                      : IN device context
    @param prect                    : IN bounding rectangle
    @param tabindex                 : IN tab index
    @returns the size of the tab
    */
    wyInt32                         DrawVaryingLengthTab(HDC hdc, RECT * prect, wyInt32 tabindex, wyBool ismouseovertab);

    ///function calculates the tab length
    /**
    @param hdc                      : IN device context
    @param index                    : IN index of the tab
    @returns size of the tab
    */
    wyInt32                         CalculateTabLength(HDC hdc, wyInt32 index);


    ///Event handler for drag operation timer
    /**
    @returns void
    */
    void                            OnDragScrollTimer();

    ///Draw the plus button
    /**
    @param hdc                      : IN device context
    @param prect                    : IN boudning rectangle
    @param pnt                      : IN cursor postion
    @returns void
    */
    void                            DrawPlusButton(HDC hdc, PRECT prect, PPOINT pnt);

    void                            DrawScrollButton(HDC hdc, wyInt32 id, wyInt32 x, PPOINT pnt);


    void                            DrawDragArrow(HDC hdc, wyInt32 left, wyInt32 top);

    static VOID CALLBACK            IconAnimateTimerProc(HWND hwnd, UINT message, UINT_PTR idevent, DWORD dwtime);

    void                            DrawNextAnimationIcon(wyInt32 tabindex);

    void                            SetDefaultColorInfo();

    void                            CreateResources();

    void                            CreateBottomLinePen();

	void							UpdateDropDownStructPosition(wyInt32 selectedtab, wyInt32 dropindex);
	void							UpdateDropDownNodePosforconnection(wyInt32 selectedtab, wyInt32 dropindex);
	void							FindAndUpdateTheDropDown(wyInt32 selectedtab,wyInt32 dropindex);

    /// Flag for tab width is fixed or not
	wyBool          m_isfixedlength;

	/// Flag for tab width is fixed or not
	wyBool          m_isdragenabled;

    ///Bounding window for drag operation
	HWND            m_hwnddragwin; 

    ///Minimum tab count to be retained for each control
    wyInt32         m_mintabcount;

    /// Tab control id
    wyInt32         m_id;

    /// Tab window proc
	CTBWNDPROC      lpTBWndProc;

    /// HNADLE to font
	HFONT           m_hfont;

	/// HANDLE to selected font
	HFONT           m_hselectedfont;

	/// Window HANDLE
	HWND            m_hwnd;

	/// Message pointer
	LPARAM          m_lparamdata;

	/// Tab item information
	LPCTCITEMEX       m_tabdet;

    HPEN            m_hpenhighlight;

	/// Handle to a active pen 
	HPEN            m_hpenactivesep;

	/// Handle to a close button shade pen (red)
	HPEN            m_hpenclosebutton;

	///grey color for tab
	HPEN		    m_hpengrey;

	/// Tab count 
	wyInt32         m_tabs;

	/// Selected Tab
	wyInt32         m_selectedtab;

	///previously selected tab needed for tab deletion of tab other than selected tab
	wyInt32         m_prevtab;

	/// Starting Tab
	wyInt32         m_starttab;

	/// Ending Table
	wyInt32         m_endtab;

    ///Wheter the dragging operation is in progress
	wyBool		    m_isdragging;

	/// Minimum width possible for a tab
	wyInt32         m_minimumtabwidth;

	/// Tab height
	wyInt32         m_tabheight;


	///Close button flag
	wyBool          m_extflagclose;
	
	/// Size if the tab
	SIZE            m_size;

	//image list that contains drag image
	HIMAGELIST	    m_himl;

	//default colors fot tab control
	wyInt32		    m_blue;
	wyInt32		    m_green;
	wyInt32		    m_red;

	//track the mouse movement
    POINT           m_mouseprevpt;

    //whether the drag image is created
    wyBool          m_isdragmagepresent;

    ///the drag hotspot
    POINT           m_draghotspot;

    ///is scrolling required
    wyBool          m_isscroll;

    ///current scroll position
    wyInt32         m_scrollpos;

    ///maximum scroll position
    wyInt32         m_maxscrollpos;

    ///left most and right most visible tabs
    wyInt32         m_leftmostvisibletab;
    wyInt32         m_rightmostvisibletab;

    ///previous mouse over tab
    wyInt32         m_lastmouseovertab;

    ///whether to animate the tab behavior.... not yet implemented
    wyBool          m_isanimate;

    ///whether to have a plus button next to tabs
    wyBool          m_isplussign;

    ///Whether the mouse is over clos, used for optimizing paint operation
    wyBool          m_ismouseoverclose;

    ///tab index where mouse is down on close rectangle
    wyInt32         m_closebuttondowntab;

    ///The drop target tab index
    wyInt32         m_dragarrowtab;

    ///Whether to draw only tab header
    wyBool          m_isdrawonlytabheader;

    ///R/M button down tab
    wyInt32         m_rbuttondowntab;
    wyInt32         m_mbuttondowntab;    

    HIMAGELIST      m_hiconlist;

    HWND            m_hwndprevfocus;

    HPEN            m_hpenbottomline;

    TABCOLORINFO    m_colorinfo;

    wyInt32         m_tabcontroldown;
    wyInt32         m_overtabcontrol;

    HDC             m_hdcantialias;

    wyBool          m_istabchanging;
};

    
/// Registers the Custom Tab Control
/**
@returns class atom that uniquely identifies the class being registered else 0
*/
ATOM		        InitCustomTab();

/// Creates the CustomTab control
/**
@param hwndparent   : IN Parent window HANDLE
@param x            : IN Specifies the initial horizontal position of the window
@param y            : IN Specifies the initial vertical position of the window
@param width        : IN Specifies the width, in device units, of the window
@param height       : IN Specifies the height, in device units, of the window
@param lpgvwndproc  : IN Tab window procedure
@param lparam       : IN Message parameter
*/
HWND		        CreateCustomTab(HWND, wyInt32, wyInt32, wyInt32, wyInt32, CTBWNDPROC, LPARAM, LPARAM style = 0);

/// Create a tooltip associated with tab
/**
@param hwndtab      : IN Tab Window Handle
@returns the handle of tooltip
*/
VOID		        CreateCustomTabTooltip(HWND hwndtab);
	

/// Creates custom tab with extended window style
/**
@param hwndparent   : IN Parent window HANDLE
@param x            : IN Specifies the initial horizontal position of the window
@param y            : IN Specifies the initial vertical position of the window
@param width        : IN Specifies the width, in device units, of the window
@param height       : IN Specifies the height, in device units, of the window
@param lpgvwndproc  : IN Grid window procedure
@param styles       : IN Style
@param lparam       : IN Long message parameter
*/
HWND		        CreateCustomTabEx(HWND, wyInt32, wyInt32, wyInt32, wyInt32, CTBWNDPROC, DWORD, LPARAM);

/// Sets the class pointer associated with the window
/**
@param hwnd         : IN Tab Window Handle
@param ccp          : IN Custom tab pointer
@returns the previous value on success else returns 0
*/
wyInt32		        SetCustTabCtrlData(HWND, CCustTab*);

 /// Gets the pointer associated with the window
/**
@param hwnd         : IN tab Window Handle
@returns the pointer on success else 0       
*/
CCustTab*	        GetCustTabCtrlData(HWND);


    
/*Interface to the TabControl.*/

/// Function to insert custom grid tab item
/**
@returns position of the tab
*/
wyInt32		        CustomTab_InsertItem(HWND , wyInt32, LPCTCITEM);

/// Function to get the currently focus tab
/**
@param hwnd			: IN Window HANDLE
@returns index of the tab selected
*/
wyInt32             CustomTab_GetCurSel(HWND hwnd);

/// Function to get the visible tab count
/**
@param hwnd			: IN Window HANDLE
@returns wyTrue on success else wyFalse
*/
wyInt32		        CustomTab_GetVisibleTabCount(HWND hwnd);


/// Function to get back the number of tabs
/**
@param hwnd			: IN Window HANDLE
@returns the number of tab
*/
wyInt32		        CustomTab_GetItemCount(HWND hwnd);

/// Function to get back the minimum width of tab
/**
@param hwnd			: IN Window HANDLE
@param cx			: IN New width 
@returns old width of the tab
*/
wyInt32		        CustomTab_SetMinTabWidth(HWND hwnd , wyInt32 cx);

/// Function to delete an item from the tab
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be deleted
@returns wyTrue on success else wyFalse
*/
wyBool  	        CustomTab_DeleteItem(HWND hwnd , wyInt32 index,wyBool isdelrequired=wyFalse);


/// Function to delete all item from the tab
/**
@param hwnd			: IN Window HANDLE
@returns wyTrue on success else wyFalse
*/
wyBool              CustomTab_RemoveAllItem(HWND hwnd);

/// Function to set the current selection in the tab
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be deleted
@returns wyTrue on success else wyFalse
*/
wyBool              CustomTab_SetCurSel(HWND hwnd, wyInt32 index, WPARAM wparam = NULL);

/// Function is used to retrieve a particular item
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be deleted
@param pitem		: IN Tab item details pointer
@returns wyTrue on success else wyFalse
*/
wyBool              CustomTab_GetItem(HWND hwnd, wyInt32 index , LPCTCITEM pitem);

/// Function is used to Change a particular item
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be deleted
@param pitem		: IN Tab item details pointer
@returns wyTrue on success else wyFalse
*/
wyBool              CustomTab_SetItem(HWND hwnd, wyInt32 index, LPCTCITEM pitem);

/// Function is used to change color of a particular tab item
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be deleted
@param pitem		: IN Tab item details pointer
@returns wyTrue on success else wyFalse
*/
wyBool		        CustomTab_SetItemColor(HWND hwnd, wyInt32 index , LPCTCITEM pitem);

/// Function is used to Set the close button flag
/**
@param hwnd			: IN Window  HANDLE
@param val			: IN Closable ?
@param mintabcount  : IN minimum tab count to be retained, supply -1 not to alter
@returns void
*/
void                CustomTab_SetClosable(HWND hwnd, wyBool val, wyInt32 mintabcount = -1, wyBool ispaint = wyTrue);

/// Function is used to set a particular item visible
/**
@param hwnd			: IN Window HANDLE
@param index		: IN Index of the tab to be made visible
@returns wyTrue on success else wyFalse
*/
wyBool              CustomTab_EnsureVisible(HWND hwnd, wyInt32 index, wyBool isanimate = wyTrue);

/// Function to get the log data values from the tab
/**
@param hwnd			: IN Window HANDLE
@returns long message
*/
LPARAM		        CustomTab_GetLongData(HWND hwnd);

/// Function to get the log data values from the tab
/**
@param hwnd			: IN Window HANDLE
@param tabcount		: IN Number of tabs
@param lparam		: IN Long message pointer
@returns long message parameter
*/
LPARAM		        CustomTab_SetItemLongValue(HWND hwnd, wyInt32 tabcount, LPARAM lparam);

/// Function to Check whether the context menu is enabled or not
/**
@param hwnd			: IN Window HANDLE
@param index        : IN tab index to check
@returns wyBool, wyTrue if context menu is enabled, otherwise wyFalse
*/
wyBool		        CustomTab_IsMenuEnabled(HWND hwnd, wyInt32 index);

//// Function to Check whether the tab is of fixed width or not
/**
@param hwnd			: IN Window HANDLE
@param isfixedlength: IN tab is of fixed length
@returns void
*/
void                CustomTab_IsFixedLength(HWND hwnd, wyBool isfixedlength = wyFalse);

//// Function to Check whether the tab can be dragged or not
/**
@param hwnd			: IN Window HANDLE
@param hwnddragwin	: IN Drag area Window HANDLE
@param image        : IN image of arrow between tabs
@param isdragenable : IN tab is draggable or not
@returns void
*/
void                CustomTab_EnableDrag(HWND hwnd, HWND hwnddragwin,  wyBool isdragenable);

/// Function enable/disable the bufferd drawing, this will affect every tab controls
/**
@param enable       : IN enalbe/disable
@returns void
*/
void                CustomTab_SetBufferedDrawing(wyBool enable);

///Enable/Disalbe the plus button
/**
@params hwnd        : IN Window HANDLE
@params enable      : IN enable/disable
*/
void                CustomTab_EnableAddButton(HWND hwnd, wyBool enable);

void	            CustomTab_BlockEvents(HWND hwnd, wyBool isblock);

void                CustomTab_SetIconList(HWND hwnd, HIMAGELIST hiconlist);

wyBool              CustomTab_StartIconAnimation(HWND hwnd, wyInt32 tabindex, wyInt32 delay = 100);

wyBool              CustomTab_StopIconAnimation(HWND hwnd, wyInt32 tabindex);

wyInt32             CustomTab_GetTabHeight(HWND hwnd);

void                CustomTab_SetColorInfo(HWND hwnd, LPTABCOLORINFO pcolorinfo);

void                CustomTab_GetColorInfo(HWND hwnd, LPTABCOLORINFO pcolorinfo);

void                CustomTab_SetPaintTimer(HWND hwnd);



const wyChar*			CustomTab_GetTitle(HWND hwnd, wyInt32 index, wyString* title);
const wyChar*			CustomTab_GetTooltip(HWND hwnd, wyInt32 index, wyString* Tooltip);
#endif
