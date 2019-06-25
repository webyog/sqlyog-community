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

/*********************************************

Author: Vishal P.R

*********************************************/

#ifndef _WYTHEME_H_
#define _WYTHEME_H_

#include "wyString.h"
#include "tinyxml2.h"
#include "CustTab.h"

///Brush constants
#define BRUSH_MDICLIENT             1
#define BRUSH_FRAMEWINDOW           2
#define BRUSH_MENU                  3
#define BRUSH_HSPLITTER             4
#define BRUSH_VSPLITTER             5
#define BRUSH_TOOLBAR               7
#define BRUSH_MENUBAR               8
#define BRUSH_BTNFACE               9
#define BRUSH_MDICHILD              10
#define BRUSH_MAINTOOLBAR           11

///Pen constants
#define PEN_HSPLITTER               1
#define PEN_VSPLITTER               2

///Colors constants
#define COLORS_CONNTAB              1
#define COLORS_QUERYTAB             2
#define COLORS_RESTAB               3
#define COLORS_TABLETAB             4

///Dual color constants
#define DUAL_COLOR_TOOLBAR          1
#define DUAL_COLOR_MENU             2
#define DUAL_COLOR_MENUTEXT         3
#define DUAL_COLOR_SELMENUTEXT      4    
#define DUAL_COLOR_MENUBAR          5
#define DUAL_COLOR_MENUBARTEXT      6
#define DUAL_COLOR_SELMENUBARTEXT   7 
#define DUAL_COLOR_FRAMEWINDOWTEXT  8
#define DUAL_COLOR_TOOLBARTEXT      9
#define DUAL_COLOR_CONNTABTEXT      10
#define DUAL_COLOR_QUERYTABTEXT     11
#define DUAL_COLOR_RESTABTEXT       12
#define DUAL_COLOR_TABLETABTEXT     13
#define DUAL_COLOR_MAINTOOLBAR      14
#define DUAL_COLOR_MAINTOOLBARTEXT  15
#define DUAL_COLOR_MAINTOOLSELECTED 16
#define DUAL_COLOR_TOOLBARSELECTED  17
#define DUAL_COLOR_MENUBARSELECTED  18
#define DUAL_COLOR_MENUSELECTED     19
#define DUAL_COLOR_MDICHILDTEXT     20

///Hyperlink color constants
#define LINK_COLOR_FRAMEWINDOW      1
#define LINK_COLOR_MDICHILD         2


///Modes for GetSetThemeInfo
#define SET_THEME                   1
#define GET_THEME                   2
#define PROBE_THEME                 3

///Theme types
#define NO_THEME                    0
#define RESOURCE_THEME              1
#define FILE_THEME                  2

//Structure representing dual color
typedef struct dual_color
{
    ///Flag tells which member is valid
    wyInt32  m_flag; 

    //First color
    COLORREF m_color1;

    //Second color
    COLORREF m_color2;
}DUALCOLOR, *LPDUALCOLOR;

///Theme info structure
typedef struct theme_info
{
    ///Theme type
    wyInt32     m_type;

    ///Theme name
    wyString    m_name;

    ///The filename/resource identifier for the theme
    wyString    m_filename;
}THEMEINFO, *LPTHEMEINFO;

///The class that wraps up the entire library operations.
class wyTheme
{
    public:
        ///Static function to allocate the memory for the object
        /**
        @returns wyTrue on success else wyFalse
        */
        static wyBool               Allocate();

        ///Static function to release the memory for the object
        /**
        @returns void
        */
        static void                 Release();

        ///A generic function to subclass the edit controls
        /**
        @returns void
        */
        static void                 SubclassControls();

        ///Measure menu item height and width
        /**
        @param lpms                 : IN measure item sturct pointer
        @param hmenu                : IN handle to the menu
        @param ispopup              : IN whether the menu is a popup menu
        @returns wyTrue on success else wyFalse
        */
        static wyBool               MeasureMenuItem(LPMEASUREITEMSTRUCT lpms, HMENU hmenu, wyBool ispopup = wyTrue);

        ///Function draw the menu item
        /**
        @param lpds                 : IN menu item drawing sturct
        @param hmenubar             : IN handle to the menu bar that contains the menu, if any
        @returns wyTrue on success else wyFalse
        */
        static wyBool               DrawMenuItem(LPDRAWITEMSTRUCT lpds, HMENU hmenubar = NULL);

        ///Function to set a menu items for owner drawing
        /**
        @param hmenu                : IN handle to the menu
        @param startitemindex       : IN start menu item index for which the owner drawing should be enabled
        @param ignoreitemcount      : IN the number of menu items to be ignored in the end for owner drawing
        @param themecolorindex      : IN menu background color
        @param applytosubmenu       : IN whether to apply for submenus also, if any
        @returns void
        */
        static void                 SetMenuItemOwnerDraw(HMENU hmenu, wyInt32 startitemindex = 0, wyInt32 ignoreitemcount = 0, wyInt32 themecolorindex = BRUSH_MENU, wyBool applytosubmenu = wyTrue);

        ///Function to draw toolbar
        /**
        @param lparam               : IN LPARAM in WM_NOTIFY
        @param ismaintoolbar        : IN is it the main toolbar
        @returns -1 if the drawing is not done/cannot be done/the notification is not about toolbar drawing, else the apropriate return code. The caller should continue processing if the return value is -1
        */
        static wyInt32              DrawToolBar(LPARAM lparam, wyBool ismaintoolbar = wyFalse);

        ///Function to initialize the resources
        /**
        @param wyTrue on success else wyFalse
        */
        static wyBool               Init();

        ///Function to free the resources
        /**
        @returns void
        */
        static void                 FreeResources();

        ///Function to get the brush handle for the index specified
        /**
        @param index                : IN  the brush index for which the handle to be returned
        @param phbrush              : OUT the pointer to the handle to the brush 
        @returns wyTrue on success else wyFalse
        */
        static wyBool               GetBrush(wyInt32 index, HBRUSH* phbrush);

        ///Function to get the pen handle for the index specified
        /**
        @param index                : IN  the pen index for which the handle to be returned
        @param phpen                : OUT the pointer to the handle to the pen 
        @returns wyTrue on success else wyFalse
        */
        static wyBool               GetPen(wyInt32 index, HPEN* phpen);

        ///Function to get the tab colorsfor the index specified
        /**
        @param index                : IN  the tab color index
        @param pcolorinfo           : OUT the pointer to the tab color info
        @returns wyTrue on success else wyFalse
        */
        static wyBool               GetTabColors(wyInt32 index, TABCOLORINFO* pcolorinfo);

        ///Function to get the dual color for the index specified
        /**
        @param index                : IN  dual color index
        @param pdualcolor           : OUT pointer to the dual color struct
        @returns wyTrue on success else wyFalse
        */
        static wyBool               GetDualColor(wyInt32 index, DUALCOLOR* pdualcolor);

        ///Function to get the hyperlink color for the index specified
        /**
        @param index                : IN  hyperlink  color index
        @param plinkcolor           : OUT pointer to the hyperlink color struct
        @returns wyTrue on success else wyFalse
        */
        static wyBool               GetLinkColor(wyInt32 index, COLORREF* plinkcolor);

        ///Function to set the gradient in the rectangle given
        /**
        @param hdc                  : IN handle to the device context
        @param prect                : IN rectnagle to be filled
        @param pdualcol             : IN dual color to be used to set the gradient
        @param pframecolor          : IN the color to be used to frame the rectangle
        @returns void
        */
        static void                 SetGradient(HDC hdc, RECT* prect, DUALCOLOR* pdualcol, COLORREF* pframecolor = NULL);

        ///Function to draw the status bar part
        /**
        @param lpds                 : IN draw item struct
        @param str                  : IN text to be drawn
        @returns void
        */
        static void                 DrawStatusBarPart(LPDRAWITEMSTRUCT lpds, const wyWChar* str);

        ///Function gets the available themes in an array. Caller should free the array using FreeThemes
        /**
        @param pthemeinfo           : OUT pointer to the pointer to the theme array
        @returns the number of themes
        */
        static wyInt32              GetThemes(LPTHEMEINFO* pthemeinfo);

        ///Function to free the themes array 
        /**
        @param pthemeinfo           : IN pointer to the theme array
        @param count                : IN number of themes in the array
        @returns void
        */
        static void                 FreeThemes(LPTHEMEINFO pthemeinfo, wyInt32 count);

        ///Get the current theme info, use it for read purpose only
        /**
        @returns the active theme info
        */
        static const LPTHEMEINFO    GetActiveThemeInfo();

        ///Function to draw the button caption using the dual color given
        /**
        @param lparam               : IN LPARAM of WM_NOTIFY message
        @param pdc                  : IN pointer to the dual color info
        @returns -1 if the drawing is not done/cannot be done/the notification is not about button drawing, else the apropriate return code. The caller should ignore the return value if it is -1
        */
        static wyInt32              PaintButtonCaption(LPARAM lparam, LPDUALCOLOR pdc);

        ///Function checks the system menu is enabled or not
        /**
        @param hwnd                 : IN handle to the window for whcih the system menu should be checked against. if it is NULL, this will return the system menu property in the theme
        @returns wyTrue if system menu is enabled, else wyFalse
        */
        static wyBool               IsSysmenuEnabled(HWND hwnd = NULL);

        ///Function to get/set theme info
        /**
        @param mode                 : IN can be GET_THEME or SET_THEME or PROBE_THEME
        @param pthemeinfo           : IN the theme info to be get/set. 
        @param pthemedir            : IN the directory to be searched for file themes
        @return wyTrue on success else wyFalse
        */
        static wyBool               GetSetThemeInfo(wyInt32 mode, LPTHEMEINFO pthemeinfo = NULL, wyString* pthemedir = NULL);

        ///Function to apply theme in SQLyog
        /**
        @returns void
        */
        static void                 ApplyTheme();

        ///Function to initialize theme
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      InitTheme();

        ///Function to get the brush handle for the index specified
        /**
        @param index                : IN  the brush index for which the handle to be returned
        @param phbrush              : OUT the pointer to the handle to the brush 
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetThemeBrush(wyInt32 index, HBRUSH* phbrush);

        ///Function to get the pen handle for the index specified
        /**
        @param index                : IN  the pen index for which the handle to be returned
        @param phpen                : OUT the pointer to the handle to the pen 
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetThemePen(wyInt32 index, HPEN* phpen);
        
        ///Function to get the tab colorsfor the index specified
        /**
        @param index                : IN  the tab color index
        @param pcolorinfo           : OUT the pointer to the tab color info
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetThemeTabColors(wyInt32 index, LPTABCOLORINFO pcolorinfo);

        ///Function to get the dual color for the index specified
        /**
        @param index                : IN  dual color index
        @param pdualcolor           : OUT pointer to the dual color struct
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetThemeDualColor(wyInt32 index, DUALCOLOR* pdualcolor);

        ///Function to get the hyperlink color for the index specified
        /**
        @param index                : IN  hyperlink  color index
        @param plinkcolor           : OUT pointer to the hyperlink color struct
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetThemeLinkColor(wyInt32 index, COLORREF* plinkcolor);

        ///The one and only instance of the class
        static wyTheme*             m_theme;

        ///Static class to hold the button face brush
        static HBRUSH               m_hbrushbuttonface;


    private:
        ///Private constructor; Done this to prevent accidential creation of multiple instances of the class from outside
                                    wyTheme();

        //Priate destructor
                                    ~wyTheme();
        
        ///Function to set the theme directory
        /**
        @param pstr                 : IN theme directory
        @returns void
        */
        static void                 SetThemeDir(wyString* pstr);
        
        ///Helper function to create bitmap brush
        /**
        @param filename             : IN name/id of the bitmap file/resource to be used to create the brush
        @returns handle to the brush on success else NULL
        */
        HBRUSH                      CreateBitMapBrush(const wyChar* filename);

        ///Function to load the theme
        /**
        @param pthemedir            : IN  theme directory if the theme type is FILE_THEME
        @param pthemeinfo           : OUT the theme info loaded
        @returns the root node of theme xml on success else NULL
        */
        static tinyxml2::XMLDocument*       LoadTheme(wyString* pthemedir, LPTHEMEINFO pthemeinfo);

        ///Function to create the brush for the element
        /**
        @param pele                 : IN xml element
        @param element              : IN child element name
        @returns the brush handle on success else NULL
        */
        HBRUSH                      CreateBgBrush(tinyxml2::XMLElement* pele, const wyChar* element);

        ///Function converts color encoded in hex to COLORREF 
        /**
        @param color                : IN color in hex
        @returns the COLORREF
        */
        COLORREF                    GetColorFromHex(wyInt32 color);

        ///Function to allocate the tab colors
        /**
        @param pele                 : IN  xml element
        @param element              : IN  child element name
        @param pcolorinfo           : OUT tab color info
        @returns void
        */
        void                        CreateTabColors(tinyxml2::XMLElement* pele, const wyChar* element, LPTABCOLORINFO pcolorinfo);

        ///Function to set the dual color
        /**
        @param pele                 : IN  xml element
        @param element              : IN  child element name
        @param subele               : IN  sub element of child element
        @param attrib1              : IN  attribute to be scanned for the first color
        @param attrib2              : IN  attribute to be scanned for the second color
        @param pdulacolor           : OUT dual color info
        @returns void
        */
        void                        SetDualColors(tinyxml2::XMLElement* pele, const wyChar* element, const wyChar* subele, const wyChar* attrib1, const wyChar* attrib2, DUALCOLOR* pdulacolor);

        ///Function to get the color info
        /**
        @param pele                 : IN  xml element
        @param element              : IN  child element name
        @param attribute            : IN  attribute to be scanned for the color
        @param pcolor               : IN  color
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetColorInfo(tinyxml2::XMLElement* pele, const wyChar* element, const wyChar* attribute, COLORREF* pcolor);

        ///Superclassed procedure for edit controls
        /**
        @param hwnd                 : IN window handle
        @param message              : IN message
        @param wparam               : IN message word parameter
        @param lparam               : IN message long parameter
        @return LRESULT
        */
        static LRESULT	CALLBACK    SuperClassedEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Enumercation callbacks to update tab colors and resize the windows
        /**
        @param hwnd                 : IN window handle
        @param lparam               : IN LPARAM
        */
        static BOOL CALLBACK        EnumChildProcUpdateTabs(HWND hwnd, LPARAM lparam);
        static BOOL CALLBACK        EnumChildProcResize(HWND hwnd, LPARAM lparam);

        ///Original edit proc
        static WNDPROC  m_origeditclassproc;

        ///Theme directory
        wyString        m_themedir;

        ///Theme info
        THEMEINFO       m_themeinfo;
        
        ///MDI brush
        HBRUSH          m_hmdibrush;

        ///Connection tab color
        TABCOLORINFO    m_conncolorinfo;

        ///Query tab color
        TABCOLORINFO    m_qtcolorinfo;

        ///Tab management color
        TABCOLORINFO    m_tmcolorinfo;

        ///Table tab color
        TABCOLORINFO    m_tabletabcolorinfo;

        ///Frame window brush
        HBRUSH          m_hframebrush;

        ///Mneu background brush
        HBRUSH          m_hmenubrush[2];

        ///Splitter bursh
        HBRUSH          m_hhsplitterbrush;

        ///Vertical splitter brush
        HBRUSH          m_hversplitterbrush;

        ///Toolbar background brush
        HBRUSH          m_htoolbarbrush;

        ///Main toolbar brush
        HBRUSH          m_hmaintoolbarbrush;

        ///MDI child brush
        HBRUSH          m_hmdichildbrush;

        ///Menu highlight color
        DUALCOLOR       m_colormenuhighlight[2];

        ///Toolbar highlight color
        DUALCOLOR       m_colortoolbarhighlight;

        ///Main toolbar highlight color
        DUALCOLOR       m_colormaintoolbarhighlight;

        ///Menu text color
        DUALCOLOR       m_colormenutext[2];

        ///Selected menu text color
        DUALCOLOR       m_colorselmenutext[2];

        ///Select menu
        DUALCOLOR       m_colormenuselected;

        ///Menu bar selected color
        DUALCOLOR       m_colormenubarselected;

        ///Toolbar selected color
        DUALCOLOR       m_colortoolbarselected;

        ///Main toolbar selected color
        DUALCOLOR       m_colormaintoolbarselected;
        
        ///Framewindow text color
        DUALCOLOR       m_colorframewindowtext;

        ///Toolbar text color
        DUALCOLOR       m_colortoolbartext;

        ///Main toolbar text color
        DUALCOLOR       m_colormaintoolbartext;

        ///Connection tab text color
        DUALCOLOR       m_colorconntabtext;

        ///Query tab text color
        DUALCOLOR       m_colorquerytabtext;

        ///Result tab text color
        DUALCOLOR       m_colorrestabtext;

        ///Table tab text color
        DUALCOLOR       m_colortabletabtext;

        ///Horizontal splitter pen
        HPEN            m_hhsplitterpen;

        ///Vertical splitter pen
        HPEN            m_hvsplitterpen;

        ///Is sysmenu property set
        wyBool          m_issysmenu;

        ///MDI link color
        DUALCOLOR       m_mdichildlink;

        DUALCOLOR       m_colormdichildtext;

        ///Frame window link color
        DUALCOLOR       m_framewindowlink;
};

#endif