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
        
#define	GVN_BEGINLABELEDIT	        	0x00001
#define GVN_ENDLABELEDIT	        	0x00002
#define	GVN_BEGINADDNEWROW	        	0x00003	
#define	GVN_ENDADDNEWROW	        	0x00004
#define	GVN_RBUTTONDOWN		        	0x00005
#define	GVN_BEGINCHANGEROW	        	0x00006
#define GVN_LBUTTONDBLCLK	        	0x00007
#define	GVN_ENDCHANGEROW	        	0x00008
#define	GVN_BUTTONCLICK		        	0x00009
#define GVN_DESTROY			        	0x00010
#define	GVN_SELCHANGING		        	0x00011
#define	GVN_BEGINROWCHECK	        	0x00012
#define	GVN_GETOWNERTEXT	        	0x00013
#define	GVN_ISWHITEBKGND				0x00014
#define GVN_DELETEROW					0x00015
#define	GVN_GETDISPINFO					0x00016
#define GVN_COLUMNCLICK					0x00017
#define	GVN_COLUMNDRAW					0x00018
#define GVN_SETOWNERCOLDATA				0x00019
#define	GVN_KILLFOCUS					0x00020	
#define	GVN_DRAWROWCHECK				0x00021
#define	GVN_CHECKBOXCLICK				0x00022
#define	GVN_ISROWCHECKSELECTED			0x00023
#define	GVN_PASTECLIPBOARD              0x00024
#define	GVN_PASTECLIPBOARDBEGIN         0x00025
#define	GVN_GETDISPLENINFO              0x00026
#define	GVN_GETDISPINFODATA             0x00027
#define	GVN_SETSCROLLBARINFO			0x00028
#define	GVN_FINISHENDLABELEDIT          0x00029
#define	GVN_NCSHOWLIST                  0x00028
#define GVN_SELECTALLCLICK              0x00051
#define GVN_DRAWSELECTALL               0x00052
#define GVN_CANCELLABELEDIT				0x00049

#define GVN_LBUTTONMOUSEMOVE            0x00030
#define GVN_LBUTTONUP                   0x00031 
#define GVN_MOUSELEAVE                  0x00032
#define	GVN_PREVTABORDERITEM			0x00033
#define GVN_SPLITTERMOVE				0x00034
#define GVN_LBUTTONDOWN                 0x00050

#define	GVN_LBITEMCHANGED				0x00035
#define GVN_BROWSEBUTTONCLICK           0x00036
#define GVN_KEYDOWN                     0x00037
#define GVN_GETCOLUMNMARK               0x00038
#define	GVN_NEXTTABORDERITEM			0x00039
#define GVN_HELP                        0x00040
#define GVN_SYSKEYDOWN                  0x00041
#define GVN_ROWCHANGINGTO               0x00055
#define GVN_ROWCHANGED                  0x00056
#define GVN_BEGINROWDRAW                0x00057
#define GVN_GETCELLCOLOR                0x00058
#define GVN_VSCROLL                     0x00059
#define GVN_HSCROLL                     0x00060
#define GVN_MOUSEWHEEL                  0x00061
#define GVN_DRAWWATERMARK               0x00062
#define GVN_SETFOCUS                    0x00063

#define GVKEY_HOME                        36
#define GVKEY_END                         35
#define GVKEY_RIGHT                       39
#define GVKEY_LEFT                        37
#define GVKEY_UP                          38
#define GVKEY_DOWN                        40
#define HIGHLIGHTBORDERCOLOR            RGB(242, 149, 54)
#define HIGHLIGHTCOLOR                  RGB(244, 203, 123)
//#define ROWHIGHLIGHTCOLOR				RGB(226, 229, 238)
#define ROWHIGHLIGHTCOLOR				RGB(180, 204, 238)
#define ALTHIGHLIGHTCOLOR				RGB(244, 244, 244)

#ifndef _CUST_GRID_
#define _CUST_GRID_


/*! \struct tagGridViewColumn
    \brief Grid view column details
    \param  wyInt32		cx;                 // X co-ordinate
	\param  wyInt32		nListCount;         // Number of list
	\param  wyInt32		nElemSize;          // Element size
	\param  wyInt32		cchTextMax;         // Size of buffer
	\param  wyChar	    issource;           // Checks for owner
	\param  UINT	    fmt;                // Format
	\param  UINT	    mask;               // Type
    \param  wyBool	    uIsReadOnly;        // Checks for read only state
	\param  wyBool	    uIsButtonVis;       // Checks for visibility
	\param  void*	    pszList;            // List pointer
	\param  LPSTR      text;               // Text
	\param  LPSTR	    pszButtonText;      // Button text
	\param  LPSTR	    pszDefault;         // Default text
	\param  LPARAM	    lparam;             // Long message pointer
*/
typedef struct tagGridViewColumn
{

	wyInt32		cx;
	wyInt32		nListCount;
	wyInt32		nElemSize;
	wyInt32		cchTextMax;
	wyChar	    issource;
	UINT	    fmt;
	UINT	    mask;
	wyBool	    uIsReadOnly;
	wyBool	    uIsButtonVis;
	VOID*	    pszList;
	LPSTR	    text;
	LPSTR	    pszButtonText;
	LPSTR	    pszDefault;
	LPARAM	    lparam;
    LPARAM      mark;
    wyInt32     marktype;

}GVCOLUMN, *PGVCOLUMN;

/*! \struct tagGridViewColumnNode
    \brief Grid view column node details
    \param  HWND						hwndCombo;          // Combo box window HANDLE
	\param  WNDPROC						wpOrigComboProc;    // Window procedure for combo
	\param  wyWChar*					pszDefault;         // Default text
	\param  tagGridViewColumn			pColumn;            // Grid view column pointer
	\param  tagGridViewColumnNode		*pNext;             // Next node in column
*/
typedef struct tagGridViewColumnNode
{

	HWND							hwndCombo;
	WNDPROC							wpOrigComboProc;
	wyChar*							pszDefault;
	tagGridViewColumn				pColumn;
	tagGridViewColumnNode			*pNext;
	wyBool							isshow;

}GVCOLNODE, *PGVCOLNODE;


/*! \struct tagGridScrollBarInfo
	\breif  Used to set the scroll information of the grid
	\param  HWND					m_hwnd;            // Grid handle

	\param  wyInt32					m_vertscrollrange; // Vartical scroll range
	\param  wyInt32					m_horzscrollrange; // Horizontal scroll range

	\param  wyInt32					m_vertscrollpos;   // Vertical scroll position
	\param  wyInt32					m_horzscrollpos;   // Horizontal scroll position

	\param  wyBool					m_vertscroll;      // Is vertical scroll bar is visible or not
	\param  wyBool					m_horzscroll;      // Is horizontal scroll bar is visible or not
*/

typedef struct tagGridScrollBarInfo
{
	HWND							m_hwnd;

	wyInt32							m_vertscrollrange;
	wyInt32							m_horzscrollrange;

	wyInt32							m_vertscrollpos;
	wyInt32							m_horzscrollpos;

	wyBool							m_vertscroll;
	wyBool							m_horzscroll;

}GVSCROLLBARINFO, *PGVSCROLLBARINFO;

/*! \struct tagGridViewRowNode
    \brief  Grid view row node details
    \param  LPARAM				    	lparam;           // Long pointer parameter
	\param  wyBool						excheck;          // Whether checked
	\param  tagGridViewColumnNode		*pColumn;         // Column pointer
	\param  tagGridViewRowNode			*pNext;           // Next row pointer
*/
typedef struct tagGridViewRowNode
{

	LPARAM							lparam;
    wyInt32                         rowcx;
	wyBool							excheck;
	tagGridViewColumnNode			*pColumn;
	tagGridViewRowNode				*pNext;

}GVROWNODE, *PGVROWNODE;

/*! \struct tagGridViewRowData
    \brief  Grid view row data
    \param  wyInt32			nCol;                       // Column number
	\param  wyChar**		pszData;                    // Data
	\param  wyBool*	    	source;                     // Source
*/
typedef	struct tagGridViewRowData
{
	wyInt32			nCol;
	wyChar**		pszData;
	wyBool*		    source;
}ROWDATA, *PROWDATA;


/*! \struct tagGVDISPINFO
    \brief  Grid view display information
    \param  wyInt32		    nCol;                       // Column number
	\param  wyInt32			nRow;                       // Row number
	\param  wyInt32			cchTextMax;                 // Size of buffer
	\param  LPSTR		    text;                       // Text
	\param  LPSTR			pszButtonText;              // Button Text
*/
typedef struct tagGVDISPINFO
{
	wyInt32				nCol;
	wyInt32				nRow;
	wyInt32				cchTextMax;
	LPSTR		    	text;
	LPSTR			    pszButtonText;
} GVDISPINFO, *PGVDISPINFO;

typedef struct tagGVCOLUMNMARKINFO
{
    void*       mark;
    wyInt32     marktype;
} GVCOLUMNMARKINFO, *PGVCOLUMNMARKINFO;

/*! \struct tagGVROWCHECKINFO
    \brief  Grid view row check
    \param  bool		checkorstar                     // Check or star
	\param  bool		ischecked                       // Checked or not
*/
typedef struct tagGVROWCHECKINFO
{	
	bool		checkorstar;
	bool		ischecked;
} GVROWCHECKINFO, *PROWCHECKINFO;

typedef struct tagGVINITIALBUTTONINFO   
{
    wyInt32 checkstate;
	wyBool	sendmessage;
}GVINITIALBUTTONINFO, *PINITIALBUTTONINFO;


typedef struct tagGVCELLCOLORINFO
{	
	wyInt32     nRow;
	wyInt32     nCol;
    COLORREF    color;
}GVCELLCOLORINFO, *PGVCELLCOLORINFO;

typedef struct tagGVWATTERMARK
{	
    RECT        rect;
	HDC         hdc;
}GVWATTERMARK, *PGVWATTERMARK;


/// Call back window procedure
/**
@param hwnd     : Window HANDLE
@param message  : Window message
@param wparam   : Unsigned int message parameter
@param wparam   : Long message parameter
@returns long pointer
*/
typedef LRESULT(CALLBACK* GVWNDPROC)(HWND, UINT, WPARAM, LPARAM);


class CCustGrid
{
public:

	CCustGrid(HWND h);
	~CCustGrid();

	/// Inserts the column in the grid view
    /**
    @param pgvcol       : IN Column details
    @param returns number of columns in the grid
    */
	wyInt32     Insert_Column(PGVCOLUMN pgvcol);

    /// Decides the rows per page
    /**
    @returns the number of rows per page
    */
	wyInt32		RowPerPage();

    /// Determines the column perpage display
    /**
    @returns the number of columns that can be displyed per page
    */
    wyInt32     ColumnPerPage();

    /// Inserts the row in between two rows
    /**
    @param row          : IN number of rows 
    @returns row index
    */
	wyInt32		InsertRowInBetween(wyInt32 row);

    /// Gets the item text length
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns length of the text
    */
	wyInt32		GetItemTextLength(wyInt32 row, wyInt32 col);

    /// Gets the item text
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param text         : OUT Text in the column
    @returns the length of the text
    */
	wyInt32		GetItemText(wyInt32 row, wyInt32 col, wyWChar * text);

    /// Gets the column title
    /**
    @param col          : IN Column number
    @param buffer       : OUT column title
    @returns length of the text
    */
	wyInt32		GetColumnTitle(wyInt32 col, wyWChar *buffer);

    /// Sets text in list.
    /**
    @param col          : IN Column number
    @param buffer       : IN Text to insert
    @returns index
    */
	wyInt32		InsertTextInList(wyInt32 col, wyWChar *buffer);

    /// Finds a text in list.
    /**
    @param col          : IN Column number
    @param buffer       : IN Text to find
    @returns index
    */
	wyInt32		FindTextInList(wyInt32 col, wyWChar *buffer);
    
    /// Sets the check state of as row
    /**
    @param row          : IN Row number
    @returns old state
    */
	wyInt32		SetRowCheckState(wyInt32 row, wyBool ustate);

    /// Finds a text in column.
    /**
    @param col          : IN Column number
    @param itempreced   : IN From which column to start seraching, if it is -1 it will start from first position
    @param buffer       : IN Text to find
    @returns index
    */
    wyInt32		FindTextInColumn(wyInt32 col, wyInt32 itempreced, wyChar *sztext);


    /// Gets the check state of as row
    /**
    @param row          : IN Row number
    @returns wyTrue on checked else wyFalse
    */
	wyInt32		GetRowCheckState(wyInt32 row);

    /// Helper function to draw individual cell 
    /**
    @param hdcmem       : IN Handler to device
    @param row          : IN Row number
    @param col          : IN Column number
    @param topcolstruct : IN Top column struct
    @param pgvcol       : IN Pointer to grid view column
    @param hbrbkgnd     : IN background brush handle
    @param rect         : IN coordinates of a grid window's client area
    @param rectgrey     : IN coordinates of a grayed window's client area
    @param rowrect      : IN coordinates of a row window's client area
    @returns wyTrue on success
    */
	wyBool      DrawCell(HDC hdcmem, wyInt32 row, wyInt32 col, PGVCOLNODE topcolstruct, PGVCOLUMN pgvcol,
						    HBRUSH hbrbkgnd, RECT * rect, RECT * greyrect, RECT * rowrect);

    /// Function takes the cell boundary in RECT parameter and draws
    /**
    @param hdc          : IN Handle to device
    @param rect         : IN coordinates of a button window's client area
    @param text         : IN Text to draw
    @param buttontext   : IN Button text to draw
    @param iscombodrop  : IN Checks for combo cell or not
    @returns wyTrue on success else wyFalse
    */
	wyBool		DrawButtonWithText(HDC hdc, RECT * rect, const wyChar * text, const wyChar * buttontext, 
                                    wyBool iscombodrop = wyFalse);

    /// Sets the owner data
    /**
    @param val          : IN Owner data or not
    @returns void
    */
	VOID		SetOwnerData(wyBool val);

    /// Checks whether owner data
    /**
    @returns wyTrue if owner data else wyFalse
    */
	wyBool		GetOwnerData();

    /// Sets the default value for column
    /**
    @param col          : IN Column number
    @param buf          : IN Default text
    @returns wyTrue on success else wyFalse
    */
	wyBool		SetColumnDefault(wyInt32 col, const wyChar * buf);

    /// Gets the true or false value
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success else wyFalse
    */
	wyBool		GetBoolValue(wyInt32 row, wyInt32 col);

    /// Sets the current selection
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param sendmsg      : Checks for send message option
    @returns wyTrue on success
    */
	wyBool		SetCurSelection(wyInt32 row, wyInt32 col, wyBool sendmsg = wyTrue);

    /// Sets the initial row number
    /**
    @param row          : IN Row number
    @returns wyTrue on success
    */
	wyBool		SetInitRow(wyInt32 row);

    /// Sets the initial column
    /**
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		SetInitCol(wyInt32 col);

    /// Applies change 
    /**
    @returns wyTrue on success
    */
	wyBool		ApplyChanges();

    /// gets the item from point
    /**
    @param lppnt        : IN pointer to a particular point
    @param prow         : IN Row number
    @param pcol         : IN Column number
    @returns wyTrue on success
    */
	wyBool		GetItemFromPoint(LPPOINT lppnt, wyInt32 * prow, wyInt32 * pcol);

    /// Sets the item text
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param text         : IN Text to set
    @returns wyTrue on success
    */
	wyBool		SetSubItemText(wyInt32 row, wyInt32 col, const wyChar * text);

    /// Sets the button item text
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param text         : IN Text to set
    @returns wyTrue on success
    */
	wyBool		SetSubButtonItemText(wyInt32 row, wyInt32 col, wyWChar * text);

    /// Sets the text at a given place
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param text         : IN pointer to text top set
    @returns wyTrue on success
    */
	wyBool		SetTextPointer(wyInt32 row, wyInt32 col, wyChar * text);

    /// Deletes the given row   
    /**
    @param row          : IN Row number
    @returns wyTrue on success
    */
	wyBool		DeleteRow(wyInt32 row);

    /// Deletes everything.
    /**
    @returns wyTrue on success
    */
    wyBool      DeleteAll();

    /// Deletes all rows
    /**
    @returns wyTrue on success
    */
	wyBool		DeleteAllRow(wyBool ispaint);

    /// Deletes all column
    /**
    @returns wyTrue on success
    */
	wyBool		DeleteAllColumns();

    /// Shows the grid
    /**
    @param isvisible: IN visible or not 
    @returns void
    */
    void        ShowGrid(wyBool isvisible);

    /// Deletes the content of the list in a column
    /**
    @param          : IN Column number
    @returns index
    */
	wyInt32     DeleteListContent(wyInt32 ncol);

    /// Copies the data from clipboard
    /**
    @returns wyTrue on success else wyFalse
    */
	wyBool		CopyDataFromClipboard();

    /// Copies the data to clipboard
    /**
    @returns wyTrue on success else wyFalse
    */
	wyBool		CopyDataToClipboard();

    /// Sets the maximum number of row count
    /**
    @param count        : IN Max number
    @returns void
    */
	VOID		SetMaxRowCount(LONG count);

    /// Sets row long value data 
    /**
    @param row          : IN Row number
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	wyBool		SetRowLongData(wyInt32 row, LPARAM lparam);

    /// Sets the column to readonly mode
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param ustate       : IN Column state (wyTrye/wyFalse)
    @returns wyTrue on success else wyFalse
    */
	wyBool		SetColumnReadOnly(wyInt32 row, wyInt32 col, wyBool uState);

    /// Sets the buttons to be visible
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param ustate       : IN Button state(wyBool/wyFalse)
    @returns wyTrue on success
    */
	wyBool		SetButtonVis(wyInt32 row, wyInt32 col, wyBool ustate);

    /// Sets the column width
    /**
    @param col          : IN Column number
    @param cx           : IN Width
    @returns wyTrue on success
    */
	wyBool		SetColumnWidth(wyInt32 col, UINT cx);

    /// Function to ensure that a cell is visible in the screen.
    /**
    @param row          : IN row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		EnsureVisible(wyInt32 row, wyInt32 col, wyBool init = wyFalse);

    /// Set the font for the grid
    /**
    @param lf           : IN font pointer
    @returns void
    */
	VOID		SetGridFont(PLOGFONT lf);

    /// Gets the window handle
    /**
    @returns the window handle
    */
	HWND		GetHwnd(){ return m_hwnd; };

    /// Sets the extended style
    /**
    @param style        : IN New style
    @returns old style
    */
	LONG		SetExtendedStyle(LONG style);

    /// Gets the extended style
    /**
    @returns the extended style
    */
	LONG		GetExtendedStyle(){  return m_exstyle; }; 

    /// Gets the total number of rows
    /**
    @returns the number of rows
    */
	LONG		GetRowCount(){ return m_row; };

    /// Gets the total number of columns
    /**
    @returns the number of column
    */
	LONG		GetColumnCount(){ return m_col; };

    /// Gets the current selected row
    /**
    @returns the selected row
    */
	LONG		GetCurSelRow(){ return m_curselrow; }

    /// Gets the current selected column
    /**
    @returns the selected column
    */
	LONG		GetCurSelCol(){ return m_curselcol; }

    /// Sets the current row
    /**
    @param col          : IN Row number
    @param sendmsg      : IN Checks for send message to grid(wyTrue/wyFalse), if wyTrue it will send a GVN_BEGINCHANGEROW notification message.
    */
	wyBool		SetCurSelRow(LONG nRow, wyBool sendmsg = wyTrue);

    /// Sets the current column
    /**
    @param col          : IN Column number
    @param sendmsg      : IN Checks for send message to grid(wyTrue/wyFalse), if wyTrue it will send a GVN_BEGINCHANGEROW notification message.
    */
	wyBool		SetCurSelCol(LONG nCol, wyBool sendmsg = wyTrue);

    /// Gets the current selected cell
    /**
	@param				: IN Consider focused grid or not
    @returns the row and column values
    */
	LONG		GetCurSelection(wyBool Focus);

    /// Inserts a row in the table
    /**
    @returns the number of rows
    */
	LONG		Insert_Row();

    /// Gets the initial row
    /**
    @returns the initial row
    */
	LONG		GetInitRow(){ return m_initrow; };

    /// Gets the initial column 
    /**
    @returns the initial column
    */
    LONG		GetInitCol(){ return m_initcol; };

    /// Function to work with default values of column.
    /**
    @param col          : IN Column number
    @returns the default text
    */
	wyChar		*GetColumnDefault(wyInt32 col);

    /// Gets the font 
    /**
    @returns the column font
    */
	HFONT		GetColumnFont(){ return m_htopfont; };

    /// Gets the font 
    /**
    @returns the row font
    */
	HFONT		GetRowFont(){ return m_hfont; };

    /// Gets the long data
    /**
    @returns long message pointer
    */
	LPARAM		GetLongData();

    /// Gets the long data from row
    /**
    @param row          : IN Row number
    @returns long message parameter
    */
	LPARAM		GetRowLongData(wyInt32 row);

    /// Sets the long item value for a specific cell
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param lparam       : IN Specifies the replacement value
    @returns long message parameter
    */
	LPARAM		SetItemLongValue(wyInt32 row, wyInt32 col, LPARAM lparam);

    /// Gets the long item values associated with a specific cell
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns long message parameter
    */
	LPARAM		GetItemLongValue(wyInt32 row, wyInt32 col);

    /// Sets the long values for a specific column
    /**
    @param col          : IN Column number
    @param lparam       : IN  Specifies the replacement value
    @returns 
    */
	LPARAM		SetColumnLongValue(wyInt32 col, LPARAM lparam);

    /// Gets the long values for a specific column
    /**
    @param col          : IN Column number
    @returns long message parameter
    */
	LPARAM		GetColumnLongValue(wyInt32 col);

	/// Destroys all the child windows
	/**
	@returns void
	*/
	void		DestroyResources();

    /// Gets  a particular row data
    /**
    @param prowdata     : IN Pointer to row data
    @returns pointer to row data
    */
	PROWDATA	GetItemRow(wyInt32 row);

    /// Deallocates memory allocated for row data
    /**
    @param prowdata     : IN Pointer to row data
    @returns pointer to row data
    */
	PROWDATA	FreeRowData(PROWDATA prowdata);

    /// Check whether the text is in utf8 or not.
    /**
    @param buff         : OUT buffer to store the codepage
    @returns codepage
    */
	LONG		GetCodePage(wyChar *buff);

    /// Callback window procedure for custom grid window
    /**
    @param hwnd         : IN Window handle
    @param msg          : IN Messages
    @param wparam       : IN Message parameter
    @param lparam       : IN Message parameter
    @returns long pointer to window
    */
	static	LRESULT	CALLBACK CustomGridWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /// Set the max column width 
    /**
    */
    void        SetMaxWidth(wyInt32 width);

    /// Callback window procedure for splitter window
    /**
    @param hwnd         : IN Window handle
    @param msg          : IN Messages
    @param wparam       : IN Message parameter
    @param lparam       : IN Message parameter
    @returns long pointer to window
    */
	static	LRESULT	CALLBACK SplitterWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /// Callback window procedure for edit window
    /**
    @param hwnd         : IN Window handle
    @param msg          : IN Messages
    @param wparam       : IN Message parameter
    @param lparam       : IN Message parameter
    @returns long pointer to window
    */
	static	LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /// Callback window procedure for edit window
    /**
    @param hwnd         : IN Window handle
    @param msg          : IN Messages
    @param wparam       : IN Message parameter
    @param lparam       : IN Message parameter
    @returns long pointer to window
    */
    static	LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /// Callback window procedure for combo box
    /**
    @param hwnd         : IN Window handle
    @param msg          : IN Messages
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @returns long pointer to window
    */
	static	LRESULT	CALLBACK ComboWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	
    static	LRESULT	CALLBACK BrowseButtonWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	
    /// Sets the flag to flip the grid
    /**
    @param enable       : IN Enable or disable
    */
    void        SetFlip(wyBool enable);

    /// Enable or disable the FORM view mode for the grid
    /**
    @param flag         : IN Enable/Disable
    */
    void        SetFormMode(wyBool flag);

    /// Jumps to the next record
    /**
    @param hwnd         : IN Window HANDLE
    */
    void        NextRecord(HWND hwnd);

    /// Jumps to the prev record
    /**
    @param hwnd         : IN Window HANDLE
    */
    void        PrevRecord(HWND hwnd);

    /// Jumps to the Last record
    /**
    @param hwnd         : IN Window HANDLE
    */
    void        LastRecord(HWND hwnd);

    /// Jumps to the First record
    /**
    @param hwnd         : IN Window HANDLE
    */
    void        FirstRecord(HWND hwnd);

    /// Get the max column width of the cell
    /**
    returns the max column width
    */
    wyInt32     GetMaxColumnWidth();

    /// Genetrates the signals like mouse leave etc
    void        GetMouseMovement();

    /// Gets the column header clicked
    /**
    @param pnt          : IN Mouse points
    */
    wyInt32     GetRowHeader(POINT *pnt);

    /// Gets the row and column when a drag and drop accures on the grid cell
    /**
    @param lparam       : IN Long message pointer
    @param row          : OUT Row number
    @param col          : OUT Column number
    */
    wyInt32     GetRowColumn(LPARAM lparam, wyInt32 *row, wyInt32 *col);

	/// Gets the rectangle for the given cell
   /**
   @param row          : IN Row number
   @param col          : IN Column number
   @param lprect       : OUT Pointer to cell rectangle
   @returns void
   */
	void		GetSubItemRect(wyInt32 row, wyInt32 col, LPRECT lprect);

	///Sets the Gradient
	/**
	@param hdcmem       : IN Handle to device
	vertex				: IN rect for setting the gradient
	@return void
	*/
	void		SetGradient(HDC hdcmem, TRIVERTEX *vertex, wyBool vertical = wyTrue);
		
	/// Gets the Column width
	/**
	@param col			: IN Column number
	@param width		: OUT Column Width
	@returns void
	*/
	wyBool		GetColumnWidth(wyInt32 col, wyUInt32 * width);

	//Show/Hide the Scroll bar of grid
	/**
	@scrollid	 : IN SB_VERT or SB_HORZ
	@status      : IN wyTrue to show scroll bar, wyFalse to hide
	@returns void
	*/
	//VOID		ShowOrHideScollBar(wyInt32 scrollid, wyBool status);

	///Function sets the column(s) to hide or show again
	/**
	@param column : IN Index of the column 
	@param ishide : IN wyTrue to hide, wyFalse to show
	@return VOID
	*/
	VOID		ShowOrHideColumn(wyInt32 column, wyBool ishide);

    ///Function gets the active control text length, if editing is in progress
	/**
	@return length of the text
    */
    wyInt32     GetActiveControlTextLength();

    ///Function gets the active control text, if editing is in progress
	/**
    @param text   : OUT buffer to store the text
	@param length : IN length of the text
	@return void
    */
    void        GetActiveControlText(wyWChar* text, wyInt32 length);

    ///Function sets the column mask
	/**
	@param col    : IN Index of the column 
	@param mask   : IN column mask
	@return wyTrue on success else wyFalse
	*/
    wyBool      SetColumnMask(wyInt32 col, wyInt32 mask);

    ///Function gets the column mask
	/**
	@param col    : IN Index of the column 
	@return column mask
	*/
    wyInt32     GetColumnMask(wyInt32 col);

    /// This function is called when esc key press event occurs
    /**
    @returns wyTrue
    */
	LRESULT		ProcessEscPress();

    wyBool      IsEditing();

    GVWNDPROC   GetGridProc();

    wyInt32  GetSelAllState();

    void SetSelAllState(wyInt32 state =  -1);


private:

    /// This function is called when paint message is received
    /**
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnPaint(WPARAM wparam, LPARAM lparam);
	

    /// This function is called when vertical scroll message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyFalse on success else wyTrue
    */
	LRESULT		OnVScroll(HWND hwndscroll, WPARAM wparam, LPARAM lparam);

    /// This function is called when horizontal scroll message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyFalse on success else wyTrue
    */
	LRESULT		OnHScroll(HWND hwndscroll, WPARAM wparam, LPARAM lparam);

    /// This function is called when window command message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnWMCommand(WPARAM wparam, LPARAM lparam);

    LRESULT		OnWMSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// This function is called when window notify message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnWMNotify(WPARAM wparam, LPARAM lparam);

    /// This function is called when left button down message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnLButtonDown(WPARAM wparam, LPARAM lparam);
	//////////////////////////////////////////////////
	void        GetSelectColumn(POINT pnt);


	///////////////////////////////////////////////

    /// This function is called when left button up message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnLButtonUp(WPARAM wparam, LPARAM lparam);

    /// This function is called when set focus message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnSetFocus(WPARAM wparam, LPARAM lparam);

    /// This function is called when right button down message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns window procedure pointer
    */
	LRESULT		OnRButtonDown(WPARAM wparam, LPARAM lparam);

    /// This function is called when kill focus message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnKillFocus(WPARAM wparam, LPARAM lparam);

    /// This function is called when left button double click message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnLButtonDblClk(WPARAM wparam, LPARAM lparam);

    /// This function handles the double click
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
    LRESULT		HandleDblClick(WPARAM wparam, LPARAM lparam);

    /// This function is called when left button up message is received
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnMouseMove	(WPARAM wparam, LPARAM lparam);

    /// This function is called when mousewheel event occurs
    /**
    @param hwndscroll   : IN Scroll handle
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnMouseWheel(WPARAM wparam, LPARAM lparam);

    /// This function is called when splitter window mouse move event occurs
    /**
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnSplitterMouseMove(WPARAM wparam, LPARAM lparam);

    /// This function is called when splitter window button up event occurs
    /**
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue
    */
	LRESULT		OnSplitterButtonUp(WPARAM wparam, LPARAM lparam);

    /// This function is called when non client destroy event occurs
    /**
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue
    */
	LRESULT		OnNCDestroy(WPARAM wparam, LPARAM lparam);

    /// This function is called when key down event occurs
    /**
    @param hwnd         : IN Grid Window Handle
    @param wparam       : Unsigned int message parameter
    @param lparam       : Long message parameter
    @returns wyTrue on success
    */
	LRESULT		OnKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// This function is called when tab key press event occurs
    /**
    @returns wyTrue
    */
	LRESULT		ProcessTabPress();

    /// This function is called when a key is pressed
    /**
    @param wparam       : IN Unsigned int message parameter
    @param lparam       : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	LRESULT		OnWMChar(WPARAM wparam, LPARAM lparam);

    /// This function is called the window is created
    /**
    @param lparam       : IN Long message parameter
    @returns wyFalse
    */
	LRESULT		OnCreate(LPARAM lparam);

    /// Get the dialog code
    /**
    @param lparam       : IN Long message parameter
    @returns DLGC_WANTMESSAGE on success else wyFalse 
    */
	LRESULT		OnGetDLGCode(LPARAM lparam);

    /// Sets the long message parameter 
    /**
    @param lparam       : IN long message parameter
    @returns wyTrue
    */
	LRESULT		SetLongData(LPARAM lparam);

    void        OnCancelMultipleSelList();

	/// Helper functions for painting the grid columns.
    /**
    @param hdcmem       : IN Handle to device
    @param rect         : IN Pointer to cell rectangle
    @param rectwin      : IN pointer to grid rectangle
    @param totcy        : IN Total points for x axis
    @returns number of columns
    */
	wyInt32			DrawTopColumns(HDC hdcmem, PRECT rect, PRECT rectwin, wyUInt32 *totcx);

	/// Sows or hides the scroll bar
	/**
	@param totcx        : IN Total points for x axis
	@param totcy        : IN Total points for y axis
	@param rectwin		: IN The grid window rectangle co-ordinates
	*/
	void			ShowHideScrollBar(wyUInt32 totcx, wyUInt32 totcy, RECT rectwin);

	/// Sets the limits for the scroll bars
	/**
	@param nrow			: IN Numbers of row left to display
    @param ncol			: IN Numbers of col left to display
	*/
	void			SetScrollBarLimits(wyUInt32 nrow, wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy);

	//Sets the scrollbar limits for Normal Grid(Non flip)
	/**
	@param nrow			: IN Numbers of row left to display
    @param ncol			: IN Numbers of col left to display
	@param rectwin      : IN grid rect
	@param totcx        : IN Total points for x axis
	@param totcy        : IN Total points for y axis
	@return void
	*/	
	void			SetScrollBarLimitsNonFlipGrid(wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy);

	//Sets the scrollbar limits for Flip Grid
	/**
	@param nrow			: IN Numbers of row left to display
    @param ncol			: IN Numbers of col left to display
	@param rectwin      : IN grid rect
	@param totcx        : IN Total points for x axis
	@param totcy        : IN Total points for y axis
	@return void
	*/
	void			SetScrollBarLimitsFlipGrid(wyUInt32 nrow, wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy);

    /// Helper functions for painting the grid columns.
    /**
    @param hdcmem       : IN Memory handle
    @param rect         : IN Pointer to cell rectangle
    @param rectwin      : IN pointer to grid rectangle
    @param totcx        : IN Total points for x axis
    @param totcy        : IN Total points for y axis
    @returns number of rows
    */
	wyInt32			DrawRowColumns(HDC hdcmem, PRECT rect, PRECT rectwin, wyUInt32 *totcx, wyUInt32 *totcy);

    /// Gets the remaining rows to display for the flip mode
    /**
    */
    wyInt32         GetRemainingRows(PRECT rectwin);

	/// Function recalculates the columns of each column when a column is inserted in between.
    /**
    @param pgvc     : IN column node value
    @returns wyTrue on success
    */
	wyBool		RowRecalculate(PGVCOLNODE pgvc);

    /// Starts the column editing in the grid view
    /**
    @param row      : IN Row number
    @param col      : IN Column number
    @returns wyTrue on success
    */
	wyBool		BeginColumnEdit(wyInt32 row, wyInt32 col);

    /// Starts the column editing in the grid view
    /**
    @param row      : IN Row number
    @param col      : IN Column number
    @returns wyTrue on success
    */
	wyBool		BeginListEdit(wyInt32 row, wyInt32 col);

    /// Starts the multiple label editing process
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		BeginMultipleLableEdit(wyInt32 row, wyInt32 col);

    /// Starts the label editing process
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		BeginLabelEdit(wyInt32 row, wyInt32 col);

    /// Takes care of the ending of label editing
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		EndLabelEdit(wyInt32 row, wyInt32 col);

    /// Takes care of the ending of combo selection
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		EndComboEdit(wyInt32 row, wyInt32 col);

    /// Toggles between 0 and 1
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success else wyFalse
    */
	wyBool		ToggleBoolValue(wyInt32 row, wyInt32 col);

    /// Takes care of the double click on a cell
    /**
    @param row          : IN Row number
    @param col          : IN Column number
	@param onchardown   : IN Char event, default wyFalse
    @returns wyTrue on success
    */
	wyBool		ProcessButtonClick(wyInt32 row, wyInt32 col, wyBool onchardown = wyFalse);

    /// Checks for the tick mark for columns
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success else wyFalse
    */
	wyBool		ColTrueOrFalse(wyInt32 row, wyInt32 col);

    /// Checks if the row is empty
    /**
    @param nrow         : IN Row number
    @returns wyTrue on success else returns wyFalse
    */
	wyBool		IsRowEmpty(wyInt32 nrow);


    /// Initialtes the row width
    /**
    */
    void        SetRowWidth();

    /// Inserts a new row when the key is pressed
    /**
    @param hwnd         : IN Window handle
    @returns wyTrue on success else wyFalse
    */
	wyBool		InsertNewRowOnKeyPress(HWND hwnd);

    /// Processes the view of the column of type list
    /**
    @param nrow         : IN Row number
    @param ncol         : IN Column number
    @returns wyTrue on success
    */
	wyBool		ProcessOnListFocus(wyInt32 nrow, wyInt32 ncol);

    /// Calculates the numbers of rows to move on page up
    /**    
    @returns wyTrue on success else wyFalse
    */
	wyBool		CalculatePageUp(wyBool isonpgupkey = wyFalse);

    /// Calculates the numbers of rows to move on page down
    /**    
    @returns wyTrue on success else wyFalse
    */
	wyBool		CalculatePageDown(wyBool isonpgdwnkey = wyFalse);

    /// Sets the fonts 
    /**    
    @returns wyTrue on success
    */
	wyBool		CreateFonts();

    /// Function to ensure that the last selected column is visible in the screen.
    /**    
    @returns wyTrue on success
    */
	wyBool		ShowLastCol();

    /// Gets the original text of a particular cell
    /**
    @param row          : IN row number
    @param col          : IN Column number
    @returns wyTrue on success
    */
	wyBool		GetOriginalText(wyInt32 row, wyInt32 col);

    /// Gets the structure of a row
    /**
    @param row          : IN Row number
    @returns the row list top on success else null
    */
	PGVROWNODE	GetRowStruct(wyInt32 row);
    
    /// Gets the sub item structure
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns grid view column information
    */
	PGVCOLUMN	GetSubItemStruct(wyInt32 row, wyInt32 col);

    /// Gets the column node structure
    /**
    @param index        : IN Index
    @returns the column node pointer
    */
	PGVCOLNODE	GetColNodeStruct(wyInt32 index);

	/// Function to fill the COMBOBOX of a column
    /**
    @param psrc         : IN Source column
    @param ptarget      : IN Target column
    @param pnumeric     : IN Numeric or not
    @returns wyTrue on success
    */
	wyBool		FillComboBox(PGVCOLUMN psrc, PGVCOLNODE ptarget, wyBool pnumeric);

    /// Fills multiple lists
    /**
    @param psrc         : IN Source column
    @param ptarget      : IN Target column
    @returns wyTrue on success
    */
	wyBool		FillMultipleList(PGVCOLUMN psrc, PGVCOLNODE ptarget);

    /// handles the character pressed for EditProc
    /**
    @param hwnd         : IN Window handle
    @param message      : IN Window message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list box
    @param status       : OUT Whether the messaged is able to process there not
    @returns wyTrue on success else returns wyFalse
    */
    wyInt32     OnEditProcWmChar(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist, wyInt32 *status);

    /// Handles the key press event for EditProc
    /**
    @param hwnd         : IN Window handle
    @param message      : IN Window message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list box
    @param islistdrop   : IN Checks if the list box is dropped
    @param retval       : OUT Whether the messaged is able to process there not
    @returns wyTrue on success else returns wyFalse
    */
    wyInt32     OnEditProcWmKeyDown(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, 
                                wyInt32 islist, wyInt32 islistdrop, wyInt32 *retval);

    /// Handles the return key pressed for EditProc
    /**
    @returns void
    */
    void        OnEditProcVkReturn();

    /// Handles the up key pressed for EditProc
    /**
    @param hwnd         : IN window handle
    @param message      : IN Message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list
    @param islistdrop   : IN Checks if list box is dropped
    @returns wyTrue on success else wyFalse
    */
    wyInt32     OnEditProcVkUp(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist, 
                                wyInt32 islistdrop);

    /// Handles the down key pressed for EditProc
    /**
    @param hwnd         : IN window handle
    @param message      : IN Message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list
    @param islistdrop   : IN Checks if list box is dropped
    @returns wyTrue on success else wyFalse
    */
    wyInt32     OnEditProcVkDown(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist,
                                wyInt32 islistdrop);

    /// Handles the previous key pressed for EditProc
    /**
    @param message      : IN Message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list
    @returns wyTrue on success else wyFalse
    */
    wyInt32     OnEditProcVkPrior(wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist);

    /// Handles the next key pressed for EditProc
    /**
    @param message      : IN Message
    @param wparam       : IN Unsigned message parameter
    @param lparam       : IN Long message parameter
    @param islist       : IN Checks for list
    @returns wyTrue on success else wyFalse
    */
    wyInt32     OnEditProcVkNext(wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist);

    /// Column handler
    /**
    @param pgvnode      : IN Pointer to grid view node
    @param pnt          : IN Point
    @param rectwin      : IN Grid window rect
    @param x            : IN x coordinate
    @param count        : IN Column number
    @return wyTrue on success else wyFalse
    */
    wyInt32     HandleColumn(PGVROWNODE pgvrownode, PGVCOLNODE pgvnode, POINT *pnt, RECT *rectwin, wyInt32 x, wyInt32 count);

    /// Row handler
    /**
    @param pgvrownode       : IN Pointer to grid view row node
    @param count            : IN Number of rows
    @returns wyTrue on success else wyFalse
    */
    wyInt32     HandleRow(PGVROWNODE pgvrownode, wyInt32 count);

    /// Cell handler
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @returns wyTrue on success else wyFalse
    */
    wyInt32     HandleCell(wyInt32 row, wyInt32 col);

    /// Handles the non text cell
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param pnt          : IN Mouse point
    @param rect         : IN Cell rect
    @returns void
    */
    void        HandleNonTextCell(wyInt32 row, wyInt32 col, POINT *pnt, RECT *rect);

    /// Handles the cell selection
    /**
    @param row          : IN Row number
    @param col          : IN Column number
    @param recttemp     : IN Cell rect
    @param rectwin      : IN Grid window rect
    @returns void
    */
    void        HandleCellSelection(wyInt32 row, wyInt32 col, RECT *recttemp, RECT *rectwin);


    wyInt32        HandleInitialButton();

    /// Handles the home key pressed event
    /**
    @param ctrlpressed      : IN Ctrl key pressed or not
    @returns wyTrue on success else returns wyFalse
    */
    void        OnHomeKey(wyBool ctrlpressed);

    /// Handles the up key pressed event
    /**
    @returns wyTrue on success else returns wyFalse
    */
    wyBool      OnUpKey();

    /// Handles the down key pressed event
    /**
    @param rectwin      : IN The rectangular coordinates of the grid window
    @returns wyTrue on success else returns wyFalse
    */
    wyBool      OnDownKey(RECT *rectwin);

    /// Handles the left key pressed event
    /**
    @returns wyTrue on success else returns wyFalse
    */
    wyBool      OnLeftKey();

    /// Handles the space key pressed event
    /**
    @param ctrlpressed      : IN Ctrl key pressed or not
    returns void
    */
    void        OnEndKey(wyBool ctrlpressed);

    /// Handles the space key pressed event
    /**
    returns void
    */
    void        OnSpaceKey();

    /// Handles the right key pressed event
    /**
    @param rectwin      :IN The rectangular coordinates of the grid window
    @returns wyTrue on success else returns wyFalse
    */
    wyBool      OnRightKey(RECT *rectwin);

    /// Handles the delete key pressed event
    /**
    returns void
    */
    void        OnDelete();

    /// Inserts the column in the first
    /**
    @param pgvcol       : IN Pointer to grid column node
    @returns new column size
    */
    LONG        InsertAsFirstColumn(PGVCOLUMN pgvcol);

    /// Inserts column values in the end
    /**
    @param pgvcol       : IN Pointer to grid column node
    @returns new column size
    */
    LONG        InsertAsLastColumn(PGVCOLUMN pgvcol);

    /// Filling the list with values
    /**
    @param pgvcol       : IN Pointer to grid column node
    @param temp         : IN Temp pointer to grid column node
    @returns void
    */
    void        FillList(PGVCOLUMN pgvcol, PGVCOLNODE temp);

    /// Copies values
    /**
    @param pgvcol       : IN Pointer to grid column
    @param temp         : IN Temp pointer to grid column node
    @returns void
    */
    void        CopyValues(PGVCOLUMN pgvcol, PGVCOLNODE temp);

    /// Inserts values as first row 
    /**
    returns new row size
    */
    LONG        InsertAsFirstRow();

    /// Inserts values as last row 
    /**
    returns new row size
    */
    LONG        InsertAsLastRow();

    /// Sets the values for rows
    /**
    @param colnode      : IN Column node
    @param temp         : IN Temp node
    @returns void
    */
    void        SetRowValues(PGVCOLNODE colnode, PGVCOLNODE *temp);

    /// Deletes all column node
    /**
    @param pgvcolnode       : IN Grid column node
    @returns void
    */
    void        DeleteAllColumnNode(PGVCOLNODE pgvcolnode);

    /// Sets the owner's item text
    /**
    @param nrow     : IN Row number
    @param ncol     : IN Column number
    @param text     : IN Text to set
    @returns wyTrue on success
    */
    wyBool      SetSubItemOwnerText(wyInt32 nrow, wyInt32 ncol, const wyChar *text);

    /// Sets the non owner's item text
    /**
    @param nrow     : IN Row number
    @param ncol     : IN Column number
    @param text     : IN Text to set
    @returns wyTrue on success 
    */
    wyBool      SetSubItemNonOwnerText(wyInt32 nrow, wyInt32 ncol, const wyChar *text);

    /// Adds new column node
    /**
    @param pgvc     : IN Pointer to grid column
    @returns pointer to grid view column node
    */
    PGVCOLNODE  AddNewColNode(PGVCOLNODE pgvc);

    /// Post an end label msg
    /**
    @param nrow         : IN Row number
    @param ncol         : IN Column number
    @param temptext     : IN Text message
    @returns 1 on success else 0
    */
    LONG        PostEndLabelEditMsg(wyInt32 nrow, wyInt32 ncol, wyString &temptext);

    /// Gets the current cell value
    /**
    @param pgvcolnode   : IN Grid column node
    @param temptext     : OUT Text from the cell
    @returns void
    */
    void        GetCurrentCellValue(PGVCOLNODE pgvcolnode, wyString &temptext);

    /// Gets the system mouse scroll setting
    /**
    @returns number of lines
    */
    wyUInt32    GetLinesToScrollUserSetting();

    /// Get the max text length among all the columns
    /**
    @param pgvnode      : IN PGVCOLNODE pointer
    */
    wyUInt32    GetMaxTextWidth(PGVCOLNODE pgvnode);

    /// Get the max column width among all the columns
    /**
    @param pgvnode      : IN PGVCOLNODE pointer
    */
    wyUInt32    GetMaxWidth(PGVCOLNODE pgvnode);

    /// Draws the initial button on the left - top of the grid
    /**
    */
    void        DrawInitialButton(PGVCOLNODE pgvnode, HDC hdcmem, RECT *rect);

    /// Draws all the Columns of the table
    /**
    @param rect         : IN 
    @param rectwin      : IN Windows Rectangle
    @param hdcmem       : IN 
    @param totcx        : IN Total cx
    @param totcy        : IN Total cy
    */
    wyInt32     DrawColumns(RECT *rect, RECT *rectwin, HDC hdcmem, wyUInt32 *totcx, wyUInt32 *totcy);

    /// Draws all the rows of the table
    /**
    @param rect         : IN 
    @param rectwin      : IN Windows Rectangle
    @param hdcmem       : IN 
    @param totcx        : IN Total cx
    @param totcy        : IN Total cy
    @returns the number of rows yet to be displayed
    */
    wyInt32     DrawRows(RECT *rect, RECT *rectwin, HDC hdcmem, wyUInt32 *totcx, wyUInt32 *totcy, HFONT hfontold);


    /// Skips the column by one
    /// Used in cases where the init display row is other then the first column
    /**
    @param pgvnode      : IN/OUT PGVCOLNODE structure
    @param rectcolcount : IN Column rectangle
    @param rectwin      : IN Window Rectangle
    @param totcx        : IN/OUT Total x-axis reached
    @param ncol         : IN/OUT Number of column left to be displayed
    returns the the new skiped pointer to the structure
    */
    PGVCOLNODE        SkipColumns(PGVCOLNODE pgvnode, RECT *rectcolcount, PRECT rectwin, wyUInt32 *totcx, wyInt32 *ncol);

    /// Draws the different kind of buttons for the column
    /**
    @param hdcmem       : IN Device HANDLE
    @param pgvnode      : IN/OUT PGVCOLNODE structure
    @param rect         : IN/OUT Column rectangle
    @param totcx        : IN/OUT Total x-axis traversed
    @param totviscx     : IN Total visible x-axis
    @param rectwin		: IN grid rect
    */
    void        DrawButtonForColumn(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rect, wyUInt32 *totcx, wyInt32 *totviscx, PRECT rectwin, wyInt32 col);
    
    /// Draws Column icons when the sorting is enabled
    /**
    @param hdcmem       : IN Device HANDLE
    @param nncurcol     : IN/OUT Current column
    @param recttemp     : IN/OUT Temp rectangle
    */
    void        DrawColumnIcons(HDC hdcmem, wyInt32 *ncurcol, RECT *recttemp);

    /// Calculate the horizontal scroll bar
    /**
    @param pgvnode      : IN/OUT PGVCOLNODE structure
    @param rect         : IN/OUT Column rectangle
    @param rectwin      : IN Window rectangle
    @param totcx        : IN Total x-axis traversed
    @param ncol         : IN/OUT Number of columns remaining
    @param totviscx     : IN/OUT Total visible x-axis
    @param runviscount  : IN total visible counter
    */
    void        CalculateHorizontalScrollBar(PGVCOLNODE	pgvnode, PRECT rect, PRECT rectwin, wyUInt32 *totcx,
                                                wyInt32 *ncol, wyInt32 *totviscx, wyInt32 *runviscount);

    /// Draws the tetx on the column buttons
    /**
    @param hdcmem       : IN Device HANDLE
    @param pgvcol       : IN PGVCOLUMN structure
    @param recttemp     : IN Column rectangle
    @param ncurcol      : IN Current selected column
    */
    void        DrawColumnText(HDC hdcmem, PGVCOLUMN pgvcol, RECT recttemp, wyInt32 *ncurcol);

    /// Draws the button for either 'tick' marks or '*' for the rows
    /**
    @param hdcmem       : IN Device HANDLE
    @param recttemp     : IN Column rectangle
    @param rowcount     : IN Total number of rows
    */
    void        DrawRowButtons(HDC hdcmem, RECT *rect, RECT *recttemp, wyInt32 *rowcount);

    /// Draws the grey rectangle for the cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param greyrect     : IN The grey rectangle
    @param rect2        : IN 
    @param rcttemp      : IN Column rectangle
    */
    void        DrawGreyRect(HDC hdcmem, RECT *greyrect, RECT *rect2, RECT *recttemp);

    /// Draws the bool type of cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param rowrect      : IN Rectangle
    @param disp         : IN GVDISPINFO pointer
    @param row          : IN Row number
    @param col          : IN Column number
    */
    void        DrawCellBool(HDC hdcmem, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col);

    /// Draws the button type of cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param rowrect      : IN Rectangle
    @param disp         : IN GVDISPINFO pointer
    @param row          : IN Row number
    @param col          : IN Column number
    */
    void        DrawCellButton(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col);

    /// Draws the text button type of cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param rowrect      : IN Rectangle
    @param disp         : IN GVDISPINFO pointer
    @param row          : IN Row number
    @param col          : IN Column number
    */
    void        DrawCellTextButton(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col);

    /// Draws the drop down or list type of cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param rowrect      : IN Rectangle
    @param disp         : IN GVDISPINFO pointer
    @param row          : IN Row number
    @param col          : IN Column number
    */
    void        DrawCellDropOrList(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col);

    /// Draws other type of cell
    /**
    @param hdcmem       : IN Device HANDLE
    @param rowrect      : IN Rectangle
    @param disp         : IN GVDISPINFO pointer
    @param row          : IN Row number
    @param col          : IN Column number
    */
    void        DrawCellOther(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, PGVCOLNODE topcolstruct);
   

    /// Paints the current selected column with the highlighting color
    /**
    */
    void        PaintColumnHeader(HDC hdcmem, RECT *recttemp, wyInt32 *ncurcol);
	void        PaintRowHeader(HDC hdcmem, RECT *recttemp, wyInt32 *ncurrow);

	 /// Determines the column perpage display
    /**
    @returns the number of columns that can be displyed per page
    */
	wyInt32		ColumnVisibleCount();

    /// Old text in column
	wyWChar		*m_oldcoltext;

    /// Checks for editing
	wyBool		m_isediting;

    /// Is any editing done.
    wyBool      m_iscomboediting;		            	

    /// Checks for scrolling
	wyBool		m_isscrolling;

    /// Checks for scrolling
	wyBool		m_ownerdata;

    /// First click
    wyBool      m_first;

    /// The checked row
	wyChar		*m_rowcheck;	

    LONG        m_checkcount;

    /// Window HANDLE
	HWND		m_hwnd;
    
    /// Editor window HANDLE	
	HWND		m_hwndedit;

    /// Window splitter
    HWND        m_hwndsplitter;
	HWND		m_hwndcurcombo;

    /// Extended styles
	LONG		m_exstyle;								

    /// Number of rows
	LONG		m_row;

    /// Number of columns
    LONG        m_col;		

    /// Current row that is modified
	LONG		m_currowmodify;

    /// Initial row
	LONG		m_initrow;

    /// Initial column
    LONG        m_initcol;

    /// Current selected row
	LONG		m_curselrow;

    /// Current selected column
    LONG        m_curselcol;

    /// The captured column
    LONG        m_capturedcol;

    /// Number of visible columns
    LONG        m_visiblecols;

    /// Font
	HFONT		m_hfont;

    /// Italics Font for NULL Value
    HFONT       m_hItalicsFont;

    /// Title font
    HFONT       m_htopfont;

    /// Long message pointer
	LPARAM		m_lparamdata;

    /// Background color
	COLORREF	m_crbkgnd;

    /// holds information about scroll information
    GVSCROLLBARINFO m_scrollbarinfo;

    /// Original window procedure
	WNDPROC		m_wporigeditwndproc;
    WNDPROC		m_wporiglistwndproc;
    WNDPROC     m_wporigbrowsebuttonwndproc;

    /// Grid view window procedure
	GVWNDPROC	m_lpgvwndproc;

    /// Grid view column node
	PGVCOLNODE	m_pgvcurcolnode;

    /// Undo buffer
    wyChar        *m_undobuffer;

	// linked list to maintain the data.
	PGVCOLNODE	m_collist;

    /// Grid view row list
 	PGVROWNODE	m_rowlist;

    /// Grid view last row
    PGVROWNODE	m_rowlast;

    GVINITIALBUTTONINFO m_selallinfo;

    ///grid level longpointer 
    LPARAM      m_lparam;

    ///is grid is visible or not
    wyBool      m_isvisible;

    /// Flip grid is active or not
    wyBool      m_flip;

    /// Max possible width for a cell
    wyUInt32    m_maxwidth;

    LONG    m_hight;

    /// Max possible width for a cell
    DWORD    m_maxtextwidth;

    /// Whether form mode is active or not
    wyBool      m_form;

    /// Is the action selected from keyboard or not
    wyBool      m_iskeyboard;
    
	//V- scroll pos
	DWORD    m_vscrollpos;

	//V-scroll thumb width
	DWORD		m_vscrollthumwidth;

	//V-scroll max-range
	DWORD		m_vscrollmaxrange;

	//ROws to be skipped(to avoid of scolling all columns to left, for flip grid used
	DWORD		m_vskiprows;// for flip

	//H-scroll pos
	DWORD    m_hscrollpos;

	//H-sroll thumb width
	DWORD		m_hscrollthumwidth;

	//H-scroll max-range
	DWORD		m_hscrollmaxrange;

	//Gets the Mouse position on L_Button, for restricting the 'Sor icon' for column header only if L_BUTTONDOWN & L_BUTTONUP on same column header
	POINT		m_pointlbuttondown;

	//Number of postions for a step(line right/left)
	DWORD	m_hscrollnumpos;

	//Number of rows skipped to avoid get rid of grid becomes blank
	DWORD	m_hskipcolcount;

	//WIdth of grid row
	wyUInt32 m_tox;

    LONG	m_prescrollinitrow; 

    //handle to the browse button
    HWND    m_hwndbrowsebutton;

    //whether the browse button is pressed
    wyBool  m_ispressed;
};

#define GV_MARKTYPE_ICON        1
#define GV_MARKTYPE_TEXT        2

#define	GVIF_LEFT				0x0001
#define	GVIF_RIGHT				0x0002	
#define	GVIF_CENTER				0x0004

#define GVIF_TEXT               1
#define GVIF_LIST				2
#define GVIF_BOOL				4
#define GVIF_BUTTON             8
#define GVIF_DROPDOWNLIST		16
#define GVIF_DROPDOWNMULTIPLE	32	
#define GVIF_TEXTBUTTON			64
#define GVIF_DROPDOWNNLIST      128
#define GVIF_BROWSEBUTTON       256
#define GVIF_COLUMNMARK         512
#define GVIF_CELLCOLOR          1024

#define	GV_EX_ROWCHECKBOX		1
#define	GV_EX_OWNERDATA			2
#define GV_EX_FULLROWSELECT     4
#define GV_EX_WATERMARK         8
#define GV_EX_NO_VER_BORDER     16
#define GV_EX_NO_HOR_BORDER     32
#define GV_EX_STRETCH_LAST_COL  64

#define	GV_DEFWIDTH				25
#define	GV_DEFHIGHT				18
#define	GV_SCROLLPOS			1
#define	GV_LISTBOX				100

#define GV_BOOLVALUE			wyChar*
#define	GV_TRUE					"true"
#define	GV_FALSE				"false"

#define	GV_ERR					LB_ERR
#define	GV_CHEKCED				1
#define	GV_CHECKED				GV_CHEKCED
#define	GV_UNCHECKED			0

#define	GV_UNKNOWN				-1
#define	GV_ASCENDING			2
#define GV_DESCENDING			3

#define	GVM_SETLONGDATA			    WM_USER+1
#define	GVM_PROCESSTAB			    WM_USER+2
#define	GVM_BUTTONCLICK		    	WM_USER+3
#define	GVM_STARTLISTEDIT		    WM_USER+4
#define	GVM_SETFOCUS			    WM_USER+5
#define	GVM_SETFOCUSONACTIVECTRL    WM_USER+6


#define	DEFBUTTONSIZE		    128


/// Registers the Custom Grid Control
/**
@returns class atom that uniquely identifies the class being registered else 0
*/
ATOM		InitCustomGrid();

/// Creates the CustomGrid control
/**
@param hwndparent   : IN Parent window HANDLE
@param x            : IN Specifies the initial horizontal position of the window
@param y            : IN Specifies the initial vertical position of the window
@param width        : IN Specifies the width, in device units, of the window
@param height       : IN Specifies the height, in device units, of the window
@param lpgvwndproc  : IN Grid window procedure
@param lparam       : IN Message parameter
@param isvisible    : IN VisiBle or not, default wyTrue
@param flip         : IN flip grid or not
*/
HWND		CreateCustomGrid(HWND hwndparent, wyInt32 x, wyInt32 y, wyInt32 width, wyInt32 height, 
                             GVWNDPROC lpgvwndproc, LPARAM lparam, wyBool isvisible = wyTrue, wyBool flip = wyFalse);

/// Creates custom grid with extended window style
/**
@param hwndparent   : IN Parent window HANDLE
@param x            : IN Specifies the initial horizontal position of the window
@param y            : IN Specifies the initial vertical position of the window
@param width        : IN Specifies the width, in device units, of the window
@param height       : IN Specifies the height, in device units, of the window
@param lpgvwndproc  : IN Grid window procedure
@param styles       : IN Style
@param lparam       : IN Long message parameter
@param isvisible    : IN VisiBle or not, default wyTrue
@param flip         : IN flip grid or not
*/
HWND		CreateCustomGridEx(HWND hwndparent, wyInt32 x, wyInt32 y, wyInt32 width, wyInt32 height, 
                               GVWNDPROC lpgvwndproc, DWORD styles, LPARAM lparam, wyBool isvisible = wyTrue, 
                               wyBool flip = wyFalse);

/// Sets the class pointer associated with the window
/**
@param hwnd         : IN Grid Window Handle
@param ccp          : IN Custom grid pointer
@returns the previous value on success else returns 0
*/
LONG		SetCustCtrlData	(HWND hwnd, CCustGrid* ccp);

/// Gets the pointer associated with the window
/**
@param hwnd         : IN Grid Window Handle
@returns the pointer on success else 0       
*/
CCustGrid*	GetCustCtrlData(HWND hwnd);

/// Number of rows that can be displayed per page
/**
@param hwnd         : IN Grid Window Handle
@returns the number of rows that can be displayed per page
*/
wyInt32		CustomGrid_RowPerPage			(HWND hwnd);

///
/**
@param hwnd         : IN Grid Window Handle
@param pgvcol       : IN Pointer to grid column
@param repaint      : IN Repaint the grid or not
@returns the new column size
*/
wyInt32		CustomGrid_InsertColumn			(HWND hwnd, PGVCOLUMN pgvcol, wyBool repaint = wyTrue);

/// Inserts a row in between 
/**
@param hwnd         : IN Grid Window Handle
@param rowbefore    : IN Current row
@returns the new row size
*/
wyInt32		CustomGrid_InsertRowInBetween	(HWND hwnd, wyInt32 rowbefore);

/// Gets the size of the item text
/**
@param hwnd         : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@returns the size of the text
*/
wyInt32		CustomGrid_GetItemTextLength	(HWND hwnd, wyInt32 row, wyInt32 col);

/// gets the text from the item
/**
@param hwnd         : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param text         : OUT Text from the item 
@returns the size of the text
*/
wyInt32		CustomGrid_GetItemText			(HWND hwnd, wyInt32 row, wyInt32 col, wyWChar *text);

/// Gets the column title for custom grid
/**
@param hwnd         : IN Grid Window Handle
@param col          : IN Column
@param buffer       : OUT Title length
@returns the title length
*/
wyInt32		CustomGrid_GetColumnTitle		(HWND hwnd, wyInt32 col, wyWChar *buffer);

/// Inserts text in list
/**
@param hwnd         : IN Grid Window Handle
@param col          : IN Column
@param buffer       : IN Text to insert
@returns index
*/
wyInt32		CustomGrid_InsertTextInList		(HWND hwnd, wyInt32 col, wyWChar *buffer);

/// Find text in list for custom grid
/**
@param hwnd         : IN Grid Window Handle
@param col          : IN Column
@param buffer       : IN Text to find
@returns index
*/
wyInt32		CustomGrid_FindTextInList		(HWND hwnd, wyInt32 col, wyWChar *buffer);

/// Sets the row checks
/**
@param hwnd         : IN Grid Window Handle
@param row          : IN Row number
@param ustate       : IN Checked or not
@returns wyTrue on success else wyFalse
*/
wyInt32		CustomGrid_SetRowCheckState		(HWND hwnd, wyInt32 row, wyBool ustate);

/// Checks for checks in a row
/**
@param hwnd         : IN Grid Window Handle
@param row          : IN Row number
@returns wyTrue on success
*/
wyInt32		CustomGrid_GetRowCheckState		(HWND hwnd, wyInt32 row);

/// Sets the font for the custom grid
/**
@param hwnd         : IN Grid Window Handle
@param lf           : IN Font information
@returns void
*/
void		CustomGrid_SetFont				(HWND hwnd, PLOGFONT lf);

/// Sets the flag to set or reset the Flip grid
/**
@param hwnd         : IN Window HANDLE
@param flag         : IN Flip or not
*/
void        CustomGrid_SetFlip              (HWND hwnd, wyBool flag);

/// Sets the flag to set or reset the Flip grid
/**
@param hwnd         : IN Window HANDLE
@param flag         : IN Flip or not
*/
void        CustomGrid_SetMaxWidth          (HWND hwnd, wyInt32 width);

/// Sets the flag to set or reset the FORM view mode
/**
@param hwnd         : IN Window HANDLE
@param flag         : IN Enable/Disable
*/ 
void        CustomGrid_SetFormMode          (HWND hwnd, wyBool flag);

/// Jumps to the next record 
/** 
@param              : IN Window HANDLE
*/
void        CustomGrid_NextRecord           (HWND hwnd);

/// Jumps to the prev record 
/** 
@param              : IN Window HANDLE
*/
void        CustomGrid_PrevRecord           (HWND hwnd);

/// Jumps to the Last record 
/** 
@param              : IN Window HANDLE
*/
void        CustomGrid_LastRecord           (HWND hwnd);

/// Jumps to the First record 
/** 
@param              : IN Window HANDLE
*/
void        CustomGrid_FirstRecord          (HWND hwnd);


/// Sets the owner data
/**
@param hwnd         : IN Grid Window Handle
@param owner        : IN Owner or not
@returns void
*/
void		CustomGrid_SetOwnerData			(HWND hwnd, wyBool owner);

/// Sets the maximum number of rows count
/**
@param hwnd         : IN Grid Window Handle
@param count        : IN Row count
@returns void
*/
void		CustomGrid_SetMaxRowCount		(HWND hwnd, LONG count);

/// Gets the owner data from the grid
/**
@param hwnd         : IN Grid Window Handle
@returns wyTrue if success else wyFalse
*/
wyBool		CustomerGrid_GetOwnerData		(HWND hwnd);

///  Deletes the list contents for custom grid
/**
@param hwnd         : IN Grid Window Handle
@param col          : IN Column number
@returns wyTrue if success else wyFalse
*/
wyInt32     CustomGrid_DeleteListContent	(HWND hwnd, wyInt32 col);

/// Gets the bool value for a item
/** 
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@returns wyTrue if success else wyFalse          
*/
wyBool		CustomGrid_GetBoolValue			(HWND hwnd, wyInt32 row, wyInt32 col);

/// Sets the current selection for custom grid
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column  number
@param sendmsg      : IN Confirm selection
@returns wyTrue if success
*/
wyBool		CustomGrid_SetCurSelection		(HWND hwnd, wyInt32 row, wyInt32 col, wyBool sendmsg = wyTrue);

/// Sets the current selection row
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param sendmsg      : IN Confirm selection
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_SetCurSelRow			(HWND hwnd, LONG row, wyBool sendmsg = wyTrue);

/// Sets the current selected column
/**
@param hwnd     : IN Grid Window Handle
@param col          : IN Column number
@param sendmsg      : IN Confirm selection
@returns wyTrue if success else wyFalse 
*/
wyBool		CustomGrid_SetCurSelCol			(HWND hwnd, LONG col, wyBool sendmsg = wyTrue);

/// Sets the initial row
/**
@param hwnd         : IN Grid Window Handle
@param initrow      : IN Initial row
@param repaint      : IN Confirm repaint
@returns wyTrue if success else wyFalse 
*/
wyBool		CustomGrid_SetInitRow			(HWND hwnd, wyInt32 initrow, wyBool repaint = wyTrue);

/// Sets the initial column
/**
@param hwnd     : IN Grid Window Handle
@param initcol      : IN Initial column
@param repaint      : IN Confirm repaint
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_SetInitCol			(HWND hwnd, wyInt32 initcol, wyBool repaint = wyTrue);

/// Applies changes for the custom grid
/**
@param hwnd     : IN Grid Window Handle
@param sendmsg      : IN Confirm or not
@returns wyTrue if success 
*/
wyBool		CustomGrid_ApplyChanges			(HWND hwnd, wyBool sendmsg = wyTrue);

/// Cancels changes for the custom grid
/**
@param hwnd     : IN Grid Window Handle
@param sendmsg      : IN Confirm or not
@returns wyTrue if success 
*/
wyBool      CustomGrid_CancelChanges        (HWND hwnd, wyBool sendmsg = wyTrue);

/// Set text for custom grid
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param text         : IN Text to set
@returns wyTrue if success
*/
wyBool		CustomGrid_SetText				(HWND hwnd, wyInt32 row, wyInt32 col, const wyChar *text);

/// Sets the text for the buttons 
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column    
@param text         : IN Text to set
@returns wyTrue if success       
*/
wyBool		CustomGrid_SetButtonText		(HWND hwnd, wyInt32 row, wyInt32 col, wyWChar *text);

/// Gets the text at a particular position
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param text         : IN text
@returns wyTrue if success else wyFalse           
*/
wyBool		CustomGrid_SetTextPointer		(HWND hwnd, wyInt32 row, wyInt32 col, wyChar *text);

/// Sets the ownership of the items 
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param utrueorfalse : IN Owner or not
@returns wyTrue on success
*/
wyBool		CustomGrid_SetBoolValue			(HWND hwnd, wyInt32 row, wyInt32 col, GV_BOOLVALUE trueorfalse);

/// Delete the respective row
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param repaint      : IN Whether to repaint or not
@returns wyTrue if success else wyFalse
*/

wyBool		CustomGrid_DeleteRow			(HWND hwnd, wyInt32 row, wyBool repaint = wyTrue);

/// Get the item from a point 
/**
@param hwnd     : IN Grid Window Handle
@param lppnt        : IN Point info
@param prow         : IN Row
@param pcol         : IN Column
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_GetItemFromPoint		(HWND hwnd, LPPOINT lppnt, wyInt32 *prow, wyInt32 *pcol);

/// Delete all rows
/**
@param hwnd     : IN Grid Window Handle
@returns wyTrue if success
*/
wyBool		CustomGrid_DeleteAllRow			(HWND hwnd, wyBool ispaint = wyTrue);

/// Deletes all column
/**
@param hwnd     : IN Grid Window Handle
@param repaint      : IN Repaint the window
@returns wyTrue if success
*/
wyBool		CustomGrid_DeleteAllColumn		(HWND hwnd, wyBool repaint = wyTrue);

/// Sets the long row data
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param lparam       : IN Long message parameter
@returns wyTrue if success
*/
wyBool		CustomGrid_SetRowLongData		(HWND hwnd, wyInt32 row, LPARAM lparam);

/// Sets the column to read only and vice versa
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param ustate       : IN Readonly or not
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_SetColumnReadOnly	(HWND hwnd, wyInt32 row, wyInt32 col, wyBool ustate);

/// Sets the button visibility
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param ustate       : IN Visible or not
@returns wyTrue if success
*/
wyBool		CustomGrid_SetButtonVis			(HWND hwnd, wyInt32 row, wyInt32 col, wyBool ustate);

/// Sets the column width
/**
@param hwnd     : IN Grid Window Handle
@param col          : IN Column number
@param cx           : IN Width
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_SetColumnWidth		(HWND hwnd, wyInt32 col, wyUInt32 cx);

/// Sets the column to the default value
/**
@param hwnd     : IN Grid Window Handle
@param col          : IN Column number
@param text         : IN Value that needs to be set
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_SetColumnDefault		(HWND hwnd, wyInt32 col, const wyChar *text);

/// Function to ensure that a cell is visible in the screen.
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@returns wyTrue if success else wyFalse
*/
wyBool		CustomGrid_EnsureVisible		(HWND hwnd, wyInt32 row, wyInt32 col, wyBool init = wyFalse);

/// Gets the number of rows
/**
@param hwnd     : IN Grid Window Handle
@returns the number of rows
*/
LONG		CustomGrid_GetRowCount			(HWND hwnd);

/// Gets the current selected cell
/** 
@param hwnd     : IN Grid Window Handle
@param focus	: IN Consider focused grid or not
@returns long pointer to the cell
*/
LONG		CustomGrid_GetCurSelection		(HWND hwnd, wyBool focus = wyFalse);

/// Gets the current selected row
/**
@param hwnd     : IN Grid Window Handle
@returns the current selected row
*/
LONG		CustomGrid_GetCurSelRow			(HWND hwnd);

/// Gets the current selected column 
/**
@param hwnd     : IN Grid Window Handle
@returns current selected column
*/
LONG		CustomGrid_GetCurSelCol			(HWND hwnd);

/// Gets the number of column
/**
@param hwnd     : IN Grid Window Handle
@returns the number of columns
*/
LONG		CustomGrid_GetColumnCount		(HWND hwnd);

/// Sets the extended style for custom grid
/**
@param hwnd     : IN Grid Window Handle
@param exstyle      : IN Extended style value
@returns the old style
*/
LONG		CustomGrid_SetExtendedStyle		(HWND hwnd, LONG exstyle);


/// Inserts a row in the table
/**
@param hwnd     : IN Grid Window Handle
@returns row number 
*/
LONG		CustomGrid_InsertRow			(HWND hwnd);

/// Inserts a row in the table
/**
@param hwnd     : IN Grid Window Handle
@param dontpaint    : allow repainting or not while inserting
@returns the row number    
*/
LONG		CustomGrid_InsertRow			(HWND hwnd, wyBool dontpaint);

/// Gets the initial row
/**
@param hwnd     : IN Grid Window Handle
@returns initial row
*/
LONG		CustomGrid_GetInitRow			(HWND hwnd);

/// Gets the initial column
/**
@param hwnd     : IN Grid Window Handle
@returns initial column
*/
LONG		CustomGrid_GetInitCol			(HWND hwnd);


/// Gets the column font for custom grid
/**
@param hwnd     : IN Grid Window Handle
@returns font HANDLE
*/
HFONT		CustomGrid_GetColumnFont		(HWND hwnd);

/// Gets the row font for custom grid
/**
@param hwnd     : IN Grid Window Handle
@returns font HANDLE
*/
HFONT		CustomGrid_GetRowFont			(HWND hwnd);

/// Gets the long value associated with a grid
/**
@param hwnd         : IN Grid Window Handle
@returns long message parameter
*/
LPARAM		CustomGrid_GetLongData			(HWND hwnd);

/// Sets the long value associated with a item
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@param lparam       : IN Long message parameter
@returns long message parameter
*/
LPARAM		CustomGrid_SetItemLongValue		(HWND hwnd, wyInt32 row, wyInt32 col, LPARAM lparam);

/// Gets the long value associated with a item
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param col          : IN Column number
@returns long message parameter
*/
LPARAM		CustomGrid_GetItemLongValue		(HWND hwnd, wyInt32 row, wyInt32 col);

/// Sets the long value associated with a row
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@param lparam       : IN Long message parameter
@returns long message parameter
*/
LPARAM		CustomGrid_SetColumnLongValue	(HWND hwnd, wyInt32 col, LPARAM lparam);

/// Gets the long value associated with a column
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@returns long message parameter
*/
LPARAM		CustomGrid_GetColumnLongValue	(HWND hwnd, wyInt32 col);

/// Gets the long value associated with a row
/**
@param hwnd     : IN Grid Window Handle
@param row          : IN Row number
@returns long message parameter
*/
LPARAM		CustomGrid_GetRowLongData		(HWND hwnd, wyInt32 row);

/// Free the custom grid row data
/**
@param hwnd     : IN Grid Window Handle
@param rowdata  : IN Row data
@returns row data pointer
*/
PROWDATA	CustomGrid_FreeRowData			(HWND hwnd, ROWDATA * rowdata);

/// Gets the custom grid item row
/**
@param hwnd     : IN Grid Window Handle
@param row      : IN Row number
@returns row data pointer
*/
PROWDATA	CustomGrid_GetItemRow			(HWND hwnd, wyInt32 row);

/// Finds the text in the column
/**
@param hwnd     : IN Grid Window Handle
@param row      : IN Row number
@returns row data pointer
*/
wyInt32 CustomGrid_FindTextInColumn         (HWND hwnd, wyInt32 row, wyInt32 col, wyChar *text);

/// Enables/Disables the visibility of the Grid
/**
@param hwnd     : IN Grid Window Handle
@param state    : IN wyTrue/wyFalse
@returns void
*/
void    CustomGrid_ShowGrid                 (HWND hwnd, wyBool isvisible);


/// Get the max width of the column
/**
@param hwnd     : IN Grid Window Handle
returns max column width
*/
wyInt32 CustomGrid_GetMaxColumnWidth        (HWND hwnd);

/// Tells which column header is clicked
/**
*/
wyInt32 CustomGrid_GetRowHeader(HWND hwnd, POINT *pnt);

/// Get the row and column 
/**
*/
wyInt32 CustomGrid_GetRowColumn(HWND hwnd, LPARAM lparam, wyInt32 *row, wyInt32 *col);

/// Gets the rectangle for the given cell
   /**
   @param hwnd     : IN Grid Window Handle
   @param row      : IN Row number
   @param col      : IN Column number
   @param lprect   : OUT Pointer to cell rectangle
   @returns void
 */
void CustomGrid_GetSubItemRect(HWND hwnd, wyInt32 row, wyInt32 col, RECT *rect);

/// Gets the column width 
/**
@param hwnd     : IN Grid Window Handle
@param col      : IN Column number
@param width	: OUT Column width
@returns void
*/
wyBool CustomGrid_GetColumnWidth(HWND hwnd, wyInt32 col, wyUInt32 * width);

//Show/Hide the Scroll bar of grid
/**
@param hwnd  : IN Grid Window Handle
@scrollid	 : IN SB_VERT or SB_HORZ
@status      : IN wyTrue to show scroll bar, wyFalse to hide
@returns void
*/
//void CustomGrid_ShowScrollBar(HWND hwnd, wyInt32 scrollid, wyBool status);

///Function sets the column(s) to hide or show again
/**
@param column : IN Index of the column 
@param ishide : IN wyTrue to hide, wyFalse to show
@return VOID
*/
void	CustomGrid_ShowOrHideColumn         (HWND hwnd, wyInt32 column, wyBool ishide);

///Function gets the active control text length, if editing is in progress
/**
@return length of the text
*/
wyInt32 CustomGrid_GetActiveControlTextLength(HWND hwnd);

///Function gets the active control text, if editing is in progress
/**
@param text   : OUT buffer to store the text
@param length : IN length of the text
@return void
*/
void    CustomGrid_GetActiveControlText(HWND hwnd, wyWChar* text, wyInt32 length);

///Function sets the column mask
/**
@param col    : IN Index of the column 
@param mask   : IN column mask
@return wyTrue on success else wyFalse
*/
wyBool  CustomGrid_SetColumnMask(HWND hwnd, wyInt32 col, wyInt32 mask);

///Function gets the column mask
/**
@param col    : IN Index of the column 
@return column mask
*/
wyInt32 CustomGrid_GetColumnMask(HWND hwnd, wyInt32 col);


GVWNDPROC CustomGrid_GetGridProc(HWND hwnd);

wyInt32 CustomGrid_GetSelAllState(HWND hwnd);
void CustomGrid_SetSelAllState(HWND hwnd, wyInt32 state);
#endif

