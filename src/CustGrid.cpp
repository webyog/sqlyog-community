/* Copyright (C) 2013 Webyog Inc

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

#include <mlang.h>
#include <stdio.h>
#include <crtdbg.h>
#include <winuser.h>

#include "CustGrid.h"
#include "FrameWindowHelper.h"
#include "GUIHelper.h"
#include "CommonHelper.h"

wyWChar	customname[] = L"CustomGridControl";
wyWChar	splitter[]   = L"CustomGridSplitter";

#define	GV_GRIDPICKLIST						1000
//#define GV_SELCOLOR							RGB(221, 220, 235)RGB(219,236,252)
#define	GV_SELCOLOR							RGB(184,215,246)
#define	SHIFTED								0x8000
#define	YOG_GET_WHEEL_DELTA_wparam(wparam)	((short)HIWORD(wparam))
#define	YOG_DFCS_TRANSPARENT				0x0800
#define	YOG_DFCS_HOT						0x1000
#define	FONT_DIV_FACTOR				        72
#define FONT_BASE_HEIGHT					8
#define RED									255
#define DIS_BKG								233
#define CB_BTN_LENGTH						20
#define GV_LIST_WIDTH                       20
#define GV_LIST_HEIGHT                      GV_LISTBOX
#define BROWSEBUTTONTEXT                    L"..."

#define GV_PAGEUP_DISP						2

#define SPI_GETWHEELSCROLLCHARS				0x006C
#define SPI_SETWHEELSCROLLCHARS		        0x006D
#define DEF_WHEELSCROLLLINES			    3
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL						0x020A
#endif	
//#define		WM_MOUSEWHEEL	WM_MOUSELAST+1

/*----------------------------------------------------------------------------------------
	Function	:
					InitCustomGrid
	Purpose		:
					Registers the Custom Grid Control
	Return Values :
					If the function succeeds, the return value is a class atom that 
					uniquely identifies the class being registered.
					If the function fails, the return value is zero. To get extended error 
					information, call GetLastError(). 
----------------------------------------------------------------------------------------*/
static int st = 0;

void logtext(wyChar *buff)
{
#ifdef _DEBUG
    //FILE	*fp = fopen ( "c:\\SQLyog_log.log", "a+");
	//fprintf(fp, "%s\n" , buff);
	//fclose ( fp );
#endif
}

ATOM InitCustomGrid()
{
	ATOM	   ret;	
	WNDCLASSEX wc;

	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = splitter;
	wc.hInstance      = GetModuleHandle(0);
	wc.lpfnWndProc    =(WNDPROC)CCustGrid::SplitterWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_SIZEWE);
	wc.hIcon          = 0;
	wc.lpszMenuName   = 0;
	wc.hbrBackground  =(HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	wc.style          = 0;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hIconSm        = 0;


	VERIFY(ret = RegisterClassEx(&wc));

	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = customname;
	wc.hInstance      = GetModuleHandle(0);
	wc.lpfnWndProc    =(WNDPROC)CCustGrid::CustomGridWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon          = 0;
	wc.lpszMenuName   = 0;
	wc.hbrBackground  =(HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	wc.style          = CS_DBLCLKS;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hIconSm        = 0;

	return RegisterClassEx(&wc);
}

/*----------------------------------------------------------------------------------------
	Function	:
					SetCustCtrlData
	Purpose		:
					Sets the class poINTer associated with the window 
	Return Values :
					If the function succeeds, the return value is the previous value.
					If the function fails, the return value is zero. To get extended error 
					information, call GetLastError()
----------------------------------------------------------------------------------------*/

LONG SetCustCtrlData(HWND hwnd, CCustGrid* ccp)
{
	// Set the poINTer to the window.
	return SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)ccp);
}

/*----------------------------------------------------------------------------------------
	Function		:	
						GetCustCtrlData
	Purpose			:	
						Gets the class poINTer associated with the window 
	Return Values	:	
						If the function succeeds, the return value is the poINTer.
						If the function fails, the return value is zero. To get extended error 
						information, call GetLastError()
----------------------------------------------------------------------------------------*/

CCustGrid* GetCustCtrlData(HWND hwnd)
{
	// Set the poINTer to the window.
	return(CCustGrid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

/*----------------------------------------------------------------------------------------
	Function	:
					CreateCustomGrid
	Purpose		:
					Creates the custom grid control.
	Return Values :
					If successful returns the handle to the new GridCotrol.
					If the function fails, the return value is NULL. To get extended error 
					information, call GetLastError()					
----------------------------------------------------------------------------------------*/

HWND CreateCustomGrid(HWND hwndparent, wyInt32 x, wyInt32 y, wyInt32 width, 
                      wyInt32 height, GVWNDPROC m_lpgvwndproc, LPARAM lparam, wyBool isvisible, wyBool flip)
{

	HWND	hwndgrid;
	hwndgrid = CreateWindowEx(WS_EX_WINDOWEDGE,  customname, TEXT("Custom Grid Control"),
                                WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
								x, y, width, height, hwndparent, NULL, 
                                GetModuleHandle(0),(LPVOID)m_lpgvwndproc);

	if(!hwndgrid)
		return NULL;

	SendMessage(hwndgrid, GVM_SETLONGDATA, 0, lparam);
    CustomGrid_ShowGrid(hwndgrid, isvisible);
	return hwndgrid;
}

HWND CreateCustomGridEx(HWND hwndparent, wyInt32 x, wyInt32 y, wyInt32 width, wyInt32 height, 
                        GVWNDPROC m_lpgvwndproc, DWORD styles, LPARAM lparam, wyBool isvisible, wyBool flip)
{
	HWND	hwndgrid;
	CCustGrid *pcg;
	TOOLINFO toolinfo;
	hwndgrid = CreateWindowEx(WS_EX_WINDOWEDGE,  customname, TEXT("Custom Grid Control"), 
                                    WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
								    x, y, width, height, hwndparent, NULL, 
                                    GetModuleHandle(0),(LPVOID)m_lpgvwndproc);

	if(!hwndgrid)
		return NULL;

	SendMessage(hwndgrid, GVM_SETLONGDATA, 0, lparam);

	CustomGrid_SetExtendedStyle(hwndgrid, styles);

    CustomGrid_ShowGrid(hwndgrid, isvisible);
	
	
	if(styles & GV_EX_COL_TOOLTIP) {
		memset(&toolinfo, 0, sizeof(toolinfo));
		pcg = GetCustCtrlData(hwndgrid);

		pcg->m_hwndtooltip = CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS,
			NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			hwndgrid,
			NULL,
			GetModuleHandle(0),
			NULL);		

		SetWindowPos(pcg->m_hwndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		toolinfo.cbSize = sizeof(TOOLINFO);
		toolinfo.uFlags =  TTF_SUBCLASS | TTF_ABSOLUTE | TTF_IDISHWND;
		toolinfo.hinst = GetModuleHandle(0);
		toolinfo.hwnd = hwndgrid;
		toolinfo.lpszText = LPSTR_TEXTCALLBACK;
		toolinfo.uId = (UINT_PTR)hwndgrid;
		GetClientRect(hwndgrid, &toolinfo.rect);
		SendMessage(pcg->m_hwndtooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&toolinfo);
		SendMessage(pcg->m_hwndtooltip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 800);
	}

	return hwndgrid;
}

LPARAM CustomGrid_GetLongData(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetLongData();
}

LPARAM CustomGrid_GetItemLongValue(HWND hwnd, wyInt32 row, wyInt32 col)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetItemLongValue(row, col);
}

LPARAM CustomGrid_SetItemLongValue(HWND hwnd, wyInt32 row, wyInt32 col, LPARAM lparam)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetItemLongValue(row, col, lparam);
}

LPARAM CustomGrid_GetColumnLongValue(HWND hwnd, wyInt32 col)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetColumnLongValue(col);
}

wyInt32    CustomGrid_GetColumnTitle(HWND hwnd, wyInt32 col, wyWChar *buffer)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetColumnTitle(col, buffer);
}

LPARAM CustomGrid_SetColumnLongValue(HWND hwnd, wyInt32 col, LPARAM lparam)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetColumnLongValue(col, lparam);
}

wyBool CustomGrid_GetBoolValue(HWND hwnd, wyInt32 row, wyInt32 col)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return	pcg->GetBoolValue(row, col);
}

wyInt32	
CustomGrid_InsertColumn(HWND hwnd, PGVCOLUMN pgvcol, wyBool repaint)
{
	wyInt32	    ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	ret = pcg->Insert_Column(pgvcol);

	if(repaint)
		VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

void
CustomGrid_SetMaxWidth(HWND hwnd, wyInt32 width)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    pcg->SetMaxWidth(width);
}

LONG CustomGrid_InsertRow(HWND hwnd)
{
	LONG        ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->Insert_Row();
	VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

LONG CustomGrid_InsertRow(HWND hwnd, wyBool donpaint)
{
	LONG        ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

    ret = pcg->Insert_Row();

	VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyInt32 CustomGrid_InsertRowInBetween(HWND hwnd, wyInt32 rowbefore)
{
	wyInt32     ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->InsertRowInBetween(rowbefore);

	VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyBool CustomGrid_SetText(HWND hwnd, wyInt32 row, wyInt32 col, const wyChar *text)
{
	wyBool  ret;

	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->SetSubItemText(row, col, text);

    if(ret == wyTrue)
	    VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyBool CustomGrid_SetButtonText(HWND hwnd, wyInt32 row, wyInt32 col, wyWChar *text)
{
	wyBool      ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->SetSubButtonItemText(row, col, text);

    if(ret == wyTrue)
	    VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyBool CustomGrid_SetTextPointer(HWND hwnd, wyInt32 row, wyInt32 col, wyChar *text)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetTextPointer(row, col, text);

}

wyBool CustomGrid_SetBoolValue(HWND hwnd, wyInt32 row, wyInt32 col, GV_BOOLVALUE trueorfalse)
{
	wyBool      ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	ret = pcg->SetSubItemText(row, col, trueorfalse);

    if(ret == wyTrue)
	    VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyInt32 CustomGrid_GetItemTextLength(HWND hwnd, wyInt32 row, wyInt32 col)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetItemTextLength(row, col);
}

wyInt32	CustomGrid_InsertTextInList(HWND hwnd, wyInt32 col, wyWChar *buffer)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->InsertTextInList(col, buffer);
}

wyInt32 CustomGrid_DeleteListContent(HWND hwnd, wyInt32 col)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->DeleteListContent(col);
}

wyInt32	CustomGrid_FindTextInList(HWND hwnd, wyInt32 col, wyWChar *buffer)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->FindTextInList(col, buffer);
}

wyInt32 CustomGrid_FindTextInColumn(HWND hwnd, wyInt32 col, wyInt32 itempreced, wyChar *text)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->FindTextInColumn(col, itempreced, text);
}

wyInt32 CustomGrid_SetRowCheckState(HWND hwnd, wyInt32 row, wyBool state)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetRowCheckState(row, state);
}

wyInt32 CustomGrid_GetRowCheckState(HWND hwnd, wyInt32 row)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return	pcg->GetRowCheckState(row);
}

wyBool CustomerGrid_GetOwnerData(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return	pcg->GetOwnerData();
}

void CustomGrid_SetOwnerData(HWND hwnd, wyBool val)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->SetOwnerData(val);
}

void CustomGrid_SetMaxRowCount(HWND hwnd, LONG count)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->SetMaxRowCount(count);
}

wyInt32 CustomGrid_GetItemText(HWND hwnd, wyInt32 row, wyInt32 col, wyWChar *text)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetItemText(row, col, text);
}

wyBool CustomGrid_GetItemFromPoint(HWND hwnd, LPPOINT lppnt, wyInt32 *prow, wyInt32 *pcol)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetItemFromPoint(lppnt, prow, pcol);
}

PROWDATA CustomGrid_GetItemRow(HWND hwnd, wyInt32 row)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetItemRow(row);
}

PROWDATA CustomGrid_FreeRowData(HWND hwnd, PROWDATA prowdata)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
		
	return pcg->FreeRowData(prowdata);
}

wyBool CustomGrid_DeleteRow(HWND hwnd, wyInt32 row, wyBool repaint)
{
	wyBool      ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->DeleteRow(row);

	if(repaint)
		VERIFY(InvalidateRect(hwnd, NULL, TRUE));
	
	return ret;
}

LONG CustomGrid_GetRowCount(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetRowCount();
}

LONG CustomGrid_GetColumnCount(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetColumnCount();
}



LONG CustomGrid_SetExtendedStyle(HWND hwnd, LONG m_exstyle)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetExtendedStyle(m_exstyle);
}

HFONT CustomGrid_GetColumnFont(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetColumnFont();
}

HFONT CustomGrid_GetRowFont(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetRowFont();
}

void CustomGrid_SetFont(HWND hwnd, PLOGFONT lf)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->SetGridFont(lf);
}

void CustomGrid_SetFlip(HWND hwnd, wyBool flag)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->SetFlip(flag);
}

void CustomGrid_LastRecord(HWND hwnd)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    pcg->LastRecord(hwnd);
}

void CustomGrid_FirstRecord(HWND hwnd)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    pcg->FirstRecord(hwnd);
}

void CustomGrid_NextRecord(HWND hwnd)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->NextRecord(hwnd);
}

void CustomGrid_PrevRecord(HWND hwnd)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->PrevRecord(hwnd);
}

void CustomGrid_SetFormMode(HWND hwnd, wyBool flag)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	pcg->SetFormMode(flag);
}

wyBool CustomGrid_DeleteAllRow(HWND hwnd, wyBool ispaint)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->DeleteAllRow(ispaint);
}

wyBool CustomGrid_DeleteAllColumn(HWND hwnd, wyBool repaint)
{
	wyBool		ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	ret = pcg->DeleteAllColumns();

    if(repaint)
		VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyInt32	CustomGrid_RowPerPage(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->RowPerPage();
}

wyBool CustomGrid_SetRowLongData(HWND hwnd, wyInt32 row, LPARAM lparam)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetRowLongData(row, lparam);
}

LPARAM CustomGrid_GetRowLongData(HWND hwnd, wyInt32 row)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetRowLongData(row);
}

LPARAM CustomGrid_GetLongData(HWND hwnd, wyInt32 row)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetRowLongData(row);
}

wyBool CustomGrid_CancelChanges(HWND hwnd, wyBool sendmsg)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);
    
    if(sendmsg == wyTrue && pcg->IsEditing())
        return pcg->ProcessEscPress() ? wyTrue : wyFalse;

    return wyTrue;
}

wyBool CustomGrid_ApplyChanges(HWND hwnd, wyBool sendmsg)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

    if(sendmsg == wyTrue)
	    return pcg->ApplyChanges();
     
    return wyTrue;
}

wyBool CustomGrid_SetCurSelection(HWND hwnd, wyInt32 row, wyInt32 col, wyBool sendmsg)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->SetCurSelection(row, col, sendmsg);
}

wyBool CustomGrid_SetCurSelRow(HWND hwnd, LONG row, wyBool sendmsg)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->SetCurSelRow(row, sendmsg);
}

wyBool CustomGrid_SetCurSelCol(HWND hwnd, LONG col, wyBool sendmsg)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->SetCurSelCol(col, sendmsg);
}

wyBool CustomGrid_SetInitRow(HWND hwnd, wyInt32 initrow, wyBool repaint)
{
	wyBool		ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	ret = pcg->SetInitRow(initrow);

	if(repaint)
		VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

wyBool CustomGrid_SetInitCol(HWND hwnd, wyInt32 initcol, wyBool repaint)
{
	wyBool		ret;
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	ret = pcg->SetInitCol(initcol);

	if(repaint)
		VERIFY(InvalidateRect(hwnd, NULL, TRUE));

	return ret;
}

LONG CustomGrid_GetInitRow(HWND hwnd)
{	
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->GetInitRow();
}

LONG CustomGrid_GetInitCol(HWND hwnd)
{	
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->GetInitCol();
}

LONG CustomGrid_GetCurSelection(HWND hwnd, wyBool focus)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->GetCurSelection(focus);
}

LONG CustomGrid_GetCurSelRow(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->GetCurSelRow();
}

LONG CustomGrid_GetCurSelCol(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);
	
	return pcg->GetCurSelCol();
}

wyBool CustomGrid_SetColumnReadOnly(HWND hwnd, wyInt32 row, wyInt32 col, wyBool state)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetColumnReadOnly(row, col, state);
}

wyBool CustomGrid_SetButtonVis(HWND hwnd, wyInt32 row, wyInt32 col, wyBool state)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetButtonVis(row, col, state);

}

wyBool CustomGrid_SetColumnWidth(HWND hwnd, wyInt32 col, UINT cx)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetColumnWidth	(col, cx);
}

wyBool CustomGrid_EnsureVisible(HWND hwnd, wyInt32 row, wyInt32 col, wyBool init)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->EnsureVisible(row, col, init);
}


wyBool CustomGrid_SetColumnDefault(HWND hwnd, wyInt32 col, const wyChar * buf)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->SetColumnDefault(col, buf);
}

void CustomGrid_ShowGrid(HWND hwnd, wyBool isvisible)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);
    
    pcg->ShowGrid(isvisible);

    return;
}

wyInt32 CustomGrid_GetMaxColumnWidth(HWND hwnd)
{
	CCustGrid	*pcg = GetCustCtrlData(hwnd);

	return pcg->GetMaxColumnWidth();
}

wyInt32 CustomGrid_GetRowHeader(HWND hwnd, POINT *pnt)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    return pcg->GetRowHeader(pnt);
}

wyInt32 CustomGrid_GetRowColumn(HWND hwnd, LPARAM lparam, wyInt32 *row, wyInt32 *col)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    return pcg->GetRowColumn(lparam, row, col);
}

void CustomGrid_GetSubItemRect(HWND hwnd, wyInt32 row, wyInt32 col, RECT *rect)
{
    CCustGrid	*pcg = GetCustCtrlData(hwnd);

    return pcg->GetSubItemRect(row, col ,rect);
}

wyBool CustomGrid_GetColumnWidth(HWND hwnd, wyInt32 col, wyUInt32 * width)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);

    return pcg->GetColumnWidth(col, width);
}

/*
void CustomGrid_ShowScrollBar(HWND hwnd, wyInt32 scrollid, wyBool status)
{
	CCustGrid *pcg = GetCustCtrlData(hwnd);

    pcg->ShowOrHideScollBar(scrollid, status);
}
*/

void CustomGrid_ShowOrHideColumn(HWND hwnd, wyInt32 column, wyBool ishide)
{
	CCustGrid *pcg = GetCustCtrlData(hwnd);

    pcg->ShowOrHideColumn(column, ishide);
}

wyInt32 CustomGrid_GetActiveControlTextLength(HWND hwnd)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);

    return pcg->GetActiveControlTextLength();
}

void CustomGrid_GetActiveControlText(HWND hwnd, wyWChar* text, wyInt32 length)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);

    pcg->GetActiveControlText(text, length);
}

wyBool CustomGrid_SetColumnMask(HWND hwnd, wyInt32 col, wyInt32 mask)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);
    return pcg->SetColumnMask(col, mask);
}

wyInt32 CustomGrid_GetColumnMask(HWND hwnd, wyInt32 col)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);
    return pcg->GetColumnMask(col);
}

GVWNDPROC CustomGrid_GetGridProc(HWND hwnd)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);
    return pcg->GetGridProc();
}

wyInt32 CustomGrid_GetSelAllState(HWND hwnd)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);
    return pcg->GetSelAllState();
}
void CustomGrid_SetSelAllState(HWND hwnd, wyInt32 state)
{
    CCustGrid *pcg = GetCustCtrlData(hwnd);
    return pcg->SetSelAllState(state);
}

CCustGrid::CCustGrid(HWND hwnd)
{
	m_row			= 0;
	m_col			= 0;
	m_hwnd			= hwnd;
	m_hfont			= NULL;
	m_collist		= NULL;
	m_htopfont		= NULL;
	m_rowlist		= NULL;
	m_pgvcurcolnode = NULL;
    m_rowlast		= NULL;
	m_initrow		= 0;
	m_initcol		= 0;
	m_currowmodify	= -1;
	m_capturedcol	= -1;
	m_curselcol		= -1;
	m_curselrow		= -1;
	m_hwndcurcombo	= NULL;
	m_isediting	    = wyFalse;
	m_iscomboediting= wyFalse;
	m_exstyle       = 0;
	m_oldcoltext    = NULL;
    m_first         = wyTrue;   
	m_ownerdata     = wyFalse;
	m_rowcheck      = NULL;
	m_isscrolling   = wyFalse;
    m_isvisible     = wyTrue;
    m_hight         = 18;

    memset(&m_scrollbarinfo, 0, sizeof(GVSCROLLBARINFO));

    m_flip          = wyFalse;
    m_form          = wyFalse;
    m_maxwidth      = 0;
    m_maxtextwidth  = 0;
    m_iskeyboard    = wyTrue;

	m_vscrollpos = 0;
	m_vscrollthumwidth = 0;
	m_vscrollmaxrange = 0;
	
	m_hscrollpos = 0;
	m_hscrollthumwidth = 0;
	m_hscrollmaxrange = 0;

	m_hscrollnumpos = 0;
	m_hskipcolcount = 0;	
	m_pointlbuttondown.x = 0;
	m_pointlbuttondown.y = 0;

    m_ispressed = wyFalse;
	g_bMouseTrack = wyFalse;
	m_tox = 0;
    m_selallinfo.checkstate = BST_UNCHECKED;
    m_checkcount = 0;

    m_hItalicsFont = NULL;
	m_hwndtooltip = NULL;
	m_mouseprevpt.x = 0;
	m_mouseprevpt.y = 0;
	m_tooltipidindex = 0; //  Tooltip unique id is set to zero 
}

CCustGrid::~CCustGrid()
{
	if(m_oldcoltext)
		delete[] m_oldcoltext;

	if(m_rowcheck)
		delete[] m_rowcheck;

	if(m_hfont)
		DeleteObject(m_hfont);

    if(m_hItalicsFont)
        DeleteObject(m_hItalicsFont);

	if(m_hwndtooltip)
		DestroyWindow(m_hwndtooltip);

	m_tooltipidindex = 0; // restting the tooltip unique id to zero so they are reassigned from zero again
}


LRESULT CALLBACK
CCustGrid::CustomGridWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT     ret;
	//retrieve the custom structure POINTER for THIS window 
  	CCustGrid   *pcg = GetCustCtrlData(hwnd);
    wyBool      ikeypressshandled = wyFalse;
	wyWChar*   tooltip = NULL;

	switch(msg)
	{
	case WM_NCCREATE:
		pcg = new CCustGrid(hwnd);
		SetCustCtrlData(hwnd,(CCustGrid*)pcg);
        if(pcg->m_isvisible == wyTrue)
        {
            VERIFY(pcg->CreateFonts());
		    pcg->m_crbkgnd = RGB(255, 255, 255);
        }
        pcg->m_lparam = lparam;
		return TRUE;
    
    case WM_CREATE:
        pcg->m_lpgvwndproc =(GVWNDPROC)((LPCREATESTRUCT)lparam)->lpCreateParams;
        return 0;

	case WM_COMMAND:
		return pcg->OnWMCommand(wparam, lparam);

    case WM_SYSKEYDOWN:
        if(!pcg->OnWMSysKeyDown(hwnd, wparam, lparam))
            return 0;
        break;

	case GVM_SETLONGDATA:
		return pcg->SetLongData(lparam);

	case WM_ERASEBKGND:
		return 1;

	case WM_GETDLGCODE:
		return pcg->OnGetDLGCode(lparam);

	case WM_MOUSEMOVE:
        {
            pcg->GetMouseMovement();
			if(wparam & MK_LBUTTON)
            {
                pcg->m_lpgvwndproc(hwnd, GVN_LBUTTONMOUSEMOVE, (WPARAM)0, (LPARAM)lparam);
                break;
            }

			if(!pcg->g_bMouseTrack)
			{
 				TRACKMOUSEEVENT tme = {0};
 				DWORD dwPos = GetMessagePos();
 				POINTS pts = MAKEPOINTS(dwPos);
 				tme.cbSize = sizeof(TRACKMOUSEEVENT);
 				tme.dwFlags = TME_HOVER | TME_LEAVE;
 				tme.hwndTrack = hwnd;
 				tme.dwHoverTime = 300;//HOVER_DEFAULT;
 				pcg->g_bMouseTrack = (wyBool)TrackMouseEvent(&tme);
 			}
			return pcg->OnMouseMove(wparam, lparam);
        }

	case WM_NOTIFY:
		return pcg->OnWMNotify(wparam, lparam);
	
	case WM_PAINT: 
		return pcg->OnPaint(wparam, lparam);

	case WM_VSCROLL:
        if(pcg->m_flip == wyTrue)
            ret = pcg->OnHScroll(hwnd, wparam, lparam);
        else
    		ret = pcg->OnVScroll(hwnd, wparam, lparam);

        pcg->m_lpgvwndproc(hwnd, GVN_VSCROLL, wparam, lparam);
        return ret;

	case WM_HSCROLL:
        if(pcg->m_flip == wyTrue)
            ret = pcg->OnVScroll(hwnd, wparam, lparam);
        else
		    ret = pcg->OnHScroll(hwnd, wparam, lparam);

		pcg->m_lpgvwndproc(hwnd, GVN_HSCROLL, wparam, lparam);

		return ret;

	case WM_LBUTTONDOWN:
        SetFocus(hwnd);
		pcg->OnLButtonDown(wparam, lparam);
		break;

	case WM_LBUTTONUP:
		pcg->OnLButtonUp(wparam, lparam);
		pcg->m_lpgvwndproc(hwnd, GVN_LBUTTONUP, wparam, lparam);
		break;

	case WM_MOUSEWHEEL:
		ret = pcg->OnMouseWheel(wparam, lparam);
        pcg->m_lpgvwndproc(hwnd, GVN_MOUSEWHEEL, wparam, lparam);
        return ret;

	case WM_RBUTTONDOWN:
		return pcg->OnRButtonDown(wparam, lparam);

	case WM_KEYDOWN:
        pcg->m_lpgvwndproc(hwnd, GVN_KEYDOWN, wparam, (LPARAM)&ikeypressshandled);
        return (ikeypressshandled == wyFalse) ? pcg->OnKeyDown(hwnd, wparam, lparam) : 1;

	case WM_CHAR:
		return pcg->OnWMChar(wparam, lparam);

	case WM_LBUTTONDBLCLK:
		return pcg->OnLButtonDblClk(wparam, lparam);

	case WM_SETFOCUS:
		return pcg->OnSetFocus(wparam, lparam);

	case WM_DESTROY:
		pcg->m_lpgvwndproc(pcg->m_hwnd, GVN_DESTROY, pcg->m_row, pcg->m_col);
		pcg->DestroyResources();
		break;

	case WM_NCDESTROY:
		pcg->OnNCDestroy(wparam, lparam);
		delete pcg;
		break;

	case GVM_PROCESSTAB:
		pcg->ProcessTabPress();
		return 0;

	case GVM_BUTTONCLICK:
		pcg->ProcessButtonClick(wparam, lparam);
		return 0;

	case GVM_STARTLISTEDIT:
		pcg->BeginColumnEdit(wparam, lparam);
		return 0;

	case GVM_SETFOCUS:
		SetFocus(hwnd);
		break;

    case GVM_SETFOCUSONACTIVECTRL:
        if(IsWindowVisible(pcg->m_hwndedit))
        {
            SetFocus(pcg->m_hwndedit);        
        }
        else if(pcg->m_pgvcurcolnode && pcg->m_pgvcurcolnode->hwndCombo && IsWindowVisible(pcg->m_pgvcurcolnode->hwndCombo))
        {
            SetFocus(pcg->m_pgvcurcolnode->hwndCombo);
        }
        break;
  
    case WM_MOUSELEAVE:
        pcg->m_lpgvwndproc(hwnd, GVN_MOUSELEAVE, wparam, lparam);
		pcg->g_bMouseTrack = wyFalse;
		/*if(pcg->m_hwndtooltip)
			{
			//	SendMessage(pcg->m_hwndtooltip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &pcg->m_toolinfo);
		//	pcg->m_hwndtooltip = NULL;
			}
	//	DestroyWindow(pcg->m_hwndtooltip);
	//	pcg->m_hwndtooltip = NULL;
	*/
		break;

	case WM_SIZE:
		/*For handling the painting issue in last row when change table has got H-scroll to 
		another table that doesn't have H-scroll*/
		if(pcg->m_tox <= LOWORD(lparam))
		{
			//If no scroll present then re-paint it
			InvalidateRect(hwnd, NULL, TRUE);
		}		
		break;

    case WM_HELP:
        if(!pcg->m_lpgvwndproc(hwnd, GVN_HELP, 0, 0))
        {
            return 1;
        }
        break;

    }
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void
CCustGrid::GetMouseMovement()
{
	POINT pnt;
	TRACKMOUSEEVENT	 te;

	if(m_hwndtooltip)
    {
		GetCursorPos(&pnt);
		ScreenToClient(m_hwnd, &pnt);

		if(pnt.x != m_mouseprevpt.x || pnt.y != m_mouseprevpt.y) {
			m_mouseprevpt = pnt;
			SendMessage(m_hwndtooltip, TTM_POP, 0, 0);
		}
    }

    //Here Throwing the WM_MOUSELEAVE Message
	te.cbSize    = sizeof(TRACKMOUSEEVENT);
	te.dwFlags   = TME_LEAVE;
	te.hwndTrack = m_hwnd;

    TrackMouseEvent(&te);

}

wyInt32
CCustGrid::GetMaxColumnWidth()
{
    return m_maxwidth;
}

void 
CCustGrid::NextRecord(HWND hwnd)
{
    if(m_flip == wyTrue)
        OnKeyDown(hwnd, GVKEY_RIGHT, 0);
    else
        OnKeyDown(hwnd, GVKEY_DOWN, 0);

    InvalidateRect(hwnd, NULL , FALSE);
}

void
CCustGrid::LastRecord(HWND hwnd)
{
    m_iskeyboard = wyFalse;
    
    OnKeyDown(hwnd, GVKEY_END, 0);
    
    InvalidateRect(hwnd, NULL , FALSE);
    
    m_iskeyboard = wyTrue;
}

void
CCustGrid::FirstRecord(HWND hwnd)
{
    m_iskeyboard = wyFalse;
    OnKeyDown(hwnd, GVKEY_HOME, 0);
    InvalidateRect(hwnd, NULL , FALSE);
    m_iskeyboard = wyTrue;
}

void
CCustGrid::PrevRecord(HWND hwnd)
{
    if(m_flip == wyTrue)
        OnKeyDown(hwnd, GVKEY_LEFT, 0);
    else
        OnKeyDown(hwnd, GVKEY_UP, 0);

    InvalidateRect(hwnd, NULL , FALSE);
}

void 
CCustGrid::DestroyResources()
{
	PGVCOLNODE pnode = GetColNodeStruct(0);

	while(pnode)
	{
		if(pnode->hwndCombo)
			DestroyWindow(pnode->hwndCombo);

		pnode->hwndCombo = NULL;
		pnode = pnode->pNext;
	}

    if(m_hwndbrowsebutton)
        DestroyWindow(m_hwndbrowsebutton);

    m_hwndbrowsebutton = NULL;

	if(m_hwndedit)
		DestroyWindow(m_hwndedit);

	m_hwndedit = NULL;

	if(m_hwndsplitter)
		DestroyWindow(m_hwndsplitter);

	m_hwndsplitter = NULL;

	if(m_htopfont)
	        VERIFY(DeleteFont(m_htopfont));
	
	m_htopfont = NULL;

	return;
}

LRESULT CALLBACK
CCustGrid::SplitterWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	CCustGrid *pcg =(CCustGrid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg)
	{
	case WM_NCCREATE:
        // Get the initial creation pointer to the window object
		pcg = (CCustGrid *)((CREATESTRUCT *)lparam)->lpCreateParams;        
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pcg);
        break;

	case WM_MOUSEMOVE:
		return pcg->OnSplitterMouseMove(wparam, lparam);

	case WM_CAPTURECHANGED:
		VERIFY(ShowWindow(hwnd, SW_HIDE));
		break;

	case WM_LBUTTONUP:
		pcg->OnSplitterButtonUp(wparam, lparam); 
		ReleaseCapture();
		pcg->m_lpgvwndproc(hwnd, GVN_RESETDATAVIEWTOOLTIP, 0, 0); // To Reset the tooltip
		break;
	}
	return(DefWindowProc(hwnd, msg, wparam, lparam));
}

LRESULT	CALLBACK 
CCustGrid::EditWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	wyInt32     retval, rc;
	CCustGrid	*pccustgrid	= (CCustGrid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32     islist = 0, islistdrop = 0;

	switch(message)
	{
	case WM_GETDLGCODE:
		if(lparam)
			return DLGC_WANTALLKEYS;
		break;

	case WM_CHAR:
        islist = (pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_LIST);
        rc = pccustgrid->OnEditProcWmChar(hwnd, message, wparam, lparam, islist, &retval);

        if(retval == 1)
            return rc;
		break;

    case WM_SYSKEYDOWN:
        if(!pccustgrid->OnWMSysKeyDown(hwnd, wparam, lparam))
            return 0;
        break;

	case WM_KEYDOWN:
        islist = (pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_LIST);
        islistdrop = (pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNLIST  || pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNNLIST);
        rc = pccustgrid->OnEditProcWmKeyDown(hwnd, message, wparam, lparam, islist, islistdrop, &retval);
        if(retval == 1)
            return rc;
	}

	return CallWindowProc(pccustgrid->m_wporigeditwndproc, hwnd, message, wparam, lparam);
}

LRESULT	CALLBACK 
CCustGrid::ListWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CCustGrid	*pccustgrid	= (CCustGrid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32     islist = 0, isdropdown = 0;
    wyInt32     index;
    wyWChar     text[512] = {0};
			
	switch(message)
	{
	
	case WM_LBUTTONUP:
        {			
			if(pccustgrid && pccustgrid->m_pgvcurcolnode)
			{
				islist = (pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_LIST);
				isdropdown = (pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNLIST  || 
							  pccustgrid->m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNNLIST);
			}
		
            if(!islist && !isdropdown)
                break;

            VERIFY((index = SendMessage(hwnd, LB_GETCURSEL, 0, 0)) != LB_ERR);

            if(index == -1) // invalid selection
            {
                pccustgrid->EndLabelEdit(pccustgrid->m_curselrow, pccustgrid->m_curselcol);
                break;
            }

		    SendMessage(hwnd, LB_GETTEXT, index, (LPARAM)text);
		    SendMessage(pccustgrid->m_hwndedit, WM_SETTEXT, 0, (LPARAM)text);
		    pccustgrid->EndLabelEdit(pccustgrid->m_curselrow, pccustgrid->m_curselcol);
		    VERIFY(InvalidateRect(pccustgrid->m_hwnd, NULL, FALSE));
        }
        break;	
    }

    
	return CallWindowProc(pccustgrid->m_wporiglistwndproc, hwnd, message, wparam, lparam);
}

wyInt32 
CCustGrid::OnEditProcWmChar(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, 
                                                        wyInt32 islist, wyInt32 *status)
{
    LONG		len, retval;
	wyWChar		text[1024] = {0};
	HWND		hwndcombo;

    *status = 1;
	switch(wparam)
	{
	case VK_TAB:
        if(m_isediting == wyTrue)
        {
            if(EndLabelEdit(m_curselrow, m_curselcol) == wyTrue)
            {
		        ProcessTabPress();
		        InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        else
        {
            SetFocus(m_hwnd);
            PostMessage(m_hwnd, GVM_PROCESSTAB, 0, 0);
        }
		return 0;

    case VK_RETURN:
    case VK_ESCAPE:
        return 0;

	default:

        if(wparam == VK_SPACE && GetKeyState(VK_CONTROL) & SHIFTED && IsWindowVisible(m_hwndbrowsebutton))
        {
            m_lpgvwndproc(m_hwnd, GVN_BROWSEBUTTONCLICK, (WPARAM)m_curselrow, (LPARAM)m_curselcol);
            return 0;
        }

		if(islist)
        {
			hwndcombo = m_pgvcurcolnode->hwndCombo;
			retval = CallWindowProc(m_wporigeditwndproc, hwnd, message, wparam, lparam);
			len = GetWindowText(hwnd, text, 1024-1);

            if((SendMessage(hwndcombo, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)text)) == LB_ERR)
            {
				CallWindowProc(m_wporigeditwndproc, hwnd, WM_CHAR, VK_BACK, 0);
				len = GetWindowText(hwnd, text, 1024);
				SendMessage(hwndcombo, LB_SELECTSTRING,(WPARAM)-1, (LPARAM)text);
			}

			return retval;
		} 
	}
    *status = 0;
    return 0;
}

void 
CCustGrid::OnEditProcVkReturn()
{
	PGVCOLNODE	pgvcolnode = NULL;

    EndLabelEdit(m_curselrow, m_curselcol);

    if(m_curselcol < (m_col - 1))
	{
        m_curselcol++;

		if(!GetOwnerData())
		{
			pgvcolnode = GetColNodeStruct(m_curselcol);

			while(pgvcolnode && pgvcolnode->isshow == wyFalse)
			{
				pgvcolnode = GetColNodeStruct(++m_curselcol);
			}
		}
	}

	EnsureVisible(m_curselrow, m_curselcol);
	VERIFY(InvalidateRect(m_hwnd, NULL, FALSE));

    return;
}


wyInt32 
CCustGrid::OnEditProcVkUp(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist, wyInt32 islistdrop)
{
    wyInt32 ret;
// now we have to see if its a list.
	if(islist || islistdrop)
	{
		SendMessage(m_pgvcurcolnode->hwndCombo, message, wparam, lparam);
		return 0;
	}
	
	EndLabelEdit(m_curselrow, m_curselcol);
    if(m_flip == wyTrue)
    {
        if(m_curselcol == 0)
            return 0;
    }
    else
    {
	    if(m_curselrow == 0)
		    return 0;
    }

    if(m_flip == wyFalse && !m_lpgvwndproc(hwnd, GVN_ROWCHANGINGTO, m_curselrow - 1, m_curselcol))
    {
        return 0;
    }

	ret = m_lpgvwndproc(hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);
	if(!ret)
		return 0;

    if(m_flip == wyTrue)
        m_curselcol--;
    else
	    m_curselrow--;

	m_lpgvwndproc(hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
	VERIFY(InvalidateRect(hwnd, NULL, FALSE));
	return 0;
}

wyInt32 
CCustGrid::OnEditProcVkDown(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist, wyInt32 islistdrop)
{
    RECT	rectsub, rectwin;
    wyInt32 ret;

	// now we have to see if its a list.
    if(islist || islistdrop)
	{
		SendMessage(m_pgvcurcolnode->hwndCombo, message, wparam, lparam);
		return 0;
	}
	
	memset(&rectsub, 0, sizeof(RECT));
	memset(&rectwin, 0, sizeof(RECT));

	VERIFY( GetClientRect(GetHwnd(), &rectwin));

	EndLabelEdit(m_curselrow, m_curselcol);

    if(m_flip != wyTrue)
    {
    	if(m_curselrow ==(m_row) - 1)
	    	return InsertNewRowOnKeyPress(hwnd);
    }

    if(m_flip == wyFalse && !m_lpgvwndproc(hwnd, GVN_ROWCHANGINGTO, m_curselrow + 1, m_curselcol))
    {
        return 0;
    }

	ret = m_lpgvwndproc(hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);
	if(!ret)
		return 0;

	if(m_flip == wyTrue)
    {
        if(m_curselcol != (m_col -1))
	    	m_curselcol++;
    }
    else
    {
	    if(m_curselrow != (m_row -1))
	    	m_curselrow++;
    }

	GetSubItemRect(m_curselrow, m_curselcol, &rectsub);

	if(rectsub.bottom + m_hight > rectwin.bottom)
		m_initrow++;

    m_lpgvwndproc(hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    VERIFY(InvalidateRect(hwnd, NULL, FALSE));
    return 0;
}

wyInt32 
CCustGrid::OnEditProcVkPrior(wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist)
{
	if(islist)	
    {
		SendMessage(m_pgvcurcolnode->hwndCombo, message, wparam, lparam);
		return 0;
	}
    else 
		CalculatePageUp(wyTrue);

    return 1;
}

wyInt32 
CCustGrid::OnEditProcVkNext(wyUInt32 message, WPARAM wparam, LPARAM lparam, wyInt32 islist)
{
// now we have to see if its a list.
	if(islist)
    {
		SendMessage(m_pgvcurcolnode->hwndCombo, message, wparam, lparam);
		return 0;
	}
    else 
		CalculatePageDown(wyTrue);

    return 1;
}


wyInt32 
CCustGrid::OnEditProcWmKeyDown(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam, 
                               wyInt32 islist, wyInt32 islistdrop, wyInt32 *retval)
{
    *retval = 1;

	switch(wparam)
	{	
	case VK_RETURN:
        OnEditProcVkReturn();
		return 0L;

	case VK_UP:
        return OnEditProcVkUp(hwnd, message, wparam, lparam, islist, islistdrop);

	case VK_DOWN:
        return OnEditProcVkDown(hwnd, message, wparam, lparam, islist, islistdrop);

	case VK_ESCAPE:
		m_lpgvwndproc(m_hwnd,GVN_CANCELLABELEDIT,0,0);
		ProcessEscPress();
		return 1;

	case VK_PRIOR:
        if(OnEditProcVkPrior(message, wparam, lparam, islist) == 0)
            return 0;
		break;			

	case VK_NEXT:
		if(OnEditProcVkNext(message, wparam, lparam, islist) == 0)
            return 0;
		break;			
    }

    *retval = 0;
    return  1;
}


LRESULT
CCustGrid::ComboWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	WNDPROC			origproc;
    CCustGrid*      pcg;

	VERIFY(origproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch(msg)
	{
	case WM_GETDLGCODE:
		return DLGC_WANTMESSAGE;

	case WM_KILLFOCUS:
		break;

    case WM_KEYDOWN:
        pcg = (CCustGrid*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
        
        if(wparam == VK_SPACE && GetKeyState(VK_CONTROL) & SHIFTED && IsWindowVisible(pcg->m_hwndbrowsebutton))
        {
            pcg->m_lpgvwndproc(pcg->m_hwnd, GVN_BROWSEBUTTONCLICK, (WPARAM)pcg->m_curselrow, (LPARAM)pcg->m_curselcol);
            return 0;
        }
	}

	// you cannot subclass this as the editwndproc as all the combo box uses the same origproc to work
	return CallWindowProc(origproc, hwnd, msg, wparam, lparam);
}

LRESULT	CALLBACK 
CCustGrid::BrowseButtonWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    CCustGrid* pcustgrid;
    pcustgrid = (CCustGrid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    RECT rect = {0};
    POINT pt = {0};

    switch(msg)
    {
        case WM_KEYDOWN:
            pcustgrid->m_ispressed = wyTrue;
            break;

        case WM_KEYUP:

            if(pcustgrid->m_ispressed == wyFalse)
            {
                break;
            }

            pcustgrid->m_ispressed = wyFalse;

            if(wparam == VK_ESCAPE)
            {
                pcustgrid->ProcessEscPress();
            }
            else if(wparam == VK_SPACE)
            {
                pcustgrid->m_lpgvwndproc(pcustgrid->m_hwnd, GVN_BROWSEBUTTONCLICK, (WPARAM)pcustgrid->m_curselrow, (LPARAM)pcustgrid->m_curselcol);
            }
            break;            

        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            pcustgrid->m_ispressed = wyTrue;
            break;

        case WM_LBUTTONUP:
            ReleaseCapture();
            GetCursorPos(&pt);
            GetWindowRect(hwnd, &rect);

            if(pcustgrid->m_ispressed == wyFalse || !PtInRect(&rect, pt))
            {
                break;
            }

            pcustgrid->m_ispressed = wyFalse;
            pcustgrid->m_lpgvwndproc(pcustgrid->m_hwnd, GVN_BROWSEBUTTONCLICK, (WPARAM)pcustgrid->m_curselrow, (LPARAM)pcustgrid->m_curselcol);
    }

    return CallWindowProc(pcustgrid->m_wporigbrowsebuttonwndproc, hwnd, msg, wparam, lparam);
}

LRESULT
CCustGrid::OnCreate(LPARAM lparam)
{    
	wyInt32     height, width;
	RECT		rect;
	SCROLLINFO	scf;

	memset(&scf, 0, sizeof(SCROLLINFO));

	VERIFY(height = GetSystemMetrics(SM_CYHSCROLL));
	VERIFY(width  = GetSystemMetrics(SM_CYVSCROLL));

	VERIFY(GetClientRect(m_hwnd, &rect));
	
	VERIFY(m_hwndedit	= CreateWindowEx(NULL, L"edit", NULL, WS_CHILD | ES_AUTOHSCROLL, 
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 10,
							m_hwnd,(HMENU)0, GetModuleHandle(0), 0));
	
	// change the font.
	SendMessage(m_hwndedit, WM_SETFONT,(WPARAM)m_hfont, TRUE);

    VERIFY(m_hwndbrowsebutton	= CreateWindowEx(NULL, L"button", NULL, WS_CHILD, 
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 10,
							m_hwnd,(HMENU)0, GetModuleHandle(0), 0));

    SendMessage(m_hwndbrowsebutton, WM_SETFONT,(WPARAM)m_hfont, TRUE);
    SetWindowText(m_hwndbrowsebutton, BROWSEBUTTONTEXT);

	VERIFY(m_hwndsplitter = CreateWindowEx(NULL, splitter, L"Splitter", WS_CHILD, CW_USEDEFAULT, 
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
									        m_hwnd,(HMENU)0, GetModuleHandle(0), this));

	// Now subclass the edit box.
	m_wporigeditwndproc = (WNDPROC)SetWindowLongPtr(m_hwndedit, GWLP_WNDPROC,(LONG_PTR)CCustGrid::EditWndProc);
	SetWindowLongPtr(m_hwndedit, GWLP_USERDATA, (LONG_PTR)this);

    m_wporigbrowsebuttonwndproc = (WNDPROC)SetWindowLongPtr(m_hwndbrowsebutton, GWLP_WNDPROC, (LONG_PTR)CCustGrid::BrowseButtonWndProc);
    SetWindowLongPtr(m_hwndbrowsebutton, GWLP_USERDATA, (LONG_PTR)this);

	return 0;
}

LRESULT
CCustGrid::OnWMSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    return m_lpgvwndproc(hwnd, GVN_SYSKEYDOWN, wparam, lparam);
}

LRESULT
CCustGrid::OnWMCommand(WPARAM wparam, LPARAM lparam)
{
    wyInt32 index, nlen;
	HWND	hwndlist;
	wyWChar	text[256];

	if(HIWORD(wparam) == LBN_SELCHANGE)
    {
		wyInt32 index, len;
		HWND	hwndlist;
		wyWChar	text[256];
		
		hwndlist = (HWND)lparam;

		index = SendMessage(hwndlist, LB_GETCURSEL, 0, 0);

		if(index == LB_ERR)
			return 1;

		SendMessage(hwndlist, LB_GETTEXT, index,(LPARAM)text);
		SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)text);
		len = wcslen(text);
		SendMessage(m_hwndedit, EM_SETSEL, len, len);
		SetFocus(m_hwndedit);

		//This required to paint the listbox drop-down arrow correctly.
		//ApplyChanges();

		//When listbox item is changed
		m_lpgvwndproc(m_hwnd, GVN_LBITEMCHANGED, MAKELONG(m_curselrow, m_curselcol), NULL);		
	} 
    else if(HIWORD(wparam)== LBN_DBLCLK)	
    {
		hwndlist = (HWND)lparam;

		VERIFY((index = SendMessage(hwndlist, LB_GETCURSEL, 0, 0)) != LB_ERR);
		SendMessage(hwndlist, LB_GETTEXT, index, (LPARAM)text);
		SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)text);

        nlen = wcslen(text);

		EndLabelEdit(m_curselrow, m_curselcol);
		VERIFY(InvalidateRect(m_hwnd, NULL, FALSE));
	} 

	return 1;
}

LRESULT
CCustGrid::OnWMNotify(WPARAM wparam, LPARAM lparam)
{
	LPNMLVKEYDOWN	lpnmlv =(LPNMLVKEYDOWN)lparam;
	LPNMTTDISPINFO  lpnmttd = (LPNMTTDISPINFO)lparam;
	LONG			ret;
	wyString		moldcoltextstr;
	wyInt32			x;
	
	if(lpnmlv->hdr.code == LVN_KEYDOWN)
    {
		if(lpnmlv->wVKey == VK_TAB || lpnmlv->wVKey == VK_RETURN)
        {
			ret = EndLabelEdit(m_curselrow, m_curselcol);
			if(!ret)
				return 0;

			ProcessTabPress();
			m_iscomboediting = wyFalse;
			ShowWindow(lpnmlv->hdr.hwndFrom, FALSE);
			InvalidateRect(m_hwnd, NULL, TRUE);
		} 
        else if(lpnmlv->wVKey == VK_ESCAPE)
        {
			ret = EndLabelEdit(m_curselrow, m_curselcol);
			if(!ret)
				return wyFalse;

			if(GetOwnerData() && wcsstr(m_oldcoltext, TEXT(STRING_NULL)))
				SetSubItemText(m_curselrow, m_curselcol, 0);			
			else
			{
				moldcoltextstr.SetAs(m_oldcoltext);
				SetSubItemText(m_curselrow, m_curselcol, moldcoltextstr.GetString());
			}

			m_iscomboediting = wyFalse;
			ShowWindow(m_hwndcurcombo, FALSE);
			//ProcessTabPress();
		} 
	} else if(lpnmlv->hdr.code == TTN_GETDISPINFO) {
		x = GetHoveredColumn();
		lpnmttd->szText[0] = '\0';

	    if(x != -1)
		{
			m_lpgvwndproc(m_hwnd, GVN_TOOLTIP, x, (LPARAM)&lpnmttd->lpszText);
		}
	}

	return 0;
}

LRESULT
CCustGrid::SetLongData(LPARAM lparam)
{
	m_lparamdata = lparam;
	return 1;
}

wyBool
CCustGrid::CreateFonts()
{
	HDC	        dc;
	DWORD       fontheight;
	wyString    fontname;
	wyString	dirstr, fontnamestr;

    fontname.SetAs("Verdana");

	dc = GetDC(GetParent(m_hwnd));
    fontheight = -MulDiv(FONT_BASE_HEIGHT, GetDeviceCaps(dc, LOGPIXELSY), FONT_DIV_FACTOR);
	VERIFY(ReleaseDC(GetParent(m_hwnd), dc));
	
	if(m_hfont)
		DeleteObject(m_hfont);

	if(m_htopfont)
		DeleteObject(m_htopfont);

    if(m_hItalicsFont)
        DeleteObject(m_hItalicsFont);

	// get the font name from the .ini file.
	// now search the font given in the .ini.
	// Get the complete path.
#ifdef DEF_PGLOBAL
    wyWChar		directory[MAX_PATH+1] = {0}, *lpfileport = 0;

	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;
	
	dirstr.SetAs(directory);
	wyIni::IniGetString(SECTION_NAME, "Font", "Verdana", &fontname, SIZE_128-1, dirstr.GetString());
#endif
	fontnamestr.SetAs(fontname);
	VERIFY(m_hfont = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, fontnamestr.GetAsWideChar()));
	VERIFY(m_hItalicsFont = CreateFont(fontheight, 0, 0, 0, 0, TRUE, 0, 0, 0, 0, 0, 0, 0, fontnamestr.GetAsWideChar()));
    VERIFY(m_htopfont = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, fontnamestr.GetAsWideChar()));

	return wyTrue;
}

void
CCustGrid::SetGridFont(PLOGFONT lf)
{
    PGVCOLNODE  colnode;
    LOGFONT tempFont = *(lf);
    tempFont.lfItalic = TRUE;

	if(m_hfont)
		DeleteObject(m_hfont);

    if(m_hItalicsFont)
        DeleteObject(m_hItalicsFont);

    if(lf->lfHeight <= -16)
        m_hight = -(lf->lfHeight - 2);
    else
        m_hight = 18;

	VERIFY(m_hfont = CreateFontIndirect(lf));
    VERIFY(m_hItalicsFont = CreateFontIndirect(&tempFont));


	/// Sets the font width for the field name (heading) in the grid window
	if(m_htopfont)
	{
		DeleteObject(m_htopfont);
		//lf->lfWeight = FW_SEMIBOLD;

		VERIFY(m_htopfont = CreateFontIndirect(lf));
	}

	SetWindowFont(m_hwndedit, m_hfont, FALSE);
	SetWindowFont(m_hwnd, m_hfont, FALSE);	
    //SetWindowFont(m_hwndbrowsebutton, m_hfont, FALSE);	

    colnode = m_collist;

	while(colnode)
	{
		if(colnode->hwndCombo)
			SetWindowFont(colnode->hwndCombo, m_hfont, FALSE);

		colnode = colnode->pNext;
	}
    
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);

	return;
}

wyUInt32
CCustGrid::GetMaxTextWidth(PGVCOLNODE pgvnode)
{
    wyUInt32    maxtextwidth = 0;
    PGVCOLNODE  node;
    
    node = pgvnode;

    while(node)
    {
        if(maxtextwidth < strlen(node->pColumn.text))
            maxtextwidth = strlen(node->pColumn.text);

        node = node->pNext;
    }

    return maxtextwidth;
}

wyUInt32
CCustGrid::GetMaxWidth(PGVCOLNODE pgvnode)
{
    wyInt32     maxwidth = 0;
    PGVCOLNODE  node;
    
    node = pgvnode;

    while(node)
    {
        if(maxwidth < node->pColumn.cx)
            maxwidth = node->pColumn.cx;

        node = node->pNext;
    }

    return maxwidth;
}

void
CCustGrid::SetMaxWidth(wyInt32 width)
{
    m_maxwidth = width;
    return;
}

void
CCustGrid::SetRowWidth()
{
    PGVROWNODE pgvrownode = m_rowlist;

    while(m_rowlist != NULL)
    {
        m_rowlist->rowcx = m_maxwidth;
        m_rowlist = m_rowlist->pNext;
    }

    m_rowlist = pgvrownode;
}

void
CCustGrid::DrawInitialButton(PGVCOLNODE pgvnode, HDC hdcmem, RECT *rect)
{   
	TRIVERTEX vertex[2];
    LONG style, side = 0;
    RECT rect2;
    GVINITIALBUTTONINFO info = {0};

	//if no query executed or no table is selected in OB then no need to draw the initial buttonin Grid.
	if(!pgvnode)
		return;

	if(m_flip == wyTrue)
    {    
        if(m_maxwidth == 0)
        {
            m_maxwidth  = GetMaxWidth(pgvnode);
            
            if(pgvnode && m_maxwidth == NULL)
                m_maxwidth = pgvnode->pColumn.cx;

            SetRowWidth();
        }

        if(m_maxwidth > GV_DEFWIDTH)
            rect->right = m_maxwidth;
        else
            rect->right = GV_DEFWIDTH;
    }
    VERIFY(DrawFrameControl(hdcmem, rect, DFC_BUTTON, DFCS_BUTTONPUSH|DFCS_FLAT));
    
	
	
	vertex[0].x     = rect->left + 2;
	vertex[0].y     = rect->top + 2;

	vertex[1].x     = rect->right - 2;
	vertex[1].y     = rect->bottom - 2; 

	vertex[0].Red   = 0xffffff;
	vertex[0].Green = 0xffffff;
	vertex[0].Blue  = 0xffffff;
	vertex[0].Alpha = 0xffffff;

	vertex[1].Red   = 0xffffff;
	vertex[1].Green = 0xffffff;
	vertex[1].Blue  = 0xffffff;
	vertex[1].Alpha = 0xffffff;

	//Gradient for button at lef-top most
	SetGradient(hdcmem, vertex);
    if(m_exstyle & GV_EX_ROWCHECKBOX)
    {
     
        SetSelAllState();
        
        rect->top	 += 3;
	    rect->bottom  -= 3;
        rect->left  += 3;
        rect->right  -= 3;
        
        style = DFCS_BUTTONCHECK| DFCS_FLAT|DFCS_ADJUSTRECT;


        

		if(!GetOwnerData())
        {
            info.checkstate = m_selallinfo.checkstate;
        }
        
        m_lpgvwndproc(m_hwnd, GVN_DRAWSELECTALL, 0, (LPARAM)&info);
        m_selallinfo.checkstate = info.checkstate;
		
        
        //rect2.top =  rect->top + 3;
        //    rect2.bottom = rect->bottom - 3;
        //    rect2.left = rect->left + 6;
        //    rect2.right = rect->right - 7;

        if(m_selallinfo.checkstate&BST_PUSHED)
            DrawFrameControl(hdcmem, rect, DFC_BUTTON, style|DFCS_PUSHED);
        else if(m_selallinfo.checkstate == BST_CHECKED)
            DrawFrameControl(hdcmem, rect, DFC_BUTTON, style|DFCS_CHECKED);
        else if(m_selallinfo.checkstate == BST_INDETERMINATE)
        {
            DrawFrameControl(hdcmem, rect, DFC_BUTTON, style);
            if((rect->bottom - rect->top) > (rect->right - rect->left))
            {
                side = (rect->right - rect->left);
            }
            else
            {
                side = (rect->bottom - rect->top);
            }
            
            if(side % 2 == 0)
            {
                side -=8;
            }
            else
            {
                side -=7;
            }

            if((rect->bottom - rect->top) > 12)
            {
                rect2.top = (rect->bottom - rect->top) % 2 == 0 ? (rect->bottom + rect->top)/2 - side/2 - 1 : (rect->bottom + rect->top)/2 - side/2;
                rect2.bottom = (rect->bottom - rect->top) % 2 == 0 ?(rect->bottom + rect->top)/2 + side/2 :(rect->bottom + rect->top)/2 + side/2 + 1;
                rect2.left = (rect->left + rect->right)/2 - side/2;
                rect2.right = (rect->right + rect->left)/2 + side/2 + 1;
            }
            else
            {
                rect2.top =  rect->top + 3;
                rect2.bottom = rect->bottom - 3;
                rect2.left = rect->left + 6;
                rect2.right = rect->right - 7;
            }
            HBRUSH hbrush = GetStockBrush(DKGRAY_BRUSH);
            FillRect(hdcmem, &rect2, hbrush);
        }
        else
            DrawFrameControl(hdcmem, rect, DFC_BUTTON, style);


    }
    
}

wyInt32
CCustGrid::DrawColumns(RECT *rect, RECT *rectwin, HDC hdcmem, wyUInt32 *totcx, wyUInt32 *totcy)
{
	wyInt32 colcount = 0;

    *totcy += m_hight;

	/// Initialize the coordinates.
	if(m_flip == wyTrue)
    {
        *totcx += m_maxwidth;
        rect->top = m_hight;
		
    }
    else
    {
        *totcx += GV_DEFWIDTH;
        rect->left = GV_DEFWIDTH;
	    rect->top = 0;
    }
	
	colcount = DrawTopColumns(hdcmem, rect, rectwin, totcx);

	return colcount;
}

wyInt32
CCustGrid::DrawRows(RECT *rect, RECT *rectwin, HDC hdcmem, wyUInt32 *totcx, wyUInt32 *totcy, HFONT hfontold)
{
    PGVROWNODE  pgvnode = m_rowlist;
    wyInt32     count   = 0;

    if(m_flip == wyTrue)
    {
        *totcx = m_maxwidth;
        rect->left = m_maxwidth;
        rect->top = 0;
        while(pgvnode && count< m_initrow)
        {
            pgvnode = pgvnode->pNext;
            count++;
        }
        if(pgvnode)
        {
            rect->right = rect->left + pgvnode->rowcx;
	        rect->bottom = rect->top + m_hight;
        }
    }
    else
    {
        rect->left = 0;
	    rect->top = m_hight;
	    rect->right = GV_DEFWIDTH;
	    rect->bottom = rect->top + m_hight;
    }

	hfontold = (HFONT)SelectObject(hdcmem,(HGDIOBJ)m_hfont);

    return DrawRowColumns(hdcmem, rect, rectwin, totcx, totcy);
}

LRESULT
CCustGrid::OnPaint(WPARAM wparam, LPARAM lparam)
{
	HDC			hdc, hdcmem;
	wyUInt32    totcx = 0, totcy = 0;
	RECT		rect, rectwin;
	LONG		left = 0, top = 0;
	HFONT		hfontold = NULL;		
	HBRUSH		hbrush;
	HBITMAP		hbmmem, hbmold;
	PGVCOLNODE	pgvnode;
	PGVROWNODE	pgvrownode;
	PAINTSTRUCT ps;
	wyInt32		rowscrolllimit = 0, colscrolllimit = 0;

    if(m_isvisible != wyTrue)
        return 0;

	// Get the device context for this window and prepare it for painting.
	VERIFY(hdc = BeginPaint(m_hwnd, &ps));

	// Get the rect of the window and also the dimenstions of some window..
    VERIFY(GetClientRect(m_hwnd, &rectwin));

	// Create a memory dc and compatible bitmap so that we can create the image in the
	// memory and then paINT it. This is supposed to stop flickering.
	VERIFY(hdcmem = CreateCompatibleDC(hdc));
	VERIFY(hbmmem = CreateCompatibleBitmap(hdc, rectwin.right-rectwin.left, rectwin.bottom-rectwin.top));

	// Select the bitmap INTo the offscreen DC.
    hbmold = (HBITMAP)SelectObject(hdcmem, hbmmem);		// not applicable

	// Create a white brush.
	VERIFY(hbrush = CreateSolidBrush(m_crbkgnd));

	// Erase the background.
	VERIFY(FillRect(hdcmem, &rectwin, hbrush));
	// Now by default we put a box for the row in the left.
	rect.left = left;
	rect.top  = top;
	rect.right = GV_DEFWIDTH;
	rect.bottom = rect.top + m_hight;

    pgvnode = m_collist;

    /// Drawing the initial button control on the top most left hand side.
    /// If the filp is on, the max width is calculated and assign to 
    /// all the column    
    DrawInitialButton(pgvnode, hdcmem, &rect);

	pgvrownode = m_rowlist;

	hfontold = (HFONT)SelectObject(hdcmem,(HGDIOBJ)m_hfont);
    // now we draw columns.
	// we know how many column to draw by getting traversing the collist linked list.
    colscrolllimit = DrawColumns(&rect, &rectwin ,hdcmem, &totcx, &totcy);
    
    ///now we traverse through the rows and add rows and columns.
    rowscrolllimit = DrawRows(&rect, &rectwin ,hdcmem, &totcx, &totcy, hfontold);
	
	//To get correct hight of grid if Flip grid
	if(m_flip == wyTrue)
	{
		totcy = m_hight;
		totcy = m_col * m_hight;		
	}
	
	m_tox = totcx;

	hfontold = (HFONT)SelectObject(hdcmem,(HGDIOBJ)hfontold);

	VERIFY(BitBlt(hdc, rectwin.left, rectwin.top, rectwin.right-rectwin.left, rectwin.bottom-rectwin.top, hdcmem, 0, 0, SRCCOPY));
	VERIFY(DeleteObject(hbrush));

	SelectObject(hdcmem, hbmold);
    VERIFY(DeleteObject(hbmmem));
	VERIFY(DeleteDC(hdcmem));
	
	//ShowHideScrollBar(totcx, totcy, rectwin);	
	// Set the scroll bar limits
    ShowHideScrollBar(totcx, totcy, rectwin);
	SetScrollBarLimits(rowscrolllimit > 0 ? rowscrolllimit : 5, colscrolllimit, &rectwin, totcx, totcy);
    
			
	EndPaint(m_hwnd, &ps);	// no ret

	// now if the current selected row and current selected col is of list type.
	// then we drop down the list and start editing.
	if(m_isediting == wyTrue)
		SetFocus(m_hwndedit); // no ret
	else if(m_iscomboediting == wyTrue)
    {
		SetFocus(m_hwndcurcombo);
		ShowWindow(m_hwndcurcombo, TRUE);
	}

	return 1;
}

void 
CCustGrid::SetScrollBarLimits(wyUInt32 nrow, wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy)
{
	if(m_flip == wyTrue)
	{
		m_vscrollpos = m_initrow * m_maxwidth;
		m_hscrollpos = m_initcol * m_hscrollnumpos; 
		SetScrollBarLimitsFlipGrid(nrow, ncol, rectwin, totcx, totcy);
	}

	else
	{
		m_vscrollpos = m_initrow * GV_SCROLLPOS; 
		m_hscrollpos = m_initcol * m_hscrollnumpos;
		SetScrollBarLimitsNonFlipGrid(ncol, rectwin, totcx, totcy);	
	}
}

//Non flip gird, The Vertical scroll bar meant for rows and Horizontal scroll bar for columns
void
CCustGrid::SetScrollBarLimitsNonFlipGrid(wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy)
{
    wyInt32 numrowspage = 0, nrem = 0, npages = 0, rowhtpercentage = 0, vmaxrange = 0, rowpos = 0;
	wyInt32 hnrem = 0, hnpages = 0, hmaxrange = 0, maxwidth = totcx, hremcolper = 0; 
	SCROLLINFO	scinfo;
	PGVCOLNODE	pgvnode = NULL;
	wyUInt32    totalwidth = 0, skicolcount = 0;
		
	numrowspage = ((rectwin->bottom - rectwin->top) / m_hight);

	if(numrowspage)
    {
		npages = (m_row + 1) / numrowspage;
		nrem = (m_row + 1) % numrowspage;

		rowhtpercentage = GV_SCROLLPOS;

		vmaxrange = (rowhtpercentage * numrowspage) * npages + nrem * rowhtpercentage;

		m_vscrollthumwidth = rowhtpercentage * numrowspage;

		m_vscrollmaxrange = vmaxrange;
	}
				
	if(m_col)
	{
		pgvnode = m_collist;
		totalwidth = totcx;// + 25;
			
		for(pgvnode; pgvnode; pgvnode = pgvnode->pNext)
		{
			if(totalwidth <= (rectwin->right - rectwin->left))
				break;
				        
			skicolcount ++;

			if(skicolcount == m_col - 1)
				break;

			totalwidth -= pgvnode->pColumn.cx;
		} 

		m_hskipcolcount = skicolcount;

		if(rectwin->right - rectwin->left > 0)
		{		
			hnpages = maxwidth / (rectwin->right - rectwin->left);
			hnrem = maxwidth % (rectwin->right - rectwin->left);

			if(hnrem)
			{
				hnpages += 1;
				hremcolper = MulDiv(hnrem, 100, maxwidth);
			}

			hmaxrange = maxwidth;

			if(hnpages)
				m_hscrollthumwidth = maxwidth / hnpages;
			if(hnrem)
				m_hscrollthumwidth += MulDiv(m_hscrollthumwidth, (100 - hremcolper), 100);
						
			if(m_col)
				m_hscrollnumpos = (maxwidth - m_hscrollthumwidth) / m_col;

			hmaxrange = m_hscrollthumwidth + (m_hscrollnumpos * (m_col - 1));

			m_hscrollthumwidth = m_hscrollthumwidth + ((m_col - skicolcount - 1) * m_hscrollnumpos);

			m_hscrollmaxrange = hmaxrange;
		}
	}
		
	scinfo.cbSize =  sizeof(SCROLLINFO);
	scinfo.fMask = SIF_ALL;
	GetScrollInfo(m_hwnd, SB_VERT, &scinfo);

	rowpos = scinfo.nPos / GV_SCROLLPOS;
	
    if( totcy > rectwin->bottom - rectwin->top || m_initrow != 0)
    {
        // set the scroll bar position for the vertical scroll box.
	    scinfo.nPos = m_vscrollpos;
	    scinfo.nMin = 0;
	    scinfo.nMax = vmaxrange;
	    scinfo.nPage = m_vscrollthumwidth;
	    SetScrollInfo(m_hwnd, SB_VERT, &scinfo, TRUE);	    
    }

	// set the scroll bar position for the horizontal scroll box.
	scinfo.cbSize =  sizeof(SCROLLINFO);
	scinfo.fMask = SIF_ALL;
	GetScrollInfo(m_hwnd, SB_HORZ, &scinfo);

    if(totcx > rectwin->right - rectwin->left || m_initcol != 0)
    {
	    scinfo.nPos = m_hscrollpos;
	    scinfo.nMin = 0;
	    scinfo.nMax = hmaxrange;
	    scinfo.nPage = m_hscrollthumwidth;
	    SetScrollInfo(m_hwnd, SB_HORZ, &scinfo, TRUE);				
    }
}

void	
CCustGrid::SetScrollBarLimitsFlipGrid(wyUInt32 nrow, wyUInt32 ncol, RECT *rectwin, wyUInt32 totcx, wyUInt32 totcy)
{
	PGVCOLNODE 	pgvnode = NULL;
	PGVROWNODE	pgrownode = NULL;
	wyInt32		numrowspage = 0, nrem = 0, npages = 0; 
	wyInt32		rowhtpercentage = 0, vmaxrange = 0, rowpos = 0, total = 0l, skiprows = 0;
	wyInt32		hnrem = 0, hnpages = 0, hmaxrange = 0, maxwidth = totcy, hremcolper = 0; 
	wyUInt32    totalwidth, skicolcount = 0;
	SCROLLINFO	scinfo;

	total = totcx;

	if(m_maxwidth)
		numrowspage = ((rectwin->right - rectwin->left) / m_maxwidth) - 1;

	if(numrowspage)
	{
		npages = (m_row + 1) / numrowspage;
		nrem = (m_row + 1) % numrowspage;

		rowhtpercentage = m_maxwidth;

		vmaxrange = (rowhtpercentage * numrowspage) * npages + nrem * rowhtpercentage;

		m_vscrollthumwidth = rowhtpercentage * numrowspage;

		m_vscrollmaxrange = vmaxrange;

		pgrownode = m_rowlist;

		total = m_maxwidth;
		skiprows = 0;

		for(pgrownode; pgrownode; pgrownode = pgrownode->pNext)
		{
			if(total >= (rectwin->right - rectwin->left))
			{
				skiprows--;
				break;
			}
				            
			skiprows ++;

			if(skiprows == m_row - 1)
			{
				skiprows--;
				break;
			}

			total += pgrownode->rowcx;				
		}
        
		m_vskiprows = skiprows;		
	}		
				
	if(m_col)
	{
		pgvnode = m_collist;
		totalwidth = totcy;
		
		for(pgvnode; pgvnode; pgvnode = pgvnode->pNext)
		{
			if(totalwidth <= ((rectwin->bottom - rectwin->top)))
				break;
			        
			skicolcount ++;

			if(skicolcount == m_col - 1)
				break;

			totalwidth -= m_hight;			
		} 

		m_hskipcolcount = skicolcount;

		if(rectwin->bottom - rectwin->top > 0)
		{
			hnpages = maxwidth / (rectwin->bottom - rectwin->top);
			hnrem = maxwidth % (rectwin->bottom - rectwin->top);
		}

		if(hnrem)
		{
			hnpages += 1;
			hremcolper = MulDiv(hnrem, 100, maxwidth);
		}


		if(hnpages)
		{
			hmaxrange = maxwidth;
			m_hscrollthumwidth = maxwidth / hnpages;
			if(hnrem)
				m_hscrollthumwidth += MulDiv(m_hscrollthumwidth, (100 - hremcolper), 100);
							
			m_hscrollnumpos = m_hight;

			hmaxrange = m_hscrollthumwidth + (m_hscrollnumpos * (m_col - 1));

			m_hscrollmaxrange = hmaxrange;
		}
	}
	
	scinfo.cbSize =  sizeof(SCROLLINFO);
	scinfo.fMask = SIF_ALL;
	GetScrollInfo(m_hwnd, SB_HORZ, &scinfo);

	rowpos = scinfo.nPos / m_hight;
		
	    // set the scroll bar position for the vertical scroll box.
	if(numrowspage && nrow > 0)
	{
		scinfo.nPos = m_vscrollpos;
		scinfo.nMin = 0;
		scinfo.nMax = vmaxrange;
		scinfo.nPage = m_vscrollthumwidth;
		SetScrollInfo(m_hwnd, SB_HORZ, &scinfo, TRUE);			
	}

	    // set the scroll bar position for the horizontal scroll box.
	if(m_col - 1 > 0)
	{
		scinfo.cbSize =  sizeof(SCROLLINFO);
		scinfo.fMask = SIF_ALL;
		GetScrollInfo(m_hwnd, SB_VERT, &scinfo);

		scinfo.nPos = m_hscrollpos;
		scinfo.nMin = 0;
		scinfo.nMax = hmaxrange;
		scinfo.nPage = m_hscrollthumwidth;
		SetScrollInfo(m_hwnd, SB_VERT, &scinfo, TRUE);			
    }
}

void
CCustGrid::ShowHideScrollBar(wyUInt32 totcx, wyUInt32 totcy, RECT rectwin)
{
	if(m_flip == wyTrue)
    {
        //if(totcy > (wyUInt32)rectwin.bottom)
		if(m_row - 1 > 0 && totcx > (wyUInt32)rectwin.right)
		    VERIFY(ShowScrollBar(m_hwnd, SB_HORZ, TRUE)); 
	    else if(m_initrow == 0)
		    VERIFY(ShowScrollBar(m_hwnd, SB_HORZ, FALSE));  
	
        //if(m_colscrolllimit > 0)
		if(m_col - 1 > 0 && ((m_col + 1) * m_hight) > (wyUInt32)rectwin.bottom)
		    VERIFY(ShowScrollBar(m_hwnd, SB_VERT, TRUE));
	    else if(m_initcol == 0)
		    VERIFY(ShowScrollBar(m_hwnd, SB_VERT, FALSE));
    }
    else
    {
		if(m_row - 1 > 0 && totcy > (wyUInt32)rectwin.bottom - rectwin.top)//GetSystemMetrics(SM_CYHSCROLL))
		    VERIFY(ShowScrollBar(m_hwnd, SB_VERT, TRUE));
	    else if(m_initrow == 0)
		    VERIFY(ShowScrollBar(m_hwnd, SB_VERT, FALSE));

	    if(m_col - 1 > 0 && totcx > (wyUInt32)rectwin.right)
		    VERIFY(ShowScrollBar(m_hwnd, SB_HORZ, TRUE)); 
	    else if(m_initcol == 0)
		    VERIFY(ShowScrollBar(m_hwnd, SB_HORZ, FALSE));		
    }
}

PGVCOLNODE
CCustGrid::SkipColumns(PGVCOLNODE pgvnode, RECT *rectcolcount, PRECT rectwin, wyUInt32 *totcx, wyInt32 *ncol)
{
    if(m_flip == wyTrue)
	{
        rectcolcount->right += m_maxwidth;
		*totcx += m_maxwidth;
    }
    else
    {
        rectcolcount->right += pgvnode->pColumn.cx;
	    *totcx += pgvnode->pColumn.cx;
    }

	if(*totcx > (wyUInt32)rectwin->right)
		*ncol++;

	pgvnode = pgvnode->pNext;

    return pgvnode;
}

void    
CCustGrid::DrawButtonForColumn(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rect, wyUInt32 *totcx, wyInt32 *totviscx, PRECT rectwin, wyInt32 col)
{
    wyUInt32	state = DFCS_BUTTONPUSH | DFCS_FLAT ;
	TRIVERTEX	vertex[2];

    rect->bottom = rect->top + m_hight;
		
    if(m_flip == wyTrue)
    {
        rect->right = rect->left + m_maxwidth;
		*totcx += m_maxwidth;
        *totviscx += m_maxwidth;
		rect->top=rect->top-1;
    }
    else
    {	
        if(pgvcol)
        {
		    //If single column visible then its max width limited to 'right of grid - 25'
		    if(rectwin->right > 75 && pgvcol->cx >= (rectwin->right - 35) && rect->left == 25)
			    pgvcol->cx = rectwin->right - 35;			

            rect->right = rect->left + pgvcol->cx;
			rect->left=rect->left-1;//fixing extra displacement of top cell
		    *totcx += pgvcol->cx;
            *totviscx += pgvcol->cx-1;		
        }
        else
        {
            rect->right = rectwin->right;
        }
    }
	
	vertex[0].x     = rect->left + 2;
	vertex[0].y     = rect->top + 2;

	vertex[1].x     = rect->right - 2;
	vertex[1].y     = rect->bottom - 2; 

	vertex[0].Red   = 0xc900;
	vertex[0].Green = 0xc600;
	vertex[0].Blue  = 0xb800;
	vertex[0].Alpha = 0x0000;

	vertex[1].Red   = 0xc900;
	vertex[1].Green = 0xc600;
	vertex[1].Blue  = 0xb800;
	vertex[1].Alpha = 0x0000;
		
	DrawFrameControl(hdcmem, rect, DFC_BUTTON, state);

	//Column header gradient
	//SetGradient(hdcmem, vertex, wyFalse);
}

void
CCustGrid::DrawColumnIcons(HDC hdcmem, wyInt32 *ncurcol, RECT *recttemp)
{
	wyInt32     iconid = 0;
	HICON       icon;

	/// If pGlobals defined
#ifndef DEF_PGLOBAL

	iconid = m_lpgvwndproc(m_hwnd, GVN_COLUMNDRAW, *ncurcol, 0);

	if(iconid == GV_ASCENDING || iconid == GV_DESCENDING)
	{
		if(iconid == GV_ASCENDING)
			VERIFY(icon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_SORTASC)));
		else
			VERIFY(icon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_SORTDSC)));

		VERIFY(DrawIconEx(hdcmem, recttemp->left, recttemp->top, icon, 16, 16, 2, NULL, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE));
	}

	if(iconid == GV_ASCENDING || iconid == GV_DESCENDING)
	{
		// increase the rectangle coordinate so that text comes a little on the right and we have space for
		// the icon
		recttemp->left+= SORTICONWIDTH;
	} 

#endif
		
}

wyInt32
CCustGrid::DrawTopColumns(HDC hdcmem, PRECT rect, PRECT rectwin, wyUInt32 *totcx)
{
	wyInt32     count = 0, ncol = 0, ncurcol = 0, runviscount = 0, totviscx = 0;
    RECT		recttemp = {0}, rectcolcount = {0};
	HGDIOBJ		hgdiobj;
	PGVCOLUMN	pgvcol = NULL;
	PGVCOLNODE	pgvnode = m_collist;

    ncol = (wyInt32)((m_col * m_hight) - (rectwin->bottom - 2* m_hight)) / m_hight;

    while(pgvnode != NULL)
	{
		if(count < m_initcol)
		{
			count++; 
	        ncurcol++;

            /// Skips rows
            pgvnode = SkipColumns(pgvnode, &rectcolcount, rectwin, totcx, &ncol);
			continue;
		}

		//Dont bring column(s) to be hidded to draw
		if(pgvnode->isshow == wyFalse)
		{
			ncurcol++;
			pgvnode = pgvnode->pNext;
			continue;
		}

		pgvcol = &pgvnode->pColumn;
		if(!pgvcol)
			break;
		
        /// Draws the column buttons
        DrawButtonForColumn(hdcmem, pgvcol, rect, totcx, &totviscx, rectwin, ncurcol);

		memcpy(&recttemp, rect, sizeof(RECT));

		recttemp.left += 3;
		recttemp.right -= 2;

        PaintColumnHeader(hdcmem, &recttemp, &ncurcol);

        /// Draws the different sorting icons for the columns
       	DrawColumnIcons(hdcmem, &ncurcol, &recttemp);
		
		if((m_curselcol == ncurcol) && (GetFocus()== m_hwnd))
			hgdiobj = SelectObject(hdcmem, (HGDIOBJ)m_htopfont);
		      
		else

			hgdiobj = SelectObject(hdcmem, (HGDIOBJ)m_hfont);
	
		VERIFY(SetBkMode(hdcmem, TRANSPARENT));

		/// We now text out the text.
		DrawColumnText(hdcmem, pgvcol, recttemp, &ncurcol);

		// Now we calculate some values for the horizontal scroll box.
		CalculateHorizontalScrollBar(pgvnode, rect, rectwin, totcx, &ncol, &totviscx, &runviscount);

		ncurcol++;
		SelectObject(hdcmem, hgdiobj);

        pgvnode = pgvnode->pNext;
	}

    if((m_exstyle & GV_EX_STRETCH_LAST_COL) && m_col > 0)
    {
        DrawButtonForColumn(hdcmem, NULL, rect, totcx, &totviscx, rectwin, ncurcol);
    }

    /// Determine the number of visible columns
    m_visiblecols = m_col - m_initcol - runviscount;

	return ncol;
}

void
CCustGrid::PaintColumnHeader(HDC hdcmem, RECT *recttemp, wyInt32 *ncurcol)
{
    TRIVERTEX	vertex[2];
	
	//Gradient variables
	vertex[0].x     = recttemp->left-3;
	vertex[0].y     = recttemp->top ;

	vertex[1].x     = recttemp->right+1;
	vertex[1].y     = recttemp->bottom-1; 

	vertex[0].Red   = 0x4C00;
	vertex[0].Green = 0xAA00;
	vertex[0].Blue  = 0xF900;
	vertex[0].Alpha = 0x0000;

	vertex[1].Red   = 0x4C00;
	vertex[1].Green = 0xAA00;
	vertex[1].Blue  = 0xF900;
	
    logtext("PaintColumnHeader");

    //Gradient for selected column header
	if(m_curselcol == *ncurcol)
		SetGradient(hdcmem, vertex);      
}

void
CCustGrid::PaintRowHeader(HDC hdcmem, RECT *recttemp, wyInt32 *ncurrow)
{    
    RECT        temprect;
	TRIVERTEX	vertex[2];

	//Gradient variables
	vertex[0].Red   = 0xffffff;
	vertex[0].Green = 0xffffff;
	vertex[0].Blue  = 0xffffff;
	vertex[0].Alpha = 0xffffff;

	vertex[1].Red   = 0xffffff;
	vertex[1].Green = 0xffffff;
	vertex[1].Blue  = 0xffffff;
	
    if(m_curselrow == *ncurrow)
	{
         ///  For the compensation of the 3d effect in the buttons
		if(recttemp->left > 2)
			temprect.left = recttemp->left + 1;
		else
			temprect.left = 2;

		temprect.top = (recttemp->top?recttemp->top:1);
		temprect.bottom = recttemp->bottom - 1;
		temprect.right= recttemp->right - 2;

		vertex[0].x     = recttemp->left+1 ;
		vertex[0].y     = recttemp->top ;

		vertex[1].x     = recttemp->right-1 ;
		vertex[1].y     = recttemp->bottom-1 ; 
		
		//Gradienty for selected row-header 
		SetGradient(hdcmem, vertex);        
    }
}

void
CCustGrid::DrawColumnText(HDC hdcmem, PGVCOLUMN	pgvcol, RECT recttemp, wyInt32 *ncurcol)
{
    wyUInt32    format = 0;
    wyString    text;
	RECT        rect = {0}, temp = {0};
    HBRUSH      hbrush;
    wyWChar*    str = NULL;
    HICON       hicon = NULL;

    if(pgvcol->mask & GVIF_COLUMNMARK)
    {
        if(pgvcol->marktype == GV_MARKTYPE_TEXT)
        {
            str = (wyWChar*)pgvcol->mark;
        }
        else if(pgvcol->marktype == GV_MARKTYPE_ICON)
        {
            hicon = (HICON)pgvcol->mark;
        }

        if(str || hicon)
        {
            rect.left = temp.left = recttemp.left;
            rect.right = temp.right = recttemp.right;

            if(str)
            {
                rect.top = temp.top = recttemp.top + 3;
                rect.bottom = temp.bottom = recttemp.bottom - 3;
                DrawText(hdcmem, str, -1, &temp, DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS | DT_CALCRECT);
                rect.left = rect.right - (temp.right - temp.left) - 3;
            }
            else
            {
                rect.top = temp.top = recttemp.top + 2;
                rect.bottom = temp.bottom = recttemp.bottom - 2;
                rect.left = rect.right - (rect.bottom - rect.top + 3);
            }

            if(rect.left < recttemp.left)
            {
                rect.left = recttemp.left;
            }

            recttemp.right = rect.left - 2;
        }
    }

    format = DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS;

	if(pgvcol  && pgvcol->text)
	{
		if(m_curselcol == *ncurcol)
		{
			SetTextColor(hdcmem,RGB(255,255,255));
		}
		text.SetAs(pgvcol->text);
	DrawText(hdcmem, text.GetAsWideChar(), lstrlen(text.GetAsWideChar()), &recttemp, format);
	if(m_curselcol == *ncurcol)
		{
			SetTextColor(hdcmem,RGB(0,0,0));
		}
	}

    if(pgvcol->mask & GVIF_COLUMNMARK)
    {
        if(str)
        {
            hbrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdcmem, &rect, hbrush);
            DrawText(hdcmem, str, -1, &rect, format);
            DeleteObject(hbrush);
        }
        else if(hicon)
        {
            DrawIconEx(hdcmem, rect.left, rect.top, hicon, rect.right - rect.left, rect.bottom - rect.top, 0, NULL, DI_NORMAL);
        }
    }
}

void
CCustGrid::CalculateHorizontalScrollBar(PGVCOLNODE	pgvnode, PRECT rect, PRECT rectwin, wyUInt32 *totcx,
                                        wyInt32 *ncol, wyInt32 *totviscx, wyInt32 *runviscount)
	{
    if(*totcx > (wyUInt32)rectwin->right)
        *ncol++;

    if(*totviscx > ((wyUInt32)rectwin->right - GV_DEFWIDTH))
        (*runviscount)++;

	pgvnode = pgvnode->pNext;
	
    if(m_flip == wyTrue)
        rect->top = rect->bottom;
    else
        rect->left = rect->right;
}

void
CCustGrid::DrawRowButtons(HDC hdcmem, RECT *rect, RECT *recttemp, wyInt32 *rowcount)
{
    wyInt32             state;
    GVROWCHECKINFO		info = {0};
    HFONT	            fontold;
    wyWChar		        star[] = L"*";
	TRIVERTEX			vertex[2];
	
	//Gradient variables
	vertex[0].x     = rect->left+1;
	vertex[0].y     = rect->top;

	vertex[1].x     = rect->right-1;
	vertex[1].y     = rect->bottom-1; 

	vertex[0].Red   = 0xffffff;
	vertex[0].Green = 0xffffff;
	vertex[0].Blue  = 0xffffff;
	vertex[0].Alpha = 0xffffff;

	vertex[1].Red   = 0xffffff;
	vertex[1].Green = 0xffffff;
	vertex[1].Blue  = 0xffffff;
	vertex[1].Alpha = 0xffffff;
   
	// now we see whether we need a checkbox somewhere.
	if(m_exstyle & GV_EX_ROWCHECKBOX)
    {
		VERIFY(DrawFrameControl(hdcmem, rect, DFC_BUTTON, DFCS_BUTTONPUSH|DFCS_FLAT));

		PaintRowHeader(hdcmem, rect, rowcount);

		state = DFCS_BUTTONCHECK |  DFCS_FLAT ;

	    m_lpgvwndproc(m_hwnd, GVN_DRAWROWCHECK, *rowcount, (LPARAM)&info);

		if(GetOwnerData())
        {
			if(info.ischecked)
				state |= DFCS_CHECKED;
		} 
        else 
        {
			// we call back on user to ask whether we can draw * or we required
			// a check box
			if(info.ischecked)
				state |= DFCS_CHECKED;
		}

	    memcpy(recttemp, rect, sizeof(RECT));
		
	    recttemp->top	 += 1;
	    recttemp->bottom  -= 2;

		//Gradient for unselected items(selected items fills with color)
		if(m_curselrow != *rowcount)
			SetGradient(hdcmem, vertex);
		
		if(!info.checkorstar)
		{
			recttemp->top+=2;
			recttemp->bottom--;
		   						
			VERIFY(DrawFrameControl(hdcmem, recttemp, DFC_BUTTON, state));
		}
		else
			VERIFY(DrawText(hdcmem, L"*", 1, recttemp, DT_VCENTER | DT_SINGLELINE | DT_CENTER));
	} 
    else
    {
		state = DFCS_BUTTONPUSH | DFCS_FLAT;
			
		VERIFY(DrawFrameControl(hdcmem, rect, DFC_BUTTON, state));

		//Gradient if not check box
		SetGradient(hdcmem, vertex);
		
		PaintRowHeader(hdcmem, rect, rowcount);

        if((m_curselrow == *rowcount))	
        {
			fontold =(HFONT)SelectObject(hdcmem,(HGDIOBJ)m_htopfont);
		    /*if(m_flip == wyTrue)
            {
                VERIFY(hbrush = CreateSolidBrush(color));            
                VERIFY(FillRect(hdcmem, rect, hbrush));
            }
            else
                VERIFY(DrawText(hdcmem, star, wcslen(star), rect, DT_VCENTER | DT_SINGLELINE | DT_CENTER));*/

			if(m_flip != wyTrue)
                VERIFY(DrawText(hdcmem, star, wcslen(star), rect, DT_VCENTER | DT_SINGLELINE | DT_CENTER));

			fontold =(HFONT)SelectObject(hdcmem, (HGDIOBJ)fontold);			
		}        
    }	
}

wyInt32
CCustGrid::GetRemainingRows(PRECT rectwin)
{
    /*PGVROWNODE  pgvnode = m_rowlist;
    wyInt32     numberofrows = 0, count = 0;
    wyInt32     cx = m_maxwidth;

    while(pgvnode && numberofrows < m_initrow)
    {
        pgvnode = pgvnode->pNext;
        numberofrows++;
    }

    while(pgvnode && cx < rectwin->right)
    {
        cx += pgvnode->rowcx;
        pgvnode = pgvnode->pNext;
        numberofrows++;
    }

    while(pgvnode)
    {
        count++;
        pgvnode = pgvnode->pNext;
    }*/

    return m_row - 1;
}

wyInt32
CCustGrid::DrawRowColumns(HDC hdcmem, PRECT rect, PRECT rectwin, wyUInt32 *totcx, wyUInt32 *totcy)
{
	wyInt32             rowcount = 0, colcount = 0, nrow;	
    RECT		        rect2 = {0}, recttemp = {0}, greyrect = {0};
	HPEN		        hpen, hpenold;
	HBRUSH		        hbrbkgnd;
	PGVCOLUMN	        pgvcol = NULL;
	PGVROWNODE	        pgvrownode = m_rowlist;
    PGVROWNODE	        prownode = m_rowlist, temprowlist;
	PGVCOLNODE	        pgvnode = NULL, pgvcolnode = NULL;
    wyInt32             rcount = 0;
    wyBool              isbreak = wyFalse;
    GVWATTERMARK        gvwattermark = {0};
		
	VERIFY(hpen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_ACTIVEBORDER)));
	hpenold = (HPEN)SelectObject(hdcmem, hpen);	// no ret
	VERIFY(hbrbkgnd = CreateSolidBrush(RGB(DIS_BKG, DIS_BKG, DIS_BKG)));

	//Draw all rows if Scroll bar is not present
	if(m_flip == wyFalse && ((m_row + 2) * m_hight < rectwin->bottom))
        m_initrow = 0;

    /// We calculate how many extra rows will be there in the grid.
	if(m_flip == wyTrue)
        nrow = GetRemainingRows(rectwin);
    else
        nrow = (wyInt32)((m_row * m_hight)-(rectwin->bottom-(2*m_hight)))/m_hight;
	
	*totcy  += (m_initrow * m_hight);
	
	/// If its not owner data then we have to reach the correct first row list
	if(!GetOwnerData())
	{
		for(rowcount = 0; pgvrownode && rowcount < m_initrow; rowcount++)
        {
            prownode = prownode->pNext;
			pgvrownode = pgvrownode->pNext;
        }
	}

    if(m_form == wyTrue)
    {
        rcount = m_row;
        m_row = m_initrow + 1;
    }
	
    for(rowcount = m_initrow; rowcount < m_row && isbreak == wyFalse; rowcount++)
	{
		if(!GetOwnerData()&& !pgvrownode)
			break;

        if(GetOwnerData() && !m_lpgvwndproc(m_hwnd, GVN_BEGINROWDRAW, rowcount, 0))
        {
            continue;
        }

		//If H-Scroll is not present
		if(rect->bottom > rectwin->bottom && *totcx <= rectwin->right)
		{
			*totcy += m_hight + 5;

            //break next time around
			isbreak = wyTrue;
		}

		//If H-Scroll presents draw one more row to avoid painting issue. Post 8.04 Bea2
		else if(rect->bottom > rectwin->bottom && (rectwin->bottom + 2 * m_hight) < (rect->bottom) && *totcx > rectwin->right)
		{
			*totcy += m_hight + 5;

            //break next time around
			isbreak = wyTrue;
		}

		/// Now we see whether we need a checkbox somewhere.
        DrawRowButtons(hdcmem, rect, &recttemp, &rowcount);
	
		if(!GetOwnerData())
			pgvnode = pgvrownode->pColumn;
		else
			pgvnode = NULL;

        if(m_flip == wyTrue)
        {
            *totcx += prownode->rowcx;
            rect2.left = rect->left;
		    rect2.top = m_hight;
        }
        else
        {
		    rect2.left = rect->left+GV_DEFWIDTH;
		    rect2.top = rect->top;
        }

		/// now reach the initcol if its not owner data.
		if(!GetOwnerData())
			for(colcount = 0; colcount < m_initcol && (pgvnode = pgvnode->pNext); colcount++);
				
		for(colcount = m_initcol; colcount < m_col; colcount++)
		{
			if(!GetOwnerData())
            {
				if(!pgvnode)
					break;
			}

			VERIFY(pgvcolnode = GetColNodeStruct(colcount));
			
			if(!GetOwnerData() && (pgvcolnode->isshow == wyTrue))
				VERIFY(pgvcol = &pgvnode->pColumn);
			
            if(m_flip == wyTrue)
            {
                rect2.right = rect2.left + prownode->rowcx;
			    rect2.bottom = rect2.top + m_hight;
            }
            else
            {
				//If single column visible then its max width limited to 'right of grid - 25'
				if(pgvcolnode->isshow == wyTrue && rectwin->right > 75 && pgvcolnode->pColumn.cx >= (rectwin->right - 35) && rect2.left == 25)
					pgvcolnode->pColumn.cx = rectwin->right - 35;
				
			    if(pgvcolnode && pgvcolnode->isshow == wyTrue)
				{
					rect2.right = rect2.left+pgvcolnode->pColumn.cx;
					rect2.bottom = rect2.top + m_hight;
				}
            }

            /// Draw grey rectangle
			if(pgvcolnode && pgvcolnode->isshow == wyTrue)
				DrawGreyRect(hdcmem, &greyrect, &rect2, &recttemp);
						
			// Draw the cell text
			if(pgvcolnode && pgvcolnode->isshow == wyTrue)
			{
				DrawCell(hdcmem, rowcount, colcount, pgvcolnode, pgvcol, hbrbkgnd, &rect2, &greyrect, &recttemp);
			}

			if(m_flip == wyTrue)
            {
                rect2.top = rect2.bottom;
                rect2.left = rect->left;
            }
            else if(pgvcolnode && pgvcolnode->isshow == wyTrue)
				rect2.left = rect2.right;

			if(!GetOwnerData())
				pgvnode = pgvnode->pNext;
			
			//Break the loop if the column crossed the width of grid window
			/*if(rect2.right > rectwin->right + 50)
				break;*/
		}
				
        if(m_flip == wyTrue)
        {
            rect->left = rect->left + prownode->rowcx;
            temprowlist = prownode->pNext;
            if(temprowlist)
	    	    rect->right = rect->right + temprowlist->rowcx;
        }
        else if(pgvcolnode && pgvcolnode->isshow == wyTrue)
        {
			rect->top = rect->top + m_hight;
			rect->bottom = rect->top + m_hight;
        }

		*totcy += m_hight;

		// if not owner data then we need to move to the new row list array
		if(!GetOwnerData())
			pgvrownode = pgvrownode->pNext;

        if(prownode)
            prownode = prownode->pNext;

	}	

    if(!m_row)
    {
        gvwattermark.hdc = hdcmem;
        gvwattermark.rect = *rectwin;
        gvwattermark.rect.top = *totcy;
        m_lpgvwndproc(m_hwnd, GVN_DRAWWATERMARK, 0, (LPARAM)&gvwattermark);
    }
	
    if(m_form == wyTrue)
        m_row = rcount ;

	SelectObject(hdcmem,(HGDIOBJ)hpenold);
	VERIFY(DeleteObject(hpen));
	VERIFY(DeleteObject(hbrbkgnd));

	return nrow;
}


/// Makes the background of the cell grey as in dataview query Read-only mode
void
CCustGrid::DrawGreyRect(HDC hdcmem, RECT *greyrect, RECT *rect2, RECT *recttemp)
{
	
    memcpy(greyrect, rect2, sizeof(RECT));

	greyrect->left++; 
    greyrect->right--; 
    greyrect->bottom--; 
    greyrect->top++;

	VERIFY(MoveToEx(hdcmem, rect2->right - 1, rect2->top, NULL));
	VERIFY(LineTo(hdcmem, rect2->right - 1, rect2->bottom));

	VERIFY(MoveToEx(hdcmem, rect2->left, rect2->bottom - 1, NULL));
	VERIFY(LineTo(hdcmem, rect2->right, rect2->bottom - 1));

	memcpy(recttemp, rect2, sizeof(RECT));

	recttemp->left += 1;
	recttemp->right -= 3;
	}


void
CCustGrid::DrawCellBool(HDC hdcmem, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col)
{
wyUInt32    state = 0;

	rowrect->bottom -= 2;
	rowrect->top += 1;

	// now we check if it is false or not.
	if(!GetOwnerData())
    {
		if(ColTrueOrFalse(row, col))
			state = DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_FLAT;
		else
			state = DFCS_BUTTONCHECK | DFCS_FLAT;

        VERIFY(DrawFrameControl(hdcmem, rowrect, DFC_BUTTON, state));
	} 
    else 
    {
	m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO, (WPARAM)disp, 0);

	if(!stricmp(disp->text, "true"))
			state = DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_FLAT;
        else 
			state = DFCS_BUTTONCHECK | DFCS_FLAT;

		VERIFY(DrawFrameControl(hdcmem, rowrect, DFC_BUTTON, state));
	}
}


void
CCustGrid::DrawCellButton(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col)
{
wyUInt32    len;
wyChar      temp[512] = {0};
PGVCOLUMN	pgvcolbutton;
wyString    tempstr, disptextstr;

	if(!GetOwnerData())
		VERIFY(pgvcolbutton = GetSubItemStruct(row, col));
	else
		pgvcolbutton = NULL;
	
	if(!GetOwnerData()&& pgvcolbutton->uIsButtonVis)
	{
		rowrect->top	+= 1;
		rowrect->bottom -= 2;
		len = _snprintf(temp, 511, "%s", pgvcol->text); 
		
	  	tempstr.SetAs(temp);

		// now we draw a button.
		//VERIFY(DrawFrameControl(hdcmem, rowrect, DFC_BUTTON, DFCS_BUTTONPUSH));
		VERIFY(DrawFrameControl(hdcmem, rowrect, DFC_BUTTON, DFCS_BUTTONPUSH|DFCS_FLAT));
		DrawText(hdcmem, tempstr.GetAsWideChar(), len, rowrect, DT_VCENTER | DT_SINGLELINE | DT_CENTER);
	}	
    else if(pgvcolbutton->uIsButtonVis)
    {
		rowrect->top	+= 1;
		rowrect->bottom -= 2;
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO, (WPARAM)disp, 0);
		
        disptextstr.SetAs(disp->text);

		DrawButtonWithText(hdcmem, rowrect, disp->text, "");
		DrawText(hdcmem, disptextstr.GetAsWideChar(), wcslen(disptextstr.GetAsWideChar()), rowrect, DT_VCENTER | DT_SINGLELINE | DT_CENTER| DT_END_ELLIPSIS);
		rowrect->left--;
	}
}

void
CCustGrid::DrawCellTextButton(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col)
{
PGVCOLUMN	pgvcolbutton;
wyUInt32    len;
wyChar      temp[512] = {0};

	if(!GetOwnerData())
		VERIFY(pgvcolbutton = GetSubItemStruct(row, col));
	else
		pgvcolbutton = NULL;
	
	if(!GetOwnerData()&& pgvcolbutton->uIsButtonVis)
	{
		len = _snprintf(temp, 511, "%s", pgvcol->text); 
		// now we draw the text with the button with ....
		DrawButtonWithText(hdcmem, rowrect, temp, pgvcol->pszButtonText);
	}	
    else 
    {
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO,(WPARAM)disp, 0);
		DrawButtonWithText(hdcmem, rowrect, disp->text, disp->pszButtonText);
	}
} 

void
CCustGrid::DrawCellDropOrList(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, wyInt32 row, wyInt32 col)
{
    wyUInt32    len;
    wyChar      temp[512] = {0};
    wyString    disptextstr, dispbuttontextstr;
	
	if(!GetOwnerData())
	{
		len = _snprintf(temp, 511, "%s", pgvcol->text); 
		// now we draw the text with the button with ....

        if(pgvcol->pszButtonText && pgvcol->pszButtonText[0] == '0' && !stricmp(pgvcol->text, STRING_NULL))
        {
            SelectObject(hdcmem, (HGDIOBJ) m_hItalicsFont);
            DrawButtonWithText(hdcmem, rowrect, temp, pgvcol->pszButtonText, wyTrue);
            SelectObject(hdcmem, (HGDIOBJ) m_hfont);
        }
        else
        {
		    DrawButtonWithText(hdcmem, rowrect, temp, pgvcol->pszButtonText, wyTrue);
        }
	}	
    else 
    {
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO,(WPARAM)disp, 0);
		disptextstr.SetAs(disp->text);
		dispbuttontextstr.SetAs(disp->pszButtonText);
        if(disp->pszButtonText && disp->pszButtonText[0] == '0' && !wcsicmp(disptextstr.GetAsWideChar(), TEXT(STRING_NULL)))
        {
            SelectObject(hdcmem, (HGDIOBJ)m_hItalicsFont);
            DrawButtonWithText(hdcmem, rowrect, disp->text, disp->pszButtonText, wyTrue);
            SelectObject(hdcmem, (HGDIOBJ)m_hfont);
        }
        else
        {
		    DrawButtonWithText(hdcmem, rowrect, disp->text, disp->pszButtonText, wyTrue);
        }
	}
} 


void
CCustGrid::DrawCellOther(HDC hdcmem, PGVCOLUMN pgvcol, RECT *rowrect, GVDISPINFO *disp, PGVCOLNODE topcolstruct)
{
    wyUInt32    format = 0;
    wyString    text;

    if(pgvcol && pgvcol->text)
        text.SetAs(pgvcol->text);

    if(topcolstruct->pColumn.fmt & GVIF_RIGHT)
		format |= DT_RIGHT;
    else 
		format |= DT_LEFT;
	
	format |= DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;	

	if(!GetOwnerData())
    {
		if(!pgvcol && !pgvcol->text)
        {
            SelectObject(hdcmem, (HGDIOBJ)m_hItalicsFont);
			DrawText(hdcmem, L"(NULL)", wcslen(TEXT("(NULL)")), rowrect, format);
            SelectObject(hdcmem, (HGDIOBJ)m_hfont);
        }
        else
        {
            if(pgvcol->pszButtonText && pgvcol->pszButtonText[0] == '0' && !wcsicmp(text.GetAsWideChar(), TEXT(STRING_NULL)))
            {
                SelectObject(hdcmem, (HGDIOBJ)m_hItalicsFont);
                DrawText(hdcmem, text.GetAsWideChar(), wcslen(text.GetAsWideChar()), rowrect, format);
                SelectObject(hdcmem, (HGDIOBJ)m_hfont);
            }
            else
            {
                DrawText(hdcmem, text.GetAsWideChar(), wcslen(text.GetAsWideChar()), rowrect, format);
            }
        }
    }
	else
	{
		//LPARAM sets for handling the base2 display format in grid
        m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO, (WPARAM)disp, 1);
        
        if(disp && disp->text)
            text.SetAs(disp->text);

        if(disp->pszButtonText && disp->pszButtonText[0] == '0' && !wcsicmp(text.GetAsWideChar(), TEXT(STRING_NULL)))
        {
            SelectObject(hdcmem, (HGDIOBJ)m_hItalicsFont);
            DrawText(hdcmem, text.GetAsWideChar(), wcslen(text.GetAsWideChar()), rowrect, format);
            SelectObject(hdcmem, (HGDIOBJ)m_hfont);
        }
        else
        {
            DrawText(hdcmem, text.GetAsWideChar(), wcslen(text.GetAsWideChar()), rowrect, format);
        }
	}
} 

// helper function to draw individual cell */
wyBool
CCustGrid::DrawCell(HDC hdcmem, wyInt32 row, wyInt32 col, PGVCOLNODE topcolstruct, PGVCOLUMN pgvcol,
                    HBRUSH hbrbkgnd, RECT * greyrect, RECT * rect, RECT * rowrect)
{
	wyUInt32    format = DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS | DT_NOPREFIX;
	GVDISPINFO	disp = {0};
    HPEN		hpenthick = NULL, hpenold = NULL;
	HBRUSH		hbrgrey = NULL, hbrold = NULL;
    wyInt32		isnotgrey  = 0, count = 0;
    GVCELLCOLORINFO colorinfo = {0};
    RECT        rectwin;
    
    wyChar		usrdata[512] = {0};
    wyChar		buttondata[512] = {0};

	RECT		r;
	COLORREF    color;

	if(topcolstruct->pColumn.fmt & GVIF_RIGHT)	
		format |= DT_RIGHT;
    else 
		format |= DT_LEFT;

	if(topcolstruct->pColumn.mask & GVIF_BUTTON || topcolstruct->pColumn.mask & GVIF_TEXTBUTTON)
		format |= DT_CENTER;
   
     /// Colors and highlights cell
    disp.pszButtonText = buttondata; 
    disp.text = usrdata; 
	disp.nRow = row; 
    disp.nCol = col; 
    disp.cchTextMax = 512;
    colorinfo.nRow = row;
    colorinfo.nCol = col;

    if((topcolstruct->pColumn.mask & GVIF_CELLCOLOR) &&
        m_lpgvwndproc(m_hwnd, GVN_GETCELLCOLOR, 0, (LPARAM)&colorinfo))
    {
        while(1)
        {
            hbrgrey = CreateSolidBrush(colorinfo.color);
            count++;
            r.left = rect->left - (m_exstyle & GV_EX_NO_VER_BORDER ? 1 : 0);
		    r.top = rect->top - (m_exstyle & GV_EX_NO_HOR_BORDER ? 1 : 0);
		    r.right = rect->right + (m_exstyle & GV_EX_NO_VER_BORDER ? 1 : 0);
		    r.bottom = rect->bottom + (m_exstyle & GV_EX_NO_HOR_BORDER ? 1 : 0);
            FillRect(hdcmem, &r, hbrgrey);

            if(m_exstyle & GV_EX_FULLROWSELECT)
            {
                if(row == m_curselrow)
                {
                    hpenthick = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                    hpenthick = (HPEN)SelectObject(hdcmem, hpenthick);
                    MoveToEx(hdcmem, rect->left - 2, rect->top - 1, NULL);
                    LineTo(hdcmem, rect->right, rect->top - 1);
                    MoveToEx(hdcmem, rect->left - 2, rect->bottom, NULL);
                    LineTo(hdcmem, rect->right, rect->bottom);
                    hpenthick = (HPEN)SelectObject(hdcmem, hpenthick);
                    DeleteObject(hpenthick);
                }

            }
            else
            {
                if(row == m_curselrow && col == m_curselcol)
                {
                    hpenthick = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
                    hpenthick = (HPEN)SelectObject(hdcmem, hpenthick);
                    MoveToEx(hdcmem, rect->left - 1, rect->top - 1, NULL);
                    LineTo(hdcmem, rect->right, rect->top - 1);
                    MoveToEx(hdcmem, rect->right, rect->top - 1, NULL);
                    LineTo(hdcmem, rect->right, rect->bottom);
                    MoveToEx(hdcmem, rect->right, rect->bottom, NULL);
                    LineTo(hdcmem, rect->left - 2, rect->bottom - 1);
                    MoveToEx(hdcmem, rect->left - 1, rect->bottom - 1, NULL);
                    LineTo(hdcmem, rect->left - 1, rect->top - 2);
                    hpenthick = (HPEN)SelectObject(hdcmem, hpenthick);
                    DeleteObject(hpenthick);
                }
            }

            DeleteObject(hbrgrey);

            if(count == 1 && col == m_col - 1 && (m_exstyle & GV_EX_STRETCH_LAST_COL))
            {
                GetClientRect(m_hwnd, &rectwin);

                if(rect->right < rectwin.right)
                {
                    colorinfo.nCol++;
                    m_lpgvwndproc(m_hwnd, GVN_GETCELLCOLOR, 0, (LPARAM)&colorinfo);
                    rect->left = rect->right;
                    rect->right = rectwin.right;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    {
	    isnotgrey = m_lpgvwndproc(m_hwnd, GVN_ISWHITEBKGND, row, col);

        // now if the row is the current selected row then we draw a thin border thru the row

        if((row == m_curselrow &&  col == m_curselcol) && topcolstruct && topcolstruct->isshow == wyTrue)
        {
		    VERIFY(hbrgrey = CreateSolidBrush(GV_SELCOLOR));
		    VERIFY(hpenthick = CreatePen(PS_SOLID, 2, RGB(59,125,187)));
		    hbrold	= (HBRUSH)SelectObject(hdcmem, hbrgrey); // no ret
		    hpenold = (HPEN)SelectObject(hdcmem, hpenthick); // no ret

		    VERIFY(Rectangle(hdcmem, rect->left-1, rect->top-1, rect->right+1, rect->bottom+1));

            SelectObject(hdcmem, hbrold); // no ret
		    SelectObject(hdcmem, hpenold); // no ret

            VERIFY(DeleteObject(hbrgrey));
            VERIFY(DeleteObject(hpenthick));
	    }

	    if(row == m_curselrow && col != m_curselcol)
        {
		    color = ROWHIGHLIGHTCOLOR;
		    r.left = rect->left-1;
		    r.top = rect->top-1;
		    r.right = rect->right;
		    r.bottom = rect->bottom;

            VERIFY(hbrgrey = CreateSolidBrush(color));            
            VERIFY(FillRect(hdcmem, &r, hbrgrey));
            VERIFY(DeleteObject((HGDIOBJ)hbrgrey));   
	    }
	    else if(row != m_curselrow)
	    {
		    r.left = rect->left-1;
		    r.top = rect->top-1;
		    r.right = rect->right;
		    r.bottom = rect->bottom;
		    color = ALTHIGHLIGHTCOLOR;

		    if(row%2)
		    {
			    VERIFY(hbrgrey = CreateSolidBrush(color));            
			    VERIFY(FillRect(hdcmem, &r, hbrgrey));
			    VERIFY(DeleteObject((HGDIOBJ)hbrgrey));
		    }
		    else if(!isnotgrey)
		    {
			    VERIFY(FillRect(hdcmem, &r, hbrbkgnd));
		    }
	    }
    }

	if(topcolstruct->pColumn.mask & GVIF_BOOL)
	    DrawCellBool(hdcmem, rowrect, &disp, row, col);

	else if(topcolstruct->pColumn.mask & GVIF_BUTTON)
	    DrawCellButton(hdcmem, pgvcol, rowrect, &disp, row, col);
		
	else if(topcolstruct->pColumn.mask & GVIF_TEXTBUTTON)
        DrawCellTextButton(hdcmem, pgvcol, rowrect, &disp, row, col);

    else if(topcolstruct->pColumn.mask & GVIF_DROPDOWNLIST || topcolstruct->pColumn.mask & GVIF_DROPDOWNNLIST ||
            topcolstruct->pColumn.mask & GVIF_DROPDOWNMULTIPLE || topcolstruct->pColumn.mask & GVIF_LIST)	
        DrawCellDropOrList(hdcmem, pgvcol, rowrect, &disp, row, col);

    else 
        DrawCellOther(hdcmem, pgvcol, rowrect, &disp, topcolstruct);

	return wyTrue;
}	

/* Function takes the cell boundary in RECT parameter and draws like:

	aaaaaaaa [...] // [...] signifies a button */

wyBool
CCustGrid::DrawButtonWithText(HDC hdc, RECT * rect, const wyChar * text, const wyChar * buttontext, wyBool iscombodrop)
{
	wyInt32				buttonlength;
	wyInt32				extra = 3;
	wyInt32				oldleft=0;
	wyString			textstr(text), buttontextstr;
	SIZE				size;
	wyString			buttonstr("999K");
	
	rect->top		+= 1;
	rect->bottom	-= 2;

	//Width of the button
	GetTextExtentPoint32(hdc, buttonstr.GetAsWideChar(), buttonstr.GetLength()+ 2, &size);

	/* depending upon whether we want to draw a combo or button we keep the length of the button */
	if(iscombodrop == wyFalse)
	{
		buttonlength = size.cx + 10;

		//if column width is less than button length 
		if(rect->right - rect->left < buttonlength)
		{
			buttonlength = rect->right - rect->left;
		}		
	}
	else
		buttonlength = CB_BTN_LENGTH;
	
	rect->right -= (buttonlength - extra);
	
    if(buttontext && buttontext[0] == '0' && !wcsicmp(textstr.GetAsWideChar(), TEXT(STRING_NULL)))
    {
        SelectObject(hdc, (HGDIOBJ) m_hItalicsFont);
        DrawText(hdc, textstr.GetAsWideChar(), wcslen(textstr.GetAsWideChar()), rect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(hdc, (HGDIOBJ) m_hfont);
    }
    else
    {
	    DrawText(hdc, textstr.GetAsWideChar(), wcslen(textstr.GetAsWideChar()), rect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

	/* revert back to original */
	rect->right += (buttonlength+extra);

	/* to draw the text we have to first increase the left..draw the button and get it back to original */
	oldleft = rect->left;

	rect->left = (rect->right - buttonlength);

	/* draw the button and ... */
	rect->right -= 5;
	
	if(iscombodrop == wyFalse)
		VERIFY(DrawFrameControl(hdc, rect, DFC_BUTTON, DFCS_BUTTONPUSH|DFCS_FLAT));
	else
		VERIFY(DrawFrameControl(hdc, rect, DFC_SCROLL, DFCS_SCROLLDOWN|DFCS_FLAT));
	
	rect->left += 2;

	if(buttontext)
		buttontextstr.SetAs(buttontext);
	else
		buttontextstr.SetAs("");
	
	rect->right -= 10;
	if(iscombodrop ==wyFalse)
		DrawText(hdc, buttontextstr.GetAsWideChar(), wcslen(buttontextstr.GetAsWideChar()), rect, 
		DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_RIGHT );

	/* get back the left & right coord to original */
	rect->right += 15;
	rect->left = oldleft;

	return wyTrue;
}

LRESULT
CCustGrid::OnVScroll(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	SCROLLINFO	sciv;
    SCROLLINFO	scih;
	DWORD		hiword = 0;
	DWORD		pos, remstemps = 0, vsrem = 0;

	ZeroMemory(&sciv, sizeof(SCROLLINFO));
    ZeroMemory(&scih, sizeof(SCROLLINFO));

	sciv.cbSize = sizeof(SCROLLINFO);
	sciv.fMask = SIF_ALL;

	scih.cbSize = sizeof(SCROLLINFO);
	scih.fMask = SIF_ALL;

	VERIFY(GetScrollInfo(m_hwnd, SB_VERT, &sciv));
    VERIFY(GetScrollInfo(hwnd, SB_HORZ, &scih));

	pos = sciv.nPos / GV_SCROLLPOS;
		
	if(m_flip == wyFalse && m_initrow != pos)
		m_initrow = pos;

	if(m_iscomboediting == wyTrue || m_isediting == wyTrue)
		ApplyChanges();

	//if(!m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol))
		//return 1;

	m_isscrolling = wyTrue;
	m_prescrollinitrow = m_initrow;

	
	switch(LOWORD(wparam))
	{
	case SB_PAGEDOWN:
        if(m_flip == wyTrue)
        {
            if(m_initrow > scih.nMax - 1)
			    return 1;

			if((scih.nPos + scih.nPage) + scih.nPage > scih.nMax)
			{
				vsrem = m_row - m_initrow - m_vskiprows;
				
				if(vsrem <= m_initrow)
					m_initrow += (m_row - m_initrow - m_vskiprows) + 1;
				else
					m_initrow = vsrem;        				
			}
			else
				m_initrow += m_visiblecols;			
        }
        else
		{
		    CalculatePageDown();

			//limits to max scroll pos, else it will flicker
			remstemps = (m_vscrollmaxrange - m_vscrollthumwidth) / GV_SCROLLPOS;
			if(m_initrow > remstemps)
				m_initrow = remstemps;			
		}
				
		break;

	case SB_LINEDOWN:
        if(m_flip == wyTrue)
        {
            if(m_initrow > scih.nMax - 1)
			    return 1;

			if(scih.nPos >= (m_vscrollmaxrange - scih.nPage))
				return 1;

            m_initrow++;			
        }
        else
        {
            if(((m_initrow * GV_SCROLLPOS) + m_vscrollthumwidth) >= sciv.nMax)
			    return 1;
		    
            m_initrow++;			
        }

		break;

	case SB_PAGEUP:
        if(m_flip == wyTrue)
        {
            if(m_initrow <= 0)
			    return 1;

		    m_initrow-=m_visiblecols;

            if (m_initrow <= 0) 
                m_initrow = 0;
        }
        
		else
			CalculatePageUp();
			
		break;

	case SB_LINEUP:
		if(m_initrow <= 0)
			return 1;

		m_initrow--;
		
		break;

	case SB_THUMBTRACK:
		{				
			if(m_flip == wyFalse)
				VERIFY(GetScrollInfo(m_hwnd, SB_VERT, &sciv));
					
			else
				VERIFY(GetScrollInfo(m_hwnd, SB_HORZ, &sciv));
				
			hiword = sciv.nTrackPos;
			
			if(hiword == 0)
				m_initrow = 0;
			else if(m_flip == wyFalse)
				m_initrow = hiword / GV_SCROLLPOS;

			else if(m_maxwidth)//if(m_flip == wyTRue)
				m_initrow = hiword / m_maxwidth;		
		}

		break;
	}

	VERIFY(InvalidateRect(hwnd, NULL, TRUE));
	UpdateWindow(hwnd);
	m_isscrolling = wyFalse;

	return 0;
}

LRESULT
CCustGrid::OnHScroll(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    SCROLLINFO	scih;
	WPARAM		hiword;		
	wyInt32		pos, remstemps = 0;	

    ZeroMemory(&scih, sizeof(SCROLLINFO));

	scih.cbSize = sizeof(SCROLLINFO);
	scih.fMask = SIF_ALL;

	VERIFY(GetScrollInfo(hwnd, SB_HORZ, &scih));

	pos = scih.nPos / m_hight;
	
	if(m_iscomboediting == wyTrue || m_isediting == wyTrue)
		ApplyChanges();

	switch(LOWORD(wparam))
	{
	case SB_PAGERIGHT:
        {
            if(m_initcol >= scih.nMax)
                return 1;

            if(m_flip == wyTrue)
                m_initcol  += RowPerPage();
            else
				m_initcol += (m_visiblecols ? m_visiblecols : 1);

            m_initcol = min(m_initcol, m_col - 1);
			
			//limits to max scroll pos, else it will flicker
			if(m_hscrollnumpos)
				remstemps = (m_hscrollmaxrange - m_hscrollthumwidth) / m_hscrollnumpos;
			if(m_initcol > remstemps)
				m_initcol = remstemps;			
        }
        break;

	case SB_LINERIGHT:
	    if(m_initcol < scih.nMin)
			return 1;

		//checking the limit
		if(m_flip == wyFalse && m_initcol >= m_hskipcolcount)
			return 1;

		if(m_flip == wyTrue)
		{
			if(m_initcol >= m_col - 1)
				return 1;

			m_initcol++;					
		}
        else
		{
			if(m_initcol >= m_hskipcolcount)
				return 1;

			m_initcol++;

            m_initcol = min(m_initcol, m_col - 1);			
		}

		break;

	case SB_PAGELEFT:

        if(m_flip == wyTrue)
            m_initcol  -= RowPerPage();
        else
            m_initcol -= ColumnVisibleCount();

        if (m_initcol <= 0) 
            m_initcol = 0;
        		        
        break;

	case SB_LINELEFT:
		if(m_initcol <= 0)
			return 1;

		m_initcol--;

        if (m_initcol <= 0) 
            m_initcol = 0;
		
		break;

	case SB_THUMBTRACK:
		{	
			if(m_flip == wyFalse)
				VERIFY(GetScrollInfo(hwnd, SB_HORZ, &scih));			
			else
				VERIFY(GetScrollInfo(hwnd, SB_VERT, &scih));			

			hiword = scih.nTrackPos;
			
			if(hiword == 0 || m_hscrollnumpos == 0)
				m_initcol = 0;
			else if(m_hscrollnumpos)
				m_initcol = hiword / m_hscrollnumpos;

			if(m_flip == wyFalse && m_initcol >= m_hskipcolcount)
				m_initcol = m_hskipcolcount;
		}

		break;
	}
    
    VERIFY(InvalidateRect(hwnd, NULL, TRUE));
	m_isscrolling = wyFalse;

	return 0;
}

LRESULT
CCustGrid::OnGetDLGCode(LPARAM lparam)
{
	if(lparam)
		return DLGC_WANTMESSAGE; 
	else
		return 0;
}

LRESULT
CCustGrid::OnMouseMove(WPARAM wparam, LPARAM lparam)
{
    wyInt32		    xpos, ypos, colcount = 0, rowcount = 0;
	wyInt32		    tempwidth;
	PGVCOLNODE	topcolumn = m_collist;
    PGVROWNODE      rowlist = m_rowlist;


	xpos = GET_X_LPARAM(lparam);
	ypos = GET_Y_LPARAM(lparam);
	
    if(m_flip == wyTrue)
        tempwidth = m_maxwidth;
    else
        tempwidth = GV_DEFWIDTH;
    
	if(!(ypos < m_hight))
		return 0;

    if(m_flip == wyTrue)
    {
        while(rowcount < m_initrow)
	    {
		    rowlist = rowlist->pNext;
            if(rowlist == 0)
                return 0;

		    rowcount++;
	    }

	    while(rowlist != NULL)
	    {
            tempwidth += rowlist->rowcx;

		    if((xpos >= tempwidth - 10)&&(xpos <= tempwidth + 10))
		    {
			    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			    break;
		    }

		    rowlist = rowlist->pNext;
	    }
    }
    else
    {
	while(colcount < m_initcol)
	{
		topcolumn = topcolumn->pNext;
        if(topcolumn == 0)
            return 0;
		colcount++;
	}

	while(topcolumn != NULL)
	{
		if(topcolumn->isshow == wyTrue)
			tempwidth += topcolumn->pColumn.cx;

		//Splitter window
		if((xpos >= tempwidth - 10)&&(xpos <= tempwidth + 10))
		{

			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			break;
		}

		topcolumn = topcolumn->pNext;
	}
    }
	return 1;
}

// implements wm_mousewheel.....
LRESULT
CCustGrid::OnMouseWheel(WPARAM wparam, LPARAM lparam)
{
    wyInt32		zdelta;
    SCROLLINFO	sci;
    wyInt32     count = 0;
    wyInt32     r1, r;
    RECT        rect;
    POINT       pt;

    if(m_pgvcurcolnode && m_pgvcurcolnode->hwndCombo && IsWindowVisible(m_pgvcurcolnode->hwndCombo))
    {
        pt.x = GET_X_LPARAM(lparam);
        pt.y = GET_Y_LPARAM(lparam);
        GetWindowRect(m_pgvcurcolnode->hwndCombo, &rect);
        
        if(PtInRect(&rect, pt))
        {
            SendMessage(m_pgvcurcolnode->hwndCombo, WM_MOUSEWHEEL, wparam, lparam);
            return 1;
        }
    }

	// if the number of rows is less then the rows that can be shown then we dont process it onyl

	if(GetRowCount() <= count + RowPerPage())
		return 0;

	zdelta = YOG_GET_WHEEL_DELTA_wparam(wparam);

	ZeroMemory(&sci, sizeof(SCROLLINFO));

	sci.cbSize	= sizeof(SCROLLINFO);
	sci.fMask	= SIF_ALL;

    if(m_flip == wyTrue)
        VERIFY(GetScrollInfo(m_hwnd, SB_HORZ, &sci));
    else
	    VERIFY(GetScrollInfo(m_hwnd, SB_VERT, &sci));

	if(m_iscomboediting == wyTrue || m_isediting == wyTrue)
		ApplyChanges();

	//m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);

	if(zdelta > 0)
    {
		if(m_flip == wyFalse && m_initrow <= 0 ||
           m_flip == wyTrue && m_initcol <= 0)
			return 0;
		
        if(m_flip == wyTrue)
        {
            r = RowPerPage() - m_visiblecols - m_initcol;
            r1 = GetLinesToScrollUserSetting();
            
            if(r>0)
                m_initcol -= r>=r1?r1:r;
            else
                m_initcol -= r1;

            if(m_initcol < 0)
                m_initcol = 0;
        }
        else
        {
		    m_initrow--;
            //m_initrow -= GetLinesToScrollUserSetting();
        }

        if(m_initrow <= 0) 
            m_initrow = 0;
	} 
    else 
    {
		//Once reached max. limit then just return
		if(m_flip == wyTrue && ((m_initrow * m_hight) + m_vscrollthumwidth) >= sci.nMax)
			return 1;

		else if(m_flip == wyFalse && (((m_initrow - 1) * GV_SCROLLPOS) + m_vscrollthumwidth) >= sci.nMax)
			return 1;

        if(m_flip == wyTrue)
	        count = m_initcol;
        else
            count = m_initrow;

	    // if the number of rows is less then the rows that can be shown then we dont process it onyl

		if(m_flip == wyFalse && GetRowCount() <= count + RowPerPage())
		    return 0;

        if((m_flip == wyFalse && m_initrow >= sci.nMax) ||
            (m_flip == wyTrue && m_initcol >= sci.nMax))  
        {
			return 0;
        }
		
		if(m_flip == wyTrue)
        {
            r = RowPerPage() - m_visiblecols - m_initcol;
            r1 = GetLinesToScrollUserSetting();

            //if(r > 0)
                m_initcol ++;//= r>=r1?r1:r;
            //else
              //  m_initcol += r1;

            if(m_initcol >= m_col)
                m_initcol = m_col - 1;

			if(m_initcol >= sci.nMax) 
				m_initcol = sci.nMax;
        }
        else
        {
		    m_initrow++;
           // m_initrow += GetLinesToScrollUserSetting();
			//if(m_initrow >= sci.nMax) 
			//	m_initrow = sci.nMax;
        }

       if(m_initrow >= sci.nMax) 
            m_initrow = sci.nMax;
	}
	
	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
	return 1;
}

LRESULT
CCustGrid::OnSplitterMouseMove(WPARAM wparam, LPARAM lparam)
{
	RECT	rectwin;
	POINT	pt;

	VERIFY(GetCursorPos(&pt));
	VERIFY(ScreenToClient(m_hwnd, &pt)); 	
	VERIFY(GetClientRect(m_hwnd, &rectwin));

	/*if(!(PtInRect(&rectwin, pt)))
	{
		ReleaseCapture();
		return 0;
	}*/

	VERIFY(MoveWindow(m_hwndsplitter, pt.x, 0, 2, 
        rectwin.bottom-rectwin.top, TRUE));
	VERIFY(SetCursor(LoadCursor(NULL, IDC_SIZEWE)));

	return 1;
}

LRESULT
CCustGrid::OnSplitterButtonUp(WPARAM wparam, LPARAM lparam)
{
	wyInt32     colcount = 0, colwidth = GV_DEFWIDTH, diff;
	RECT		rectwin, rctcell;
	POINT		pt;
	PGVCOLNODE	colnode = m_collist;
    PGVROWNODE	rownode = m_rowlist;

	VERIFY(GetCursorPos(&pt));
	VERIFY(ScreenToClient(m_hwnd, &pt)); 	
	VERIFY(GetClientRect(m_hwnd, &rectwin));
	
	if(pt.x > rectwin.right)
		pt.x = rectwin.right;

    if(m_flip == wyTrue)
    {
        while(colcount < m_initrow)	
        {
		    rownode = rownode->pNext;
		    colcount++;
	    } 
        
        colwidth = m_maxwidth;
        
        while(colcount < m_capturedcol)
        {
		    colwidth += rownode->rowcx;
            rownode = rownode->pNext;
		    colcount++;
	    }

        //restrict the minimum row width
        if(pt.x - colwidth < 50)
            rownode->rowcx = 50;
        else
            rownode->rowcx = pt.x - colwidth;

    }
    else
    {
		while(colcount < m_initcol)	
		{
			colnode = colnode->pNext;
			colcount++;
		} 

		while(colcount < m_capturedcol)
		{
			if(colnode && colnode->isshow == wyTrue)
				colwidth += colnode->pColumn.cx;

			colnode = colnode->pNext;
			colcount++;
		}

		diff = pt.x - colwidth;
		//we are limiting the column width
		if(diff > 25)
			colnode->pColumn.cx = pt.x - colwidth;	
		else
			colnode->pColumn.cx = 25;		
    }

	m_lpgvwndproc(m_hwnd, GVN_SPLITTERMOVE, colcount, 0);

	//H-Scroll hide if not necessary, for avoiding paiting issue happens at last row
	if(m_flip == wyFalse)
	{
		GetSubItemRect(m_row -1 , m_col - 1, &rctcell);
		if(rctcell.right < rectwin.right)
		{
			ShowScrollBar(m_hwnd, SB_HORZ, FALSE);		
	        }
	}
	
	VERIFY(InvalidateRect(m_hwnd, NULL, FALSE));
	return 1;
}

wyInt32 CCustGrid::GetHoveredColumn()
 {
 	POINT		pnt;	
	wyInt32		x;

	GetCursorPos(&pnt);
	ScreenToClient(m_hwnd, &pnt);

    if(m_flip == wyTrue)
        x = m_maxwidth;
    else
        x = GV_DEFWIDTH;
 
	if(pnt.y <= m_hight && pnt.x > x)
 	{
		return GetRowHeader(&pnt);
    }
   
 	return -1;
 }
 
 

LRESULT
CCustGrid::OnLButtonDown(WPARAM wparam, LPARAM lparam)
{
  	wyInt32	    count = 0, colcount = 0;
	wyInt32     selcol = 0;
	wyInt32     cursel = 0;
	wyInt32		x = 0, y = 0;
	RECT		rectwin, recttemp;
	POINT		pnt;
	PGVCOLNODE	pgvnode;
	PGVROWNODE	pgvrownode = m_rowlist;
    wyBool      checkbox = wyFalse;	
    wyBool      ret = wyTrue;
    	
	pnt.x = GET_X_LPARAM(lparam);
	pnt.y = GET_Y_LPARAM(lparam);

	//keep bufforn while checking when WM_LBUTTONUP, for sort icon on column header 
	m_pointlbuttondown.x = pnt.x;
	m_pointlbuttondown.y = pnt.y;
    
	VERIFY(GetClientRect(m_hwnd, &rectwin));
	CopyMemory(&recttemp, &rectwin, sizeof(RECT));	
	
	pgvrownode = m_rowlist;
	
	if(m_isediting == wyTrue)
		ret = EndLabelEdit(m_curselrow, m_curselcol);
	else if(m_iscomboediting == wyTrue)
		ret = EndComboEdit(m_curselrow, m_curselcol);

    if(ret == wyFalse)
    {
        return 0;
    }

    if(m_flip == wyTrue)
        x = m_maxwidth;
    else
        x = GV_DEFWIDTH;

	y = m_hight;

	// first we check if its in column space so that we can make it a propersplitter.
	pgvnode = m_collist;

	// we move to column from which the column status start
    if(m_flip == wyTrue)
    {
        while(count < m_initrow)
	    {
		    pgvrownode = pgvrownode->pNext;
            if(!pgvrownode)
                return 0;
		    count++;
	    }
    }
    else
    {
	    while(count < m_initcol)
	    {
		    pgvnode = pgvnode->pNext;
            if(!pgvnode)
                return 0;
		    count++;
	    }
    }	

	if(pnt.y <= y && pnt.x > x)
	{
		cursel = GetRowHeader(&pnt);
		if(cursel >= 0)
		{
			if(m_flip == wyTrue)
				m_curselrow = cursel; 
			else
				m_curselcol = cursel;
			VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));	
		}
		if(m_curselcol == -1)
			m_curselcol = 0;
		if(m_curselrow == -1)
			m_curselrow = 0;		
		return HandleColumn(pgvrownode, pgvnode, &pnt, &rectwin, x, count);
	}
	if((pnt.x <= x) && (m_flip != wyTrue) )
	{
		if(m_curselrow == -1)
			m_curselrow = 0;	
		
	}
	if((pnt.x <= x) && (m_flip == wyTrue))
	{
		selcol = (pnt.y / m_hight) + m_initcol ;
		
		if(selcol <= m_col && ((selcol - m_initcol) != 0 ))
			m_curselcol = selcol - 1 ;
		
		VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

		if(m_curselrow == -1)
			m_curselrow = 0;
			
	}

	// Now we see if the its row check zone.
    if((m_flip == wyTrue && pnt.y <= y) || (m_flip == wyFalse && pnt.x < x))
        checkbox = wyTrue;

	if((m_exstyle & GV_EX_ROWCHECKBOX) && checkbox == wyTrue)
    {
		// that means we need to check it.
		if(m_flip == wyTrue)
            y = m_maxwidth;
        else
		    y = m_hight;
		
        if(m_flip == wyFalse && (pnt.y > 0) && (pnt.y <= y) &&
                (pnt.x > 0) && (pnt.x < x ))
        {
               // check the box
                HandleInitialButton();
                return 0;
        }
        else
        {

		// in owner data mode it is very simple 
		for(count = m_initrow; count <= m_row; count++)
        {
            /// When filp is active we check for the x axis when traversing through the check boxes
			if(m_flip == wyFalse && (pnt.y > y) && (pnt.y < (y + m_hight)) ||
                m_flip == wyTrue && (pnt.x > x) && (pnt.x < (x + pgvrownode->rowcx)))
            {
				//if the last selected row is updated then we need to save the row 
				if((count != m_curselrow) && 
                    (!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, count, m_curselcol) ||
                    !m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol)))
					return 0;

				// This is the row.
                return HandleRow(pgvrownode, count);
			}

			if(m_flip == wyTrue)
                x += pgvrownode->rowcx;
            else
			    y += m_hight;

            if(m_flip == wyTrue && pgvrownode)
                pgvrownode = pgvrownode->pNext;
		}
	}
    }

	//Check wheter L_BUTTON event happened on outside the data disply area, then return ot
	if(m_flip == wyFalse && !(m_exstyle & GV_EX_STRETCH_LAST_COL))
	{
		GetSubItemRect(m_initrow, m_col - 1, &recttemp);
		if(!PtInRect(&recttemp, pnt))
		{
			if(recttemp.right > 0 && pnt.x > recttemp.right)
			{
				SetFocus(m_hwnd);
				return 0;
			}
		}
	}

	for(count = m_initrow; count <= m_row; count++)
    {
		for(colcount = m_initcol; colcount < m_col; colcount++)
        {
			GetSubItemRect(count, colcount, &recttemp);

			if(PtInRect(&recttemp, pnt))
			{
				// It is already selected so we begin editing it.
				// Or if it is bool type then we check or uncheck it
				if(m_curselrow == count && m_curselcol == colcount && m_first == wyFalse)	
                    return HandleCell(count, colcount);
                else	
                {
					// first check if we want to change the row or not.
					if((m_curselrow != -1) && (count != m_curselrow) && 
                        (!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, count, colcount) ||
                        !(m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol))))
						return 0;

                    HandleNonTextCell(count, colcount, &pnt, &recttemp);
					/// Otherwise put it into column selection mode.
                    HandleCellSelection(count, colcount, &recttemp, &rectwin);
                    return 1;
				}
			}
		}

        if(m_flip == wyFalse && (m_exstyle & GV_EX_STRETCH_LAST_COL))
        {
            if(pnt.y <= recttemp.bottom && pnt.y >= recttemp.top)
            {
                if((m_curselrow != -1) && (count != m_curselrow) && 
                    (!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, count, colcount) ||
                    !(m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol))))
						return 0;

                if(count != m_curselrow)
                {
                    m_curselrow = count;
                    m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, m_curselcol);
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    UpdateWindow(m_hwnd);
                }

                break;
            }
        }
	}

	//	if it has reached here then no valid column items were selected so we
	//	set the focus to the grid 
	SetFocus(m_hwnd);

	return 0;
}


wyInt32 
CCustGrid::HandleColumn(PGVROWNODE pgvrownode, PGVCOLNODE pgvnode, POINT *pnt, RECT *rectwin, wyInt32 x, wyInt32  count)
{
	while(1)
	{
		if(m_flip == wyTrue)
        {
            if(pgvrownode == NULL)
                break;
            x += pgvrownode->rowcx;
        }
        else
        {
            if(pgvnode == NULL)
                break;

			if(pgvnode && pgvnode->isshow == wyTrue)
				x += pgvnode->pColumn.cx;
        }

		//handling splitter window
		if((pnt->x >= x - 10) && (pnt->x <= x + 10))
		{
			/// First complete all job.
			ApplyChanges();
			VERIFY(MoveWindow(m_hwndsplitter, pnt->x, 0, 2, 
                rectwin->bottom - rectwin->top - (GetSystemMetrics(SM_CYHSCROLL)), TRUE));

			m_capturedcol = count;
			SetCapture(m_hwndsplitter);
			ShowWindow(m_hwndsplitter, TRUE);
			return 1;
		}
        count++;
        
        if(m_flip == wyTrue)
		    pgvrownode = pgvrownode->pNext;    
        else
            pgvnode = pgvnode->pNext;
	}
	
	return 1;
}


wyInt32 
CCustGrid::HandleRow(PGVROWNODE pgvrownode, wyInt32 count)
{
	if(!GetOwnerData())
    {
		pgvrownode = GetRowStruct(count);

		if(!pgvrownode)
			return 0;
		
        pgvrownode->excheck = (pgvrownode->excheck) ? wyFalse : wyTrue;

        if(pgvrownode->excheck == wyTrue)
            m_checkcount++;
        else
            m_checkcount--;

        m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, count, 0);
		
		VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
		return 1;
	} 
    else 
    {
		m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, count, 0);
			
		VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
		return 1;
	}
}

wyInt32 
CCustGrid::HandleCell(wyInt32 row, wyInt32 col)
{
    PGVCOLNODE	pgvcolnode = GetColNodeStruct(col);

    if(pgvcolnode->pColumn.mask & GVIF_BOOL)
	    return ToggleBoolValue(row, col);
    else if(pgvcolnode->pColumn.mask & GVIF_BUTTON || 
        pgvcolnode->pColumn.mask & GVIF_TEXTBUTTON)
	    return ProcessButtonClick(row, col);
    else if(pgvcolnode->pColumn.mask & GVIF_LIST ||
        pgvcolnode->pColumn.mask & GVIF_DROPDOWNLIST || 
        pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE ||
        pgvcolnode->pColumn.mask & GVIF_DROPDOWNNLIST)
	    return BeginColumnEdit(row, col);

    SetFocus(m_hwnd);
    BeginColumnEdit(row, col);
    VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

    return 1;
}

void 
CCustGrid::HandleCellSelection(wyInt32 row, wyInt32 col, RECT *recttemp, RECT *rectwin)
{
    wyBool  ischangerow = wyFalse;
    RECT    rectsub;

	if(m_curselrow != row)
		ischangerow = wyTrue;
    
	m_curselrow  = row;
	m_curselcol  = col;
    m_first      = wyFalse;

	GetSubItemRect(m_curselrow, m_curselcol, &rectsub);
	
	/** Post 8.04 Beta 1(single click on partial visible column dont bring to visible, only Double click will do that now)
	while(rectsub.right > rectwin->right)
	{
		if(m_flip == wyFalse && m_initcol != (m_col-1))
		{
            m_initcol++;

			//For ssetting H-sroll position
			m_hscrollpos = m_initcol * m_hscrollnumpos; 
		}

        if(m_flip == wyTrue && m_initrow != (m_row - 1))
            m_initrow++;

		if(m_initcol == m_curselcol)
			break;

		GetSubItemRect(m_curselrow, m_curselcol, &rectsub);
	}
	*/

	// Also change the row.
	if(m_flip == wyFalse && recttemp->bottom > rectwin->bottom - GetSystemMetrics(SM_CYHSCROLL))
		++m_initrow;
	
    if(m_flip == wyTrue && recttemp->bottom > rectwin->bottom)
		++m_initcol;		
	
	if(ischangerow == wyTrue)
		m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, m_curselcol);
	
	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
	SetFocus(m_hwnd);

    return;
}

void 
CCustGrid::HandleNonTextCell(wyInt32 row, wyInt32 col, POINT *pnt, RECT *rect)
{
	PGVCOLNODE	pgvcolnode = GetColNodeStruct(col);
    wyInt32     buttonlength;
    
	if(pgvcolnode->pColumn.mask & GVIF_BOOL)
		ToggleBoolValue(row, col);
	else 
    {
        buttonlength = (rect->right - rect->left)/3;
        rect->left = rect->right - buttonlength + 3;
        
        if((pnt->x < rect->right && pnt->x > rect->left && 
            pnt->y < rect->bottom && pnt->y > rect->top)
            || pgvcolnode->pColumn.mask & GVIF_BUTTON )
        {
            if(pgvcolnode->pColumn.mask & GVIF_BUTTON || 
                pgvcolnode->pColumn.mask & GVIF_TEXTBUTTON)
				PostMessage(m_hwnd, GVM_BUTTONCLICK, row, col);
			else if(pgvcolnode->pColumn.mask & GVIF_LIST || 
                    pgvcolnode->pColumn.mask & GVIF_DROPDOWNLIST || 
                    pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE ||
                    pgvcolnode->pColumn.mask & GVIF_DROPDOWNNLIST)
				PostMessage(m_hwnd, GVM_STARTLISTEDIT, row, col);
        }
    }
    return;
}

wyInt32
CCustGrid::HandleInitialButton()
{
	PGVROWNODE	pgvrownode = NULL;
    wyInt32     count = 0, ret = -1, chcount = 0;
    pgvrownode = m_rowlist;
    GVINITIALBUTTONINFO info = {0};
    
    if(!GetOwnerData())
    {
        if(m_flip == wyFalse)
        {
            info.checkstate = m_selallinfo.checkstate;
			info.sendmessage = wyFalse;
            // some instances of select all require the row to be checked only if after processing
			// no error is returned. if this is the case, we select all rows individually 
			// and then update the select all check box (info.sendmessage should be set to true in this case.
            if(!m_lpgvwndproc(m_hwnd, GVN_SELECTALLCLICK, (WPARAM)&info, 0))
            {
                return 0;
            }

            m_selallinfo.checkstate = info.checkstate;
        }


        while(m_flip == wyFalse && pgvrownode)
	    {
            if(m_selallinfo.checkstate == BST_CHECKED || m_selallinfo.checkstate == BST_INDETERMINATE)
            {
                if(pgvrownode->excheck == wyTrue)
				{
					pgvrownode->excheck = wyFalse;
					if(info.sendmessage)
                    {
						ret = m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, count , 0);
					}	
				}

            }  
            else if(m_selallinfo.checkstate == BST_UNCHECKED )
            {
                if(pgvrownode->excheck == wyFalse)
				{
					pgvrownode->excheck = wyTrue;
					if(info.sendmessage)
                    {
						ret = m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, count , 0);
						if(ret == 0)
							chcount--;
					}	
				}
            }

            pgvrownode = pgvrownode->pNext;
			count++;
		    chcount++;
        }
		if(!info.sendmessage)
        {
			if(m_selallinfo.checkstate == BST_CHECKED || m_selallinfo.checkstate == BST_INDETERMINATE)
			{
				m_selallinfo.checkstate = BST_UNCHECKED;
				m_checkcount = 0;
			}  
			else if(m_selallinfo.checkstate == BST_UNCHECKED )
			{
				m_selallinfo.checkstate = BST_CHECKED;
				m_checkcount = m_row;
			}
		}
		else
		{
			m_checkcount = chcount;
			SetSelAllState();
		}

        
		//
		InvalidateRect(m_hwnd, NULL, TRUE);
		return 1;
	} 
    else 
    {
        m_lpgvwndproc(m_hwnd, GVN_SELECTALLCLICK, (WPARAM)&info, 0);
        m_selallinfo.checkstate = info.checkstate;
			
		VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
		return 1;
	}
}

LRESULT
CCustGrid::HandleDblClick(WPARAM wparam, LPARAM lparam)
{
	wyInt32     count = 0, colcount = 0;
	wyInt32	    x = 0, y = 0;
	RECT		rectwin, recttemp;
	POINT		pnt;
	PGVCOLNODE	pgvnode = NULL, pgvcolnode = NULL;
	PGVROWNODE	pgvrownode = NULL;
    wyBool      ret = wyTrue;
		
	pnt.x = GET_X_LPARAM(lparam);
	pnt.y = GET_Y_LPARAM(lparam);

	//post message for WM_LBUTTONDBCLICK
    m_lpgvwndproc(m_hwnd, GVN_LBUTTONDBLCLK, pnt.x, pnt.y);

	VERIFY(GetClientRect(m_hwnd, &rectwin));
	CopyMemory(&recttemp, &rectwin, sizeof(RECT));
	
	y = m_hight;
	// first we check if its in column space so that we can make it a propersplitter.
	x = GV_DEFWIDTH;
	
	pgvnode = m_collist;
    pgvrownode = m_rowlist;

    if(m_isediting == wyTrue)
		ret = EndLabelEdit(m_curselrow, m_curselcol);
	else if(m_iscomboediting == wyTrue)
		ret = EndComboEdit(m_curselrow, m_curselcol);

    if(ret == wyFalse)
    {
        return 0;
    }

	// we move to column from which the column status start
	if(m_flip == wyTrue)
    {
        while(count < m_initrow)
	    {
		    pgvrownode = pgvrownode->pNext;
		    count++;
	    }
    }
    else
    {
	    while(count < m_initcol)
	    {
		    pgvnode = pgvnode->pNext;
		    count++;
	    }
    }


	if(pnt.y <= y)
        return HandleColumn(pgvrownode, pgvnode, &pnt, &rectwin, x, count);

	// now we see if the its row check zone.
	if((m_exstyle & GV_EX_ROWCHECKBOX)&&(pnt.x < x))
    {
		// that means we need to check it.
		y = m_hight;
		
		// in owner data mode it is very simple 
		for(count = m_initrow; count <= m_row; count++)
        {
			if((pnt.y > y) && (pnt.y < (y + m_hight)))
                return HandleRow(pgvrownode, count);

			y += m_hight;
		}
	}

	//Check wheter L_BUTTONDBCLICK event happened on outside the data disply area, then return ot
	if(m_flip == wyFalse)
	{
		GetSubItemRect(m_initrow, m_col - 1, &recttemp);
		if(!PtInRect(&recttemp, pnt))
		{
			if(recttemp.right > 0 && pnt.x > recttemp.right)
			{
				SetFocus(m_hwnd);
				return 0;
			}
		}
	}

	for(count = m_initrow; count <= m_row; count++)
    {
		for(colcount = m_initcol; colcount < m_col; colcount++)
        {
            GetSubItemRect(count, colcount, &recttemp);
			if(PtInRect(&recttemp, pnt))
			{
		        // first check if we want to change the row or not.
		        if((m_curselrow != -1) && (count != m_curselrow)&& 
                    (!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, count, colcount) || 
                    !(m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol))))
			        return 0;

				//Move the column to left if its visble partially
				while(recttemp.right > rectwin.right)
				{
					if(m_flip == wyFalse && m_initcol != (m_col-1))
						m_initcol++;						
					
			       if(m_flip == wyTrue && m_initrow != (m_row - 1))
						m_initrow++;					   
				   
					if(m_initcol == m_curselcol)
						break;
					
					 GetSubItemRect(count, colcount, &recttemp);
				}

		        pgvcolnode = GetColNodeStruct(colcount);

		        if(pgvcolnode->pColumn.mask & GVIF_BOOL)
			        ToggleBoolValue(count, colcount);
		        else if(pgvcolnode->pColumn.mask & GVIF_BUTTON || 
                        pgvcolnode->pColumn.mask & GVIF_TEXTBUTTON)
		    	    PostMessage(m_hwnd, GVM_BUTTONCLICK, count, colcount);
		        else if(pgvcolnode->pColumn.mask & GVIF_LIST || 
                        pgvcolnode->pColumn.mask & GVIF_DROPDOWNNLIST || 
                        pgvcolnode->pColumn.mask & GVIF_DROPDOWNLIST || 
                        pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE)
			        PostMessage(m_hwnd, GVM_STARTLISTEDIT, count, colcount);
                else
                {
		            // Otherwise put it into column selection mode.
		            m_curselrow  = count;
		            m_curselcol  = colcount;
			        BeginColumnEdit(count, colcount);
                }

                VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
                SetFocus(m_hwnd);
                return 1;
            }
		}
	}

	/*	if it has reached here then no valid column items were selected so we
		setthe focus to the grid */
	SetFocus(m_hwnd);

	return 0;
}

// When the mouse button is up on the custom grid. Right now we only handle click on column to allow column sorting
LRESULT
CCustGrid::OnLButtonUp(WPARAM wparam, LPARAM lparam)
{
	wyInt32     colcount = 0;
	wyInt32     x = 0, y = 0;
	RECT		rectwin;
	POINT		pnt;
	PGVCOLNODE	pgvnode;

    pnt.x	= GET_X_LPARAM(lparam);
	pnt.y	= GET_Y_LPARAM(lparam);

	VERIFY(GetClientRect(m_hwnd, &rectwin));
	
	y = m_hight;
	// first we check if its in column space so that we can make it a propersplitter.
	if(m_flip == wyTrue)
        x = m_maxwidth;
    else
	x = GV_DEFWIDTH;
	pgvnode = m_collist;

	// we move to column from which the column status start
	while(pgvnode && colcount < m_initcol)
	{
		pgvnode = pgvnode->pNext;
		colcount++;
	}

	if((m_flip == wyFalse && pnt.y <= y) || (m_flip == wyTrue && pnt.x <= x))
	{
		while(pgvnode != NULL)
		{
			// check whether ith column was clicked and if yes then send a notification(also restricting 'sor icon' appear only L_Button Down & UP on same column header
			/// If flip is on then we check for the y - coordinate and not the x - coordinate
			if((m_flip == wyFalse && 
				(pnt.x > x) && 
				(pnt.x < (x + pgvnode->pColumn.cx)) && 
				(m_curselcol == colcount) 
				&& (pnt.y <= GV_DEFWIDTH && m_pointlbuttondown.y >= 0 && m_pointlbuttondown.y <= GV_DEFWIDTH)) 
				|| (m_flip == wyTrue && (pnt.y > y) && (pnt.y < (y + GV_DEFWIDTH))))
			{	
				m_lpgvwndproc(m_hwnd, GVN_COLUMNCLICK, colcount, 0);
				return 1;
			}

            if(m_flip == wyTrue)
                y += m_hight;
            else
			x += pgvnode->pColumn.cx;

			pgvnode = pgvnode->pNext;
			colcount++;
		}

        //        if(m_exstyle & GV_EX_ROWCHECKBOX)
        //{
        //      if(m_flip == wyFalse && (pnt.y > 0) && (pnt.y <= y) &&
        //        (pnt.x > 0) && (pnt.x < x ))
        //    {
        //    // check the box
        //        HandleInitialButton();
        //    }
        //}
	}

	// we dont process anything else
	return 1;
}

wyInt32
CCustGrid::GetRowColumn(LPARAM lparam, wyInt32 *row, wyInt32 *col)
{
    wyInt32	    count = 0, colcount = 0;
    RECT		recttemp = {0};
	POINT		pnt;
    
    *row = -1;
    *col = -1;

    pnt.x = GET_X_LPARAM(lparam);
	pnt.y = GET_Y_LPARAM(lparam);

	//Check wheter R_BUTTON event happened on outside the data disply area, then return ot
	if(m_flip == wyFalse )
	{
		GetSubItemRect(m_initrow, m_col - 1, &recttemp);
		if(!PtInRect(&recttemp, pnt))
		{
			if(recttemp.right > 0 && pnt.x > recttemp.right)
				return -1;
		}

		//If even happend on row-check box
		GetSubItemRect(m_initrow, 0, &recttemp);
		if(!PtInRect(&recttemp, pnt))
		{
			if(pnt.x < recttemp.left)
				return -1;
		}

	}
    
	for(count = m_initrow; count <= m_row; count++)
    {
		for(colcount = m_initcol; colcount < m_col; colcount++)
        {
            GetSubItemRect(count, colcount, &recttemp);

			if(PtInRect(&recttemp, pnt))
                goto end;
		}
	}
    return -1;

end:
    *row = count;
    *col = colcount;
	
    return 1;
}

LRESULT
CCustGrid::OnRButtonDown(WPARAM wparam, LPARAM lparam)
{
    wyInt32     row = -1, col = -1, temp;
    POINT	    pnt;
    wyBool      retflag = wyFalse;

    pnt.x		=   (LONG)GET_X_LPARAM(lparam); 
	pnt.y		=   (LONG)GET_Y_LPARAM(lparam); 

	SetFocus(m_hwnd);
    if(ApplyChanges() == wyFalse)
        return 0;

    GetRowColumn(lparam, &row, &col);
    
    if(row < 0 || col < 0)
    {
        if(m_flip == wyTrue)
            m_curselrow = GetRowHeader(&pnt);
        else
            m_curselcol = ((col = GetRowHeader(&pnt)) != -1) ? col : 
                                    ((m_exstyle & GV_EX_STRETCH_LAST_COL) ? m_curselcol : col);
    }
    else
    {
        if(m_curselrow == row || 
            (m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, row, col) && 
            m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol)))
        {
            temp = m_curselrow;
            m_curselcol = col;
            m_curselrow = row;

            if(temp != m_curselrow)
            {
                m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, m_curselcol);
            }
        }
        else
        {
            retflag = wyTrue;
        }
    }

    //EnsureVisible(row, col);

    InvalidateRect(m_hwnd, NULL, FALSE);

    if(retflag == wyTrue)
    {
        return 0;
    }
    
	return m_lpgvwndproc(m_hwnd, GVN_RBUTTONDOWN, wparam, lparam);
}

LRESULT
CCustGrid::OnLButtonDblClk(WPARAM wparam, LPARAM lparam)
{
    return HandleDblClick(wparam, lparam);
}

LRESULT
CCustGrid::OnSetFocus(WPARAM wparam, LPARAM lparam)
{
    m_lpgvwndproc(m_hwnd, GVN_SETFOCUS, wparam, lparam);

	if(m_curselcol == -1)
	{
		//m_curselcol = 0;
		m_hscrollpos = 0;
	}

	if(m_curselrow == -1)
	{
		//m_curselrow = 0;
		m_vscrollpos = 0;
	}

	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return 1;
}

LRESULT
CCustGrid::OnKillFocus(WPARAM wparam, LPARAM lparam)
{
	HWND        losewin = (HWND)wparam;
	wyInt32     change = 1;
	PGVCOLNODE	colnode;
	GVDISPINFO	disp = {0};

	colnode = GetColNodeStruct(m_curselcol);

	if(colnode && !(losewin == m_hwndedit || losewin == m_hwnd || losewin == colnode->hwndCombo))
    {
		ApplyChanges();
		disp.nRow = m_curselrow; 
        disp.nCol = m_curselcol;
		m_lpgvwndproc(m_hwnd, GVN_KILLFOCUS, (WPARAM)&disp, (LPARAM)&change);

		if(change)
			PostMessage(m_hwnd, GVM_SETFOCUS, 0, 0);
		else
			SetFocus(losewin);

	} 
	
	VERIFY(InvalidateRect(m_hwnd, NULL, FALSE));

	return 1;
}

LRESULT
CCustGrid::OnKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	RECT		rectwin, rectsub;
	wyBool		ensure = wyFalse;
    wyInt32     checkstate = 0, row;
	SHORT		state;
    wyBool      ctrlpressed = wyFalse, shiftpressed = wyFalse;      

	ZeroMemory(&rectwin, sizeof(rectwin));
	ZeroMemory(&rectsub, sizeof(rectsub));

	VERIFY(GetClientRect(hwnd, &rectwin));

	// know whether ctrl key is pressed or not
    if(m_iskeyboard == wyTrue)
    {
	    state = GetKeyState(VK_CONTROL);
 
        if(state & 0x8000)
        {
            ctrlpressed = wyTrue;
        }
        else
        {
            state = GetKeyState(VK_SHIFT);

            shiftpressed = (state & SHIFTED) ? wyTrue : wyFalse;
        }
    }
    else
        ctrlpressed = wyTrue;

	switch(wparam)
	{
	case VK_F2:
		BeginColumnEdit(m_curselrow, m_curselcol);
		break;

	case VK_HOME:
        OnHomeKey(ctrlpressed);
		break;

	case VK_END:
        OnEndKey(ctrlpressed);
		break;

	case VK_UP:
        if(m_flip == wyTrue)
            ensure = OnLeftKey();
        else
            ensure = OnUpKey();
		break;

		// If the selection is already at the bottom then we ask whether to add a new row or not.
	case VK_DOWN:
		if(m_curselrow == -1)
			break;
		
        if((m_curselrow == m_row - 1) && (m_flip == wyFalse))
			return InsertNewRowOnKeyPress(hwnd);			

        if((m_curselcol == m_col - 1) && (m_flip == wyTrue))
			return InsertNewRowOnKeyPress(hwnd);			

		else
        {
            if(m_flip == wyTrue)
                ensure = OnRightKey(&rectwin);
		else
            ensure = OnDownKey(&rectwin);
        }

		break;

	case VK_LEFT:
        if(m_flip == wyTrue)
            ensure = OnUpKey();
        else
        ensure = OnLeftKey();
		break;

	case VK_TAB:
		PostMessage(hwnd, GVM_PROCESSTAB, 0, 0);
		break;

	case VK_F4:
        OnSpaceKey();
        break;

	case VK_SPACE:
        if(ctrlpressed == wyTrue && m_isediting == wyFalse && m_iscomboediting == wyFalse)
        {
            row = GetCurSelRow();
            if(!GetOwnerData())
            {
                checkstate = GetRowCheckState(row);
                SetRowCheckState(row, checkstate? wyFalse : wyTrue);
                m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, row, 0);
            }
            else
            {
                m_lpgvwndproc(m_hwnd, GVN_CHECKBOXCLICK, row, 0);
            }

        }
        else
        {
        OnSpaceKey();
        }
		break;

	case VK_RIGHT:
        {
        if(m_flip == wyTrue)
            ensure = OnDownKey(&rectwin);
        else
			ensure = OnRightKey(&rectwin);
        }
		break;

	case VK_RETURN:
		ProcessTabPress();
		break;

	case VK_DELETE:
        OnDelete();
		break;

	case VK_ESCAPE:
		PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
		break;

	case VK_NEXT:
		CalculatePageDown(wyTrue);
		break;

	case VK_PRIOR:
		CalculatePageUp(wyTrue);
		break;

	case VK_APPS:
		m_lpgvwndproc(m_hwnd, GVN_RBUTTONDOWN, 0, 0);
		break;

	case 'C':
        if(ctrlpressed == wyTrue && m_isediting == wyFalse && m_iscomboediting == wyFalse)
			CopyDataToClipboard();
		break;

	case 'V':
		if(ctrlpressed == wyTrue && m_isediting == wyFalse && m_iscomboediting == wyFalse)
			CopyDataFromClipboard();
		break;

    case VK_INSERT:
        if(m_isediting == wyFalse && m_iscomboediting == wyFalse)
        {
            if(ctrlpressed == wyTrue)
            {
			    CopyDataToClipboard();
            }
            else if(shiftpressed == wyTrue)
            {
                CopyDataFromClipboard();
            }
        }
        break;

	default:
		break;
	}

	SetFocus(m_hwnd);
	
	if(ensure == wyTrue)
		EnsureVisible(m_curselrow, m_curselcol);

	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return 1;
}


void 
CCustGrid::OnDelete()
{
	LONG    rowcount = CustomGrid_GetRowCount(m_hwnd);
    wyInt32 ret;
    GVDISPINFO  disp = {0};
    wyChar      usrdata[2] = {0};

	if(!rowcount)
		return;

	ret = m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, m_curselrow, m_curselcol);
	if(!ret)
        return;

	if(!GetOwnerData())
    {
		SetSubItemText(m_curselrow, m_curselcol, "");
		m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(m_curselrow, m_curselcol), (LPARAM)L"");
	} 
    else
    {
		disp.nRow = m_curselrow; 
        disp.nCol = m_curselcol;
		disp.text = usrdata; 
        disp.cchTextMax = 0;

		m_lpgvwndproc(m_hwnd, GVN_SETOWNERCOLDATA, (WPARAM)&disp, 0);
        m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(m_curselrow, m_curselcol), (LPARAM)usrdata);
	}
    return;
}

wyBool 
CCustGrid::OnRightKey(RECT *rectwin)
{
    RECT    rectsub;
	PGVCOLNODE	pgvcolnode = NULL;

    if(m_curselcol == m_col - 1)
    {
        return wyFalse;
    } 
    else 
	{
		//if(m_curselcol != m_col - 1)
	    m_curselcol++;

		if(!GetOwnerData())
		{
			pgvcolnode = GetColNodeStruct(m_curselcol);

			while(pgvcolnode && pgvcolnode->isshow == wyFalse)
			{
				pgvcolnode = GetColNodeStruct(++m_curselcol);
			}
		}

	    // we move right till we reach the correct initcol but initcol is never > then m_curselcol
	    GetSubItemRect(m_curselrow, m_curselcol, &rectsub);
		while((rectsub.right > rectwin->right) && m_flip == wyFalse)
	    {
		    if(m_initcol != (m_col-1))
                m_initcol++;

		GetSubItemRect(m_curselrow, m_curselcol, &rectsub);

	    }

        if((rectsub.bottom > rectwin->bottom) && m_flip == wyTrue)
            m_initcol++;
	}

    return wyTrue;
}

void 
CCustGrid::OnSpaceKey()
{
    PGVCOLNODE	pgvcolnode;

	pgvcolnode = GetColNodeStruct(m_curselcol);

	if(!pgvcolnode)
		return;

	BeginColumnEdit(m_curselrow, m_curselcol);

    return;
}

wyBool 
CCustGrid::OnLeftKey()
{
	PGVCOLNODE	pgvcolnode = NULL;

	if(m_curselcol == 0)
		return wyFalse;

	m_curselcol--;

	if(!GetOwnerData())
	{
		pgvcolnode = GetColNodeStruct(m_curselcol);

		while(pgvcolnode && pgvcolnode->isshow == wyFalse)
		{
			pgvcolnode = GetColNodeStruct(--m_curselcol);
		}
	}

	if(m_curselcol < m_initcol)
		m_initcol--;
	
    return wyTrue;
}


void 
CCustGrid::OnHomeKey(wyBool ctrlpressed)
{
    LONG selrow = m_curselrow;

    if(ctrlpressed == wyTrue)
    {
		m_curselcol = m_curselrow = 0;
		m_initrow = m_initcol = 0;
	}
    else 
    {
        if(m_flip == wyTrue)
        {
		    m_curselrow = 0;
		    m_initrow = 0;
	    }
        else 
        {
		    m_curselcol = 0;
		    m_initcol = 0;
	    }
    }

    if(selrow != m_curselrow)
    {
        m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    }

    return;
}

void 
CCustGrid::OnEndKey(wyBool ctrlpressed)
{
    LONG selrow = m_curselrow;

    if(m_flip == wyTrue)
    {
        if(ctrlpressed == wyTrue)
        {
	        m_curselcol = (m_col - 1);
	        EnsureVisible(m_curselrow, m_curselcol);
        } 
        else 
        {
	        m_curselrow = m_row - 1;
            m_initrow = m_row - 1;
        }

        if(m_form)
            m_initrow = m_curselrow;
    }
    else
    {
        if(ctrlpressed == wyTrue)
        {
	        m_curselrow = (m_row - 1);
	        EnsureVisible(m_curselrow, m_curselcol);
        } 
        else 
        {
	        m_curselcol = m_col - 1;
            ShowLastCol();
        }

        if(m_form)
            m_initrow = m_curselrow;
    }

    if(selrow != m_curselrow)
    {
        m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    }

    return;
}

wyBool 
CCustGrid::OnUpKey()
{
    wyInt32 ret;

	if(m_curselrow == -1 || m_curselrow == 0)
		return wyFalse;

    if(!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, m_curselrow - 1, m_curselcol))
    {
        return wyFalse;
    }

	ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);

	if(!ret)
		return wyFalse;

	m_curselrow--;

    if(m_curselrow < m_initrow)
		m_initrow--;

	m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    return wyTrue;
}

wyBool 
CCustGrid::OnDownKey(RECT *rectwin)
{
    wyInt32 ret;
    RECT    rectsub;

    if(m_row == 0)
        return wyFalse;
    
    if(!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, m_curselrow + 1, m_curselcol))
    {
        return wyFalse;
    }

    ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);
    if(!ret)
	    return wyFalse;

    if((m_curselrow == m_row - 1) && m_flip == wyTrue)
        return wyFalse;

    m_curselrow++;
    GetSubItemRect(m_curselrow, m_curselcol, &rectsub);

    if((rectsub.bottom + m_hight > rectwin->bottom) && m_flip == wyFalse)
	    m_initrow++;

    if((rectsub.right > rectwin->right) && m_flip == wyTrue)
	    m_initrow++;

    if(m_form == wyTrue)
	    m_initrow++;
	
    m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    return wyTrue;
}

LRESULT
CCustGrid::ProcessTabPress()
{
	wyInt32 nrowperpage;	
	wyInt32	ncurselrow = m_curselrow, ncurselcol = m_curselcol;
	RECT	rectwin;
	SHORT	state;
	LRESULT	ret;
	PGVCOLNODE	pgvcolnode = NULL;

	state	= GetKeyState(VK_SHIFT);

	if(m_col == 0 || m_row == 0)
    {
        if(state & SHIFTED)		// if shift is pressed
            m_lpgvwndproc(m_hwnd, GVN_PREVTABORDERITEM, 0, 0);
        else
            m_lpgvwndproc(m_hwnd, GVN_NEXTTABORDERITEM, 0, 0);
		return 0;
    }

	// now notify the user that the selection is changing.
	if(state & SHIFTED)		// if shift is pressed
	{
		// we check if its the first column then we move it to the previous col.	
		if(ncurselcol == 0)
		{
			if(ncurselrow != 0) // && (m_curselrow != m_row-1))
			{
				ncurselrow--;
				ncurselcol = m_col - 1;
			}
			else
			{
				// the cursor is in the first cell, pressing SHIFT + TAB will send a the cursor outside Grid control if want
				m_lpgvwndproc(m_hwnd, GVN_PREVTABORDERITEM, 0, 0);
				return 0;
			}
		}
		else
		{
			ncurselcol--;

			if(!GetOwnerData())
			{
				pgvcolnode = GetColNodeStruct(ncurselcol);

				while(pgvcolnode && pgvcolnode->isshow == wyFalse)
				{
					pgvcolnode = GetColNodeStruct(--ncurselcol);
				}
			}
			
			// now check if the init col is also changed then we change the init col also.
			if(ncurselcol < m_initcol)
				m_initcol--;
		}
	}
	else			// shift key is not pressed.
	{
		if(ncurselcol == (m_col-1))
		{
			if(ncurselrow != (m_row-1))		// if not the last row
			{
				if(m_flip == wyTrue)
                    nrowperpage = ColumnPerPage();    
                else
				    nrowperpage = RowPerPage();
				ncurselcol = 0;
				m_initcol = 0;
				ncurselrow++;

				if(ncurselrow >= (m_initrow + nrowperpage))
					m_initrow++;
                
                if(m_form == wyTrue)
                    m_initrow ++;
			}
            else
            {
                m_lpgvwndproc(m_hwnd, GVN_NEXTTABORDERITEM, 0, 0);
            }
		}	
		else
		{
			ncurselcol++;

			if(!GetOwnerData())
			{			
				pgvcolnode = GetColNodeStruct(ncurselcol);

				while(pgvcolnode && pgvcolnode->isshow == wyFalse)
				{
					pgvcolnode = GetColNodeStruct(++ncurselcol);
				}
			}
		}
	}

	// now we have to see if the tab is pressed and the column moves
	// beyond the windows then we increase the initcol.
	VERIFY(GetClientRect(m_hwnd, &rectwin));

	if(m_curselrow != ncurselrow)
	{
        if(m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, ncurselrow, ncurselcol) && 
            m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol))
        {
			m_curselrow = ncurselrow;
			m_curselcol = ncurselcol;
			InvalidateRect(m_hwnd, NULL, FALSE);
            ret = m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, m_curselcol);
        }
    }
    else
    {
        m_curselcol = ncurselcol;
	}

	// we have to ensure that the selected column is always visible in the window.
	EnsureVisible(m_curselrow, m_curselcol);

	return 1;
}

wyBool
CCustGrid::IsEditing()
{
    return m_isediting;
}

// handles when esc is pressed
LRESULT
CCustGrid::ProcessEscPress()
{
	wyInt32			islist = 0, islistdrop = 0, isbrowsebutton = 0;
    GVDISPINFO		disp = {0};
	wyString		moldcoltextstr;	
	
	moldcoltextstr.SetAs(m_oldcoltext);

	if(m_pgvcurcolnode)
		islist = m_pgvcurcolnode->pColumn.mask & GVIF_LIST;
	
	if(m_pgvcurcolnode)
		islistdrop = m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNLIST || m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNNLIST;

    if(m_pgvcurcolnode)
        isbrowsebutton = m_pgvcurcolnode->pColumn.mask & GVIF_BROWSEBUTTON;

	if(GetOwnerData())
    {
		disp.nRow = m_curselrow;
		disp.nCol = m_curselcol;

		// the previous value can be NULL so we gotta consider that.
		if(wcsicmp(m_oldcoltext, TEXT(STRING_NULL)))
        {
			disp.text = (LPSTR)moldcoltextstr.GetString();
			disp.cchTextMax = moldcoltextstr.GetLength();
		} 

		m_lpgvwndproc(m_hwnd, GVN_SETOWNERCOLDATA,(WPARAM)&disp,(LPARAM)1);
	} 
	
	else 
		SetSubItemText(m_curselrow, m_curselcol, moldcoltextstr.GetString());			
	
	m_isediting = wyFalse;
	
	VERIFY(ShowWindow(m_hwndedit, FALSE));
	
	if((m_pgvcurcolnode->pColumn.mask & GVIF_LIST) ||
        (m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNNLIST) ||
        (m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNLIST))
		ShowWindow(m_pgvcurcolnode->hwndCombo, FALSE);
	
    if(isbrowsebutton)
    {
        ShowWindow(m_hwndbrowsebutton, SW_HIDE);
    }
	
	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return 1;
}

// Methods copies a cell data to/from clipboard
wyBool
CCustGrid::CopyDataToClipboard()
{
	GVDISPINFO	disp={0};
	wyChar		*usrdata;
    PGVCOLUMN	pgvcol;
	wyString	pgvcoltext;
	wyWChar		*buffercb;
	wyUInt32	length = 1;
	
	if(m_curselrow == -1 || m_curselcol == -1 || (m_curselrow > (m_row-1)))
		return wyTrue;

	if(GetOwnerData())
    {
		disp.nRow = m_curselrow; 
        disp.nCol = m_curselcol; 
		disp.text = NULL; 
        disp.cchTextMax = 512 - 1;		
	
        if(m_lpgvwndproc(m_hwnd, GVN_GETDISPLENINFO,(WPARAM)&disp, 0) == FALSE)
            disp.cchTextMax = 512;	
	
        usrdata =(wyChar*)calloc(sizeof(wyChar), (disp.cchTextMax));
        disp.text = usrdata;

		//LPARAM sets for handling the base2 display format in grid
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFODATA,(WPARAM)&disp, 1);
	
		if(!stricmp(usrdata, STRING_NULL)|| !stricmp(usrdata, BINARY_DATA)|| !strlen(usrdata))
        {
            free(usrdata);
			return wyTrue;
        }
	} 
    else 
    {
		VERIFY(pgvcol = GetSubItemStruct(m_curselrow, m_curselcol));
	
		if(pgvcol->text)
	    {	
			pgvcoltext.SetAs(pgvcol->text);

			usrdata =(wyChar*)calloc(sizeof(wyChar), (pgvcoltext.GetLength()+1));
			strncpy(usrdata, pgvcoltext.GetString(), pgvcoltext.GetLength());
        }
        else
            return wyTrue;
	}

	// copy data to the buffer
	HGLOBAL		hglbcopy;
	LPWSTR		lpstrcopy;
	wyString	buffer;

	buffer.SetAs(usrdata);

	hglbcopy = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (strlen(usrdata)+ 1) * 2);
	if(!hglbcopy)
        return wyFalse;

	lpstrcopy = (wyWChar*)GlobalLock(hglbcopy);	
	buffercb = buffer.GetAsWideChar(&length);
	wcsncpy(lpstrcopy, buffercb, length);
    free(usrdata);

	VERIFY((GlobalUnlock(hglbcopy))== NO_ERROR);

	if(!(OpenClipboard(m_hwnd)))
		return wyFalse;
	
	VERIFY(EmptyClipboard());
	VERIFY(SetClipboardData(CF_UNICODETEXT, hglbcopy));
	VERIFY(CloseClipboard());
	return wyTrue;
}

wyBool
CCustGrid::CopyDataFromClipboard()
{
    GVDISPINFO	disp={0};
    HGLOBAL     hglb = NULL;
    LPWSTR      lptstr; 
	wyString	strlptstr;
	LRESULT		ret = 0;
	PGVCOLUMN	pgvcol;
	
	if(m_curselrow == -1 || m_curselcol == -1 ||(m_curselrow > (m_row-1)))
		return wyTrue;

    if(!IsClipboardFormatAvailable(CF_UNICODETEXT))
            return wyFalse; 

    if(!OpenClipboard(m_hwnd))
        return wyFalse; 

    hglb = GetClipboardData(CF_UNICODETEXT); 
    if(hglb != NULL)
    { 
        lptstr =(LPWSTR)GlobalLock(hglb); 
        if(lptstr != NULL)
        { 
            // Call the application-defined ReplaceSelection 
            // function to insert the text and repaint the 
            // window. 
            PGVCOLNODE fieldcolnode = GetColNodeStruct(m_curselcol);

			//fixed : http://forums.webyog.com/index.php?showtopic=7232
            if(!(fieldcolnode->pColumn.mask & GVIF_TEXT) || fieldcolnode->pColumn.uIsReadOnly)
            {
                CloseClipboard(); 
                return wyTrue;
            }

            if(m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, m_curselrow, m_curselcol) == 0)
            {
                CloseClipboard(); 
                return wyTrue;
            }

            if(m_lpgvwndproc(m_hwnd, GVN_PASTECLIPBOARDBEGIN, m_curselrow, m_curselcol) == 0)
            {
                CloseClipboard(); 
                return wyTrue;
            }
            
			if(GetOwnerData())
            m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(m_curselrow, m_curselcol), NULL);

			strlptstr.SetAs(lptstr);

            if(GetOwnerData())
            {
		        disp.nRow = m_curselrow; 
                disp.nCol = m_curselcol; 
		        disp.text = (wyChar*)strlptstr.GetString(); 
				disp.cchTextMax = strlptstr.GetLength();		
        	
		        m_lpgvwndproc(m_hwnd, GVN_SETOWNERCOLDATA,(WPARAM)&disp, 0);
	        } 
            else 
            {
				//8.04 make paste data to cell same as Owner draw
				ret = PostEndLabelEditMsg(m_curselrow, m_curselcol, strlptstr);
				if(!ret)	
				{
					GlobalUnlock(hglb); 
					return wyFalse;
				}

		        //Sets the data to cell
		        VERIFY(pgvcol = GetSubItemStruct(m_curselrow, m_curselcol));
				SetSubItemText(m_curselrow, m_curselcol, strlptstr.GetString());
            }

            m_lpgvwndproc(m_hwnd, GVN_PASTECLIPBOARD, m_curselrow, m_curselcol);

            GlobalUnlock(hglb); 
        } 
    } 
    CloseClipboard(); 

	return wyTrue;
}


LRESULT
CCustGrid::OnWMChar(WPARAM wparam, LPARAM lparam)
{
	PGVCOLNODE	pgvcolnode = GetColNodeStruct(m_curselcol);
	wyBool		ret;

	if(!pgvcolnode)
		goto invalidate;

	if((pgvcolnode->pColumn.mask & GVIF_BUTTON || 
        pgvcolnode->pColumn.mask & GVIF_TEXTBUTTON)&& 
		(wparam >= 33)&&(wparam <= 125))
    {
		//wyTrue is passed for chardown
		ProcessButtonClick(m_curselrow, m_curselcol, wyTrue);
		goto invalidate;
	}

	if((wparam >= 33)&&(wparam <= 125))
	{
		ret = BeginColumnEdit(m_curselrow, m_curselcol);
		if(!ret)
			return 0;

		PostMessage(m_hwndedit, WM_CHAR, wparam, lparam);
	}

invalidate:
	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));
	return 1;
}

LRESULT
CCustGrid::OnNCDestroy(WPARAM wparam, LPARAM lparam)
{
    
	PGVCOLNODE	pgvcolnode, pgvcolnodetemp;

    //DeleteAll();

	pgvcolnode = m_collist;

	while(pgvcolnode)
	{
		pgvcolnodetemp = pgvcolnode->pNext;

		if(pgvcolnode->pColumn.text)
			delete[] pgvcolnode->pColumn.text;

		if(pgvcolnode->pColumn.pszButtonText)
			delete[] pgvcolnode->pColumn.pszButtonText;

        if(pgvcolnode->pColumn.pszDefault)
			delete[] pgvcolnode->pColumn.pszDefault;

		if(pgvcolnode->pszDefault)
			delete[] pgvcolnode->pszDefault;

		delete pgvcolnode;

		pgvcolnode = pgvcolnodetemp;
	}

	m_collist = NULL;

    DeleteAll();

	return 1;
}

wyInt32
CCustGrid::Insert_Column(PGVCOLUMN pgvcol)
{
	if(m_collist == NULL)
        return InsertAsFirstColumn(pgvcol);
	else
        return InsertAsLastColumn(pgvcol);
}

wyBool 
CCustGrid::SetColumnMask(wyInt32 col, wyInt32 mask)
{
    wyInt32 i;
    PGVCOLNODE* temp = &m_collist;

	for(i = 0; i < col && (*temp) != NULL; ++i)
    {
		temp = &((*temp)->pNext);
    }

    if(!*temp)
    {
        return wyFalse;
    }

    (*temp)->pColumn.mask = mask;
    return wyTrue;
}

wyInt32 
CCustGrid::GetColumnMask(wyInt32 col)
{
    wyInt32 i;
    PGVCOLNODE* temp = &m_collist;

	for(i = 0; i < col && (*temp) != NULL; ++i)
    {
		temp = &((*temp)->pNext);
    }

    if(!*temp)
    {
        return 0;
    }

    return (*temp)->pColumn.mask;
}

void 
CCustGrid::FillList(PGVCOLUMN pgvcol, PGVCOLNODE temp)
{
	if((pgvcol->mask & GVIF_LIST)||(pgvcol->mask & GVIF_DROPDOWNLIST))
		FillComboBox(pgvcol, temp, wyFalse);
    else if(pgvcol->mask & GVIF_DROPDOWNNLIST)
        FillComboBox(pgvcol, temp, wyTrue);
	else if(pgvcol->mask & GVIF_DROPDOWNMULTIPLE)
		FillMultipleList(pgvcol, temp);
}

void 
CCustGrid::CopyValues(PGVCOLUMN pgvcol, PGVCOLNODE	temp)
{
	// now copy the default
	if(pgvcol->pszDefault)
    {
		temp->pszDefault = new wyChar[strlen(pgvcol->pszDefault)+ 1];
		strcpy(temp->pszDefault, pgvcol->pszDefault);
	}

    temp->pColumn.pszButtonText = new wyChar[2];
	temp->pColumn.pszButtonText[0] = 0;
	temp->pColumn.mask = pgvcol->mask;
	temp->pColumn.cchTextMax = pgvcol->cchTextMax;
	temp->pColumn.cx = pgvcol->cx;
	temp->pColumn.fmt = pgvcol->fmt;
    temp->pColumn.uIsReadOnly = pgvcol->uIsReadOnly;
    temp->pColumn.mark = pgvcol->mark;
    temp->pColumn.marktype = pgvcol->marktype;

    return;
}

LONG 
CCustGrid::InsertAsFirstColumn(PGVCOLUMN pgvcol)
{
	PGVCOLNODE	temp = new GVCOLNODE;

	if(!temp)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return -1;
	}

	memset(temp, 0, sizeof(GVCOLNODE));
    FillList(pgvcol, temp);

	temp->pColumn.text = new wyChar[pgvcol->cchTextMax+1];
	strcpy(temp->pColumn.text, pgvcol->text);
    CopyValues(pgvcol, temp);

	temp->isshow = wyTrue;

	m_collist = temp;
	RowRecalculate(temp);
	return m_col++;
}

LONG  
CCustGrid::InsertAsLastColumn(PGVCOLUMN pgvcol)
{
	PGVCOLNODE temp = m_collist;

	while(temp->pNext != NULL)
		temp = temp->pNext;

	temp->pNext = new GVCOLNODE;

	if(!temp->pNext)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return -1;
	}

	memset(temp->pNext, 0, sizeof(GVCOLNODE));
	temp->pNext->pNext = NULL;
	temp = temp->pNext;
	temp->pColumn.text = new wyChar[pgvcol->cchTextMax+1];
	memset(temp->pColumn.text, 0, pgvcol->cchTextMax+1);

    FillList(pgvcol, temp);
	strcpy(temp->pColumn.text, pgvcol->text);
    CopyValues(pgvcol, temp);

	temp->isshow = wyTrue;

	RowRecalculate(temp);
	return m_col++;
}

wyBool
CCustGrid::FillComboBox(PGVCOLUMN psrc, PGVCOLNODE ptarget, wyBool pnumeric)
{
	wyInt32 listcount, ret;
	DWORD	listboxflags = WS_CHILD | LBS_NOINTEGRALHEIGHT | LBS_DISABLENOSCROLL | 
                                                                        WS_VSCROLL | LBS_NOTIFY | WS_BORDER;

    if(pnumeric == wyFalse)
        listboxflags |= LBS_SORT;

	VERIFY(ptarget->hwndCombo = CreateWindowEx(WS_EX_WINDOWEDGE, L"listbox",  NULL, listboxflags,  
                            0, 0, 0, 0, m_hwnd,(HMENU)GV_GRIDPICKLIST,  GetModuleHandle(0), NULL));

	// now insert them in the combo box.
	for(listcount = 0; listcount < psrc->nListCount; listcount++)
		VERIFY(ret = SendMessage(ptarget->hwndCombo, LB_ADDSTRING,(WPARAM)0, 
                    (LPARAM)(wyChar*)psrc->pszList +(listcount * psrc->nElemSize)) != -1);

	SendMessage(ptarget->hwndCombo, WM_SETFONT, (WPARAM)m_hfont, TRUE);

    // Now subclass the edit box.
	m_wporiglistwndproc = (WNDPROC)SetWindowLongPtr(ptarget->hwndCombo, GWLP_WNDPROC,(LONG_PTR)CCustGrid::ListWndProc);
	SetWindowLongPtr(ptarget->hwndCombo, GWLP_USERDATA, (LONG_PTR)this);

	return wyTrue;
}

wyBool
CCustGrid::FillMultipleList(PGVCOLUMN psrc, PGVCOLNODE ptarget)
{
	wyInt32		ret;
    DWORD	    style = WS_TABSTOP | WS_CHILD | LVS_REPORT | LVS_NOCOLUMNHEADER | WS_BORDER | LVS_SINGLESEL | LVS_SHOWSELALWAYS; 
    LVCOLUMN    lvc = {0};

    ptarget->hwndCombo = NULL;

	VERIFY(ptarget->hwndCombo = CreateWindowEx(WS_EX_WINDOWEDGE, WC_LISTVIEW, NULL, style,
                            0,0,100,100, m_hwnd, (HMENU)GV_GRIDPICKLIST, GetModuleHandle(0), NULL));

	ListView_SetExtendedListViewStyle(ptarget->hwndCombo, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	
	SendMessage ( ptarget->hwndCombo, WM_SETFONT, (WPARAM)m_hfont, TRUE);

	lvc.mask	= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvc.cx		= 0;
	lvc.fmt		= LVCFMT_LEFT;
	lvc.pszText = L"xx";
	VERIFY ( (ret = ListView_InsertColumn(ptarget->hwndCombo, 0, &lvc)) != -1 );

	(ptarget->wpOrigComboProc = (WNDPROC)SetWindowLongPtr(ptarget->hwndCombo, GWLP_WNDPROC, (LONG_PTR)CCustGrid::ComboWndProc));
	 SetWindowLongPtr(ptarget->hwndCombo, GWLP_USERDATA, (LONG_PTR)ptarget->wpOrigComboProc);
	return wyTrue;
  
}

LONG	
CCustGrid::Insert_Row()
{
	PGVROWNODE	rownode;

    rownode		= m_rowlist;
	
	if(rownode == NULL)
        return InsertAsFirstRow();
	else	
        return InsertAsLastRow();
}

void 
CCustGrid::SetRowValues(PGVCOLNODE colnode, PGVCOLNODE *temp)
{
	while(colnode != NULL)
	{
		*temp = new GVCOLNODE;
		memset(*temp, 0, sizeof(GVCOLNODE));
				
		// now see if the type is of bool or not. if it is then we have to handle
		// it differently.
		if(colnode->pColumn.mask & GVIF_BOOL)
        {
			(*temp)->pColumn.text = new wyChar[6];
			strcpy((*temp)->pColumn.text, "false");
			(*temp)->pColumn.issource = 'N';
            st++;
		} 
        else if(colnode->pszDefault)
        {
			if(m_exstyle & GV_EX_OWNERDATA)
            {
				(*temp)->pColumn.text = NULL;
				(*temp)->pColumn.issource = 'Y';
			} 
            else 
            {
				(*temp)->pColumn.text = new wyChar[strlen(colnode->pszDefault)+ 1];
				strcpy((*temp)->pColumn.text, colnode->pszDefault);
				(*temp)->pColumn.issource = 'N';
			}
		} 
        else 
        {
			if(m_exstyle & GV_EX_OWNERDATA)
            {
				(*temp)->pColumn.text = NULL;
				(*temp)->pColumn.issource = 'Y';
			} 
            else 
            {
				(*temp)->pColumn.text = new wyChar[2];
				(*temp)->pColumn.text[0] = 0;
				(*temp)->pColumn.issource = 'N';

				(*temp)->isshow = wyTrue;
			}
		}

		(*temp)->pColumn.cx = colnode->pColumn.cx;
		(*temp)->pColumn.fmt = colnode->pColumn.fmt;
        (*temp)->pColumn.uIsReadOnly = colnode->pColumn.uIsReadOnly;

		/* allocate buffer for button text */
		(*temp)->pColumn.pszButtonText = new wyChar[2];
		(*temp)->pColumn.pszButtonText[0] = 0;

		(*temp)->pNext = NULL;
		temp = &(*temp)->pNext;

		colnode = colnode->pNext;
	}
    return;
}

LONG
CCustGrid::InsertAsFirstRow()
{
    PGVCOLNODE	colnode, *temp;
	PGVROWNODE  rownode = new GVROWNODE;

	rownode->lparam = NULL;
	rownode->excheck = wyFalse;
	rownode->pNext = NULL;
    rownode->rowcx = m_maxwidth;

	colnode = m_collist;
	
	temp = &rownode->pColumn;
	*temp = NULL;

    SetRowValues(colnode, temp);

    m_rowlist = rownode;
	m_rowlast	= rownode;
    m_initrow = 0;
	return m_row++;
}

LONG 
CCustGrid::InsertAsLastRow()
{
    PGVCOLNODE	colnode, *temp;
	PGVROWNODE  rownode = m_rowlast;
	
	rownode->pNext = new GVROWNODE;
	rownode->pNext->lparam = NULL;
	rownode->pNext->excheck = wyFalse;
	rownode->pNext->pNext = NULL;
    rownode->pNext->rowcx = m_maxwidth;

	colnode = m_collist;

	temp = &rownode->pNext->pColumn;
	*temp = NULL;
	
    SetRowValues(colnode, temp);
	
	m_rowlast = rownode->pNext;
	return m_row++;
}

wyBool
CCustGrid::InsertNewRowOnKeyPress(HWND hwnd)
{
	wyInt32 ret;
	
    if(!m_lpgvwndproc(hwnd, GVN_ROWCHANGINGTO, m_curselrow + 1, m_curselcol))
    {
        return wyFalse;
    }

	ret = m_lpgvwndproc(hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);

	if(!ret)
		return wyFalse;
	
	m_lpgvwndproc(hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);

	ret = m_lpgvwndproc(hwnd, GVN_BEGINADDNEWROW, 0, 0);

	if(!ret)
		return wyFalse;

	// now we need to check whether its owner drawn or not and behave accordingly
	if(!GetOwnerData())
    {
		if(IsRowEmpty(m_curselrow))
			return wyFalse;

		ret = Insert_Row();
		m_lpgvwndproc(hwnd, GVN_ENDADDNEWROW, ret, 0);
		m_curselrow++;
		m_curselcol = 0;

		// now we see do we need to increase the init row value.
		if(RowPerPage() < m_row)
			EnsureVisible(m_curselrow, m_curselcol);

		VERIFY(InvalidateRect(hwnd, NULL, TRUE));
	}
    else
		return wyTrue;

	return wyTrue;
}

wyBool
CCustGrid::DeleteAllRow(wyBool ispaint)
{
    DeleteAll();

	m_row = 0;
	m_initcol = 0;
	m_initrow = 0;
	m_curselrow = -1;
	m_curselcol = -1;
	m_rowlist = NULL;
	m_pgvcurcolnode = NULL;
	m_isediting	= wyFalse;
	m_iscomboediting = wyFalse;

	if(ispaint == wyTrue)
		VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return wyTrue;
}

wyBool
CCustGrid::DeleteAllColumns()
{
	PGVCOLNODE	pgvcolnode, pgvcolnodetemp;

	pgvcolnode = m_collist;

	while(pgvcolnode)
	{
		pgvcolnodetemp = pgvcolnode->pNext;

		if(pgvcolnode->pColumn.text)
        {
			delete[] pgvcolnode->pColumn.text;
            pgvcolnode->pColumn.text = NULL;
        }

		/* bug fix in v5.02. Non-release of this pointer resulted in buildup of memory */
		if(pgvcolnode->pColumn.pszButtonText)
        {
			delete[] pgvcolnode->pColumn.pszButtonText;
            pgvcolnode->pColumn.pszButtonText = NULL;
        }

        if(pgvcolnode->pColumn.pszDefault)
        {
			delete[] pgvcolnode->pColumn.pszDefault;
            pgvcolnode->pColumn.pszDefault = NULL;
        }

        if(pgvcolnode->pszDefault)
        {
			delete[] pgvcolnode->pszDefault;
            pgvcolnode->pszDefault = NULL;
        }

		if(pgvcolnode->hwndCombo)
        {
			VERIFY(DestroyWindow(pgvcolnode->hwndCombo));
            pgvcolnode->hwndCombo = NULL;
        }

		delete pgvcolnode;

		pgvcolnode = pgvcolnodetemp;
	}

	m_col = 0;
	m_collist = NULL;
    m_initcol = 0;
    m_curselcol = 0;
	return wyTrue;
}

wyBool
CCustGrid::DeleteAll()
{
	PGVCOLUMN	pgvcol;
	PGVROWNODE	pgvrow, temp=NULL;
    PGVCOLNODE	pgvcolnode, pgvcolnodetemp;

	pgvrow =	m_rowlist;

	if(!pgvrow)
		return wyFalse;

	if(!m_row)
		return wyFalse;
		

        while(pgvrow)
        {
            pgvcolnode = pgvrow->pColumn;
    		
		    while(pgvcolnode)
		    {
			    pgvcol = &pgvcolnode->pColumn;

                if(pgvcol)
                {

			        if(pgvcol->issource == 'N' && pgvcol->text)
                    {
				        delete[] pgvcol->text;
                        pgvcol->text = NULL;
                    }

			        if(pgvcol->pszButtonText)
                    {
				        delete[] pgvcol->pszButtonText;
                        pgvcol->pszButtonText = NULL;
                    }

		            if(pgvcol->pszDefault)
                    {
			            delete[] pgvcol->pszDefault;
                        pgvcol->pszDefault = NULL;
                    }
                }

                if(pgvcolnode->pszDefault)
                {
                    delete[] pgvcolnode->pszDefault;
                    pgvcolnode->pszDefault = NULL;
                }

			    pgvcolnodetemp = pgvcolnode;
			    pgvcolnode = pgvcolnode->pNext;

			    delete pgvcolnodetemp;

	        }	
            temp = pgvrow;
            pgvrow = pgvrow->pNext;
            delete temp;
        }
    
	// now if its ownerdata then we have to release this
	if(GetOwnerData())
    {
		///* if m_rowcheck is not initialized we dont haev to do anything */
		if(!m_rowcheck)
			return wyTrue;

        delete[] m_rowcheck;

        m_rowcheck = NULL;
    }

    m_initrow = 0;
    m_initcol = 0;

    return wyTrue;
}

void 
CCustGrid::DeleteAllColumnNode(PGVCOLNODE pgvcolnode)
{
    PGVCOLNODE  pgvcolnodetemp;
    PGVCOLUMN	pgvcol;

	while(pgvcolnode)
	{
		pgvcol = &pgvcolnode->pColumn;

		if(pgvcol->issource == 'N' && pgvcol->text)
        {
			delete[] pgvcol->text;
            pgvcol->text = NULL;
        }	

		if(pgvcol->pszButtonText)
        {
			delete[] pgvcol->pszButtonText;
            pgvcol->pszButtonText = NULL;
        }

		if(pgvcolnode->hwndCombo)
        {
			VERIFY(DestroyWindow(pgvcolnode->hwndCombo));
            pgvcolnode->hwndCombo = NULL;
        }

		pgvcolnodetemp = pgvcolnode;
		pgvcolnode = pgvcolnode->pNext;

		delete pgvcolnodetemp;
	}
    return;
}

wyBool
CCustGrid::DeleteRow(wyInt32 row)
{
	wyInt32     rowcount;
	PGVROWNODE	pgvrow, temp=NULL;
	PGVCOLNODE	pgvcolnode;
    wyChar      *temprowcheck;

	pgvrow =	m_rowlist;

	if(!pgvrow)
		return wyFalse;

	// now if its ownerdata then we have to behave differently
	if(GetOwnerData())
    {
		if(!m_row)
			return wyFalse;

		/* if m_rowcheck is not initialized we dont haev to do anything */
		if(!m_rowcheck)
			return wyTrue;

		/*	we keep the pointr of the next row because if the row to be deleted is 0, then
			after deletion of the first row, we need to move the list head with the correct pointer */
		if(row == 0)
			temp = pgvrow->pNext;
		else 
        {
			for(rowcount = 0; rowcount < row; rowcount++)
			{
				temp = pgvrow;
				pgvrow = pgvrow->pNext;
				if(!pgvrow)
					return wyFalse;
			}
		}

		// we need to realloc and copy m_rowcheck
		temprowcheck = new wyChar[m_row];
		// copy data from previous buffer.
		memcpy(temprowcheck, m_rowcheck, sizeof(wyChar) * (row));
		// leave the current row and copy the leftover
		memcpy(temprowcheck + row, m_rowcheck + row + 1, sizeof(wyChar) * (m_row - row));

		delete[] m_rowcheck;
		m_rowcheck = temprowcheck;
		pgvcolnode = pgvrow->pColumn;
		
        DeleteAllColumnNode(pgvcolnode);

		if(m_curselrow == m_row-1)
			m_curselrow--;

		if(row == 0)
			m_rowlist = temp;
		else 
        {
			temp->pNext = pgvrow->pNext;

			if(row == m_row)
				m_rowlast = temp;
		}

		delete pgvrow;
		
		if(m_row)
			m_row--;

		return wyTrue;
	}	

	if(!pgvrow)
		return wyFalse;
	
    // send a message to the parent window that a row is being deleted.
	// so he can release any stuff attached to the row.
	m_lpgvwndproc(m_hwnd, GVN_DELETEROW, row, 0);

	if(row == 0)
	{
		temp = pgvrow->pNext;
		pgvcolnode = pgvrow->pColumn;
        
        DeleteAllColumnNode(pgvcolnode);
		
        if(pgvrow->excheck)
            m_checkcount--;

		delete pgvrow;
		m_rowlist = temp;
		m_row--;
        m_curselrow--;
		
		if(m_curselrow < 0)
		{
			m_curselrow = 0;
			m_curselcol = 0;
		}

		return wyTrue;
	}

	pgvrow = m_rowlist;

	for(rowcount = 0; rowcount < row; rowcount++)
	{
		temp = pgvrow;
		pgvrow = pgvrow->pNext;

		if(!pgvrow)
			return wyFalse;
	}

	if(!temp)
		return wyFalse;

	temp->pNext = pgvrow->pNext;
	pgvcolnode  = pgvrow->pColumn;

    DeleteAllColumnNode(pgvcolnode);

    if(pgvrow->excheck)
       m_checkcount--;
    delete pgvrow;
	m_row--;
    if(row <= m_curselrow)
	    m_curselrow--;

	if(row == m_row)
		m_rowlast = temp;

	if(row == m_initrow)
        m_initrow--;

    // check if its in the top then we change it.
    /*
    if(m_curselrow < m_initrow)
		m_curselrow = m_initrow;
    
	/*	now it might happen that the resulting number of rows in grid 
		is more less the initrow so it will fail */
	/*
    m_initrow = 0;
    */

	return wyTrue;
}

wyBool
CCustGrid::SetSubItemText(wyInt32 nrow, wyInt32 ncol, const wyChar *text)
{
	if(GetOwnerData())
        return SetSubItemOwnerText(nrow, ncol, text);
    else
        return SetSubItemNonOwnerText(nrow, ncol, text);
}

wyBool 
CCustGrid::SetSubItemNonOwnerText(wyInt32 nrow, wyInt32 ncol, const wyChar *text)
{
    wyInt32     length;
	PGVCOLUMN   pgvcol;
	wyString	textstr;
	
	textstr.SetAs(text);

	pgvcol = GetSubItemStruct(nrow, ncol);
    if(pgvcol == NULL)
        return wyTrue;

	if(pgvcol->text && pgvcol->issource != 'Y')
		delete[] pgvcol->text;

	length = strlen(textstr.GetString());
	pgvcol->text = new wyChar[length+2];
	strcpy(pgvcol->text, textstr.GetString());
	pgvcol->issource = 'N';

	return wyTrue;
}

wyBool 
CCustGrid::SetSubItemOwnerText(wyInt32 nrow, wyInt32 ncol, const wyChar *text)
{
    GVDISPINFO  disp = {0};
	wyString	textstr;
	
	if(!text)
		return wyTrue;

	textstr.SetAs(text);

	disp.nRow = nrow; 
    disp.nCol = ncol;
	disp.text = (LPSTR)textstr.GetString();

	if(text)
		disp.cchTextMax = textstr.GetLength();

	m_lpgvwndproc(m_hwnd, GVN_SETOWNERCOLDATA, (WPARAM)&disp, 0);

	return wyTrue;
}

wyBool
CCustGrid::SetSubButtonItemText(wyInt32 nrow, wyInt32 ncol, wyWChar *text)
{
	wyInt32	    length;
	PGVCOLUMN   pgvcol;
	wyString	textstr;
	
	textstr.SetAs(text);

	if(GetOwnerData())
		return wyTrue;
	
	pgvcol = GetSubItemStruct(nrow, ncol);

	if(pgvcol->pszButtonText)
    {
		delete[] pgvcol->pszButtonText;
        pgvcol->pszButtonText = NULL;
    }

	length = wcslen(text);
	pgvcol->pszButtonText = new wyChar[length+2];
	strcpy(pgvcol->pszButtonText, textstr.GetString());

	return wyTrue;
}

wyBool
CCustGrid::SetTextPointer(wyInt32 nrow, wyInt32 ncol, wyChar *text)
{
	PGVCOLUMN	pgvcol;

	if(!(m_exstyle & GV_EX_OWNERDATA))
		return wyTrue;
	
	VERIFY(pgvcol = GetSubItemStruct(nrow, ncol));

	if(pgvcol->text)
    {
		delete[] pgvcol->text;
        pgvcol->text = NULL;
    }

	pgvcol->text = text;
	pgvcol->issource = 'Y';

	return wyTrue;
}

wyInt32
CCustGrid::GetItemTextLength(wyInt32 nrow, wyInt32 ncol)
{
	PGVCOLUMN	pgvcol;

	pgvcol = GetSubItemStruct(nrow, ncol);

    if(!pgvcol)
        return 0;

	if(!pgvcol->text && pgvcol->issource == 'Y')
		return 6; //if it is owner data pgvcol->text will be (NULL)
	else if(!pgvcol->text)
		return 0;

	return strlen(pgvcol->text);

}

// Inserts a new item in the list.
wyInt32
CCustGrid::InsertTextInList(wyInt32 col, wyWChar *buffer)
{
	wyInt32     index = 0;
	PGVCOLNODE	pgvcol;
	LVITEM		lvi = {0};

	VERIFY(pgvcol = GetColNodeStruct(col));

	// now we see which style has been given for the column.
	if((pgvcol->pColumn.mask & GVIF_DROPDOWNLIST)||(pgvcol->pColumn.mask & GVIF_LIST)||(pgvcol->pColumn.mask & GVIF_DROPDOWNNLIST))
		VERIFY((index = SendMessage(pgvcol->hwndCombo, LB_ADDSTRING, 0,(LPARAM)buffer)) != LB_ERR);
	else
    {
		lvi.mask = LVIF_TEXT;
		//lvi.iItem = 500;
		//lvi.iSubItem = 0;
		lvi.cchTextMax = wcslen(buffer);
		lvi.pszText = buffer;
        lvi.iItem = ListView_GetItemCount(pgvcol->hwndCombo);
		
		VERIFY((index = ListView_InsertItem(pgvcol->hwndCombo, &lvi))!= -1);
	}

	return index;
}

// Inserts a new item in the list.
wyInt32
CCustGrid::DeleteListContent(wyInt32 col)
{
	wyInt32     index = 0;
	PGVCOLNODE	pgvcol;

	VERIFY(pgvcol = GetColNodeStruct(col));

	// now we see which style has been given for the column.
	if((pgvcol->pColumn.mask & GVIF_DROPDOWNLIST)||(pgvcol->pColumn.mask & GVIF_LIST)||(pgvcol->pColumn.mask & GVIF_DROPDOWNNLIST))
		VERIFY((index = SendMessage(pgvcol->hwndCombo, LB_RESETCONTENT, 0,(LPARAM)0)) != LB_ERR);
	else
		VERIFY(SendMessage(pgvcol->hwndCombo, LVM_DELETEALLITEMS, 0,(LPARAM)0));

	return index;
}


// finds a text in list.
wyInt32
CCustGrid::FindTextInList(wyInt32 col, wyWChar *buffer)
{
	wyInt32     index;
	PGVCOLNODE	pgvcol;

	VERIFY(pgvcol = GetColNodeStruct(col));

	index = SendMessage(pgvcol->hwndCombo, LB_FINDSTRINGEXACT, 0,(LPARAM)buffer);

	return index;
}

// finds the first instance of text in a specified column.
wyInt32
CCustGrid::FindTextInColumn(wyInt32 col, wyInt32 itempreced, wyChar *text)
{
	wyInt32     startindex =(itempreced == -1)?(0):(itempreced);
	PGVCOLUMN   pgvcol = NULL;

	for(; startindex <= m_row ; startindex++)
    {
		pgvcol = GetSubItemStruct(startindex, col);

		if(!pgvcol)
			break;

		if(strcmp(pgvcol->text, text) == 0)
			return startindex;
	}

	return GV_ERR;
}


wyInt32
CCustGrid::GetItemText(wyInt32 row, wyInt32 col, wyWChar	*text)
{
	PGVCOLUMN	pgvcol;
	wyString	pgvcoltext;

	pgvcol = GetSubItemStruct(row, col);
	
	if(!pgvcol)
		return -1;
	
	pgvcoltext.SetAs(pgvcol->text);

	if(!pgvcol->text && pgvcol->issource == 'Y')
		wcscpy(text, L"(NULL)");
	else
		wcscpy(text, pgvcoltext.GetAsWideChar());

	return wcslen(text);
}

wyBool
CCustGrid::GetItemFromPoint(LPPOINT lppnt, wyInt32 *prow, wyInt32 *pcol)
{
	RECT    recttemp;
	wyInt32 x, y;
	LONG    rowcount, colcount;

	y = m_hight;
	x = GV_DEFWIDTH;

	*prow = -1;
	*pcol = -1;

	// we first check if the point is in the columns .
	if(lppnt->y <= m_hight)
		return wyFalse;

	// now we check on what column the user has clicked.
	for(rowcount = m_initrow; rowcount <= m_row; rowcount++)
	{
        memset(&recttemp, 0, sizeof(recttemp));

		for(colcount = m_initcol; colcount < m_col; colcount++)
		{
			GetSubItemRect(rowcount, colcount, &recttemp);
			
			// now if the user has clicked on the side panel then we have to dso some modification.
			if(lppnt->x <= GV_DEFWIDTH)
			{
				recttemp.left = 0;
				recttemp.right = GV_DEFWIDTH;

				if(PtInRect(&recttemp, *lppnt))
				{
					*prow = rowcount;
					*pcol = -1;
					return wyTrue;
				}
			}

			if(PtInRect(&recttemp, *lppnt))
			{
				*prow = rowcount;
				*pcol = colcount;
				return wyTrue;
			}
		}

        if(m_flip == wyFalse && lppnt->x > recttemp.right)
        {
            return wyFalse;
        }
	}
	
	return wyFalse;
}

PGVCOLNODE 
CCustGrid::AddNewColNode(PGVCOLNODE pgvc)
{
	// no column has been added till yet.
	PGVCOLNODE newcolnode = new GVCOLNODE;
	memset(newcolnode, 0, sizeof(GVCOLNODE));
	newcolnode->pNext = NULL;
	newcolnode->pColumn.cchTextMax = 2;
	newcolnode->pColumn.text = new wyChar[2];
	newcolnode->pColumn.text[0] = 0;
	newcolnode->pColumn.pszButtonText = new wyChar[2];
	newcolnode->pColumn.pszButtonText[0] = 0;
	newcolnode->pColumn.cx = pgvc->pColumn.cx;
	newcolnode->pColumn.fmt = pgvc->pColumn.fmt;
    newcolnode->pColumn.uIsReadOnly = pgvc->pColumn.uIsReadOnly;
	newcolnode->pColumn.mask = pgvc->pColumn.mask;
	newcolnode->isshow = wyTrue;

    return newcolnode;
}

// Function recalculates the columns of each column when a column is inserted in between.
wyBool
CCustGrid::RowRecalculate(PGVCOLNODE pgvc)
{
    PGVCOLNODE	colnode, newcolnode;
    PGVROWNODE	rownode;

	if(m_row == 0)
		return wyTrue;

	rownode = m_rowlist;
	
	while(rownode != NULL)
	{
		colnode = rownode->pColumn;

		// check whether if any column has been added or not.
		if(colnode == NULL)
		{
            newcolnode = AddNewColNode(pgvc);
			colnode = newcolnode;
			return wyTrue;
		}
		
		while(colnode->pNext != NULL)
			colnode = colnode->pNext;

		newcolnode = AddNewColNode(pgvc);
        colnode->pNext = newcolnode;
		rownode = rownode->pNext;
	}
	return wyTrue;
}

wyBool
CCustGrid::BeginColumnEdit(wyInt32 nrow, wyInt32 ncol)
{
	PGVCOLUMN	pgvcol;
	PGVCOLNODE	pgvcolnode;

	if(!m_lpgvwndproc)
		return wyTrue;

    /* addition in v5.1. no rows no editing */
	if(m_curselrow == -1 || m_curselcol == -1 || !m_row)
		return wyFalse;

    /*
    LRESULT		ret;
    ret = m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, nrow, ncol);
    */

	if(!GetOwnerData())
    {
		pgvcol = GetSubItemStruct(nrow, ncol);
		if(!pgvcol || pgvcol->uIsReadOnly)
			return wyFalse;
	}

	pgvcolnode	  = GetColNodeStruct(ncol);
    if(!pgvcolnode)
        return wyFalse;

	m_pgvcurcolnode = pgvcolnode;

	if(((pgvcolnode->pColumn.mask & GVIF_LIST)||(pgvcolnode->pColumn.mask & GVIF_DROPDOWNNLIST)||(pgvcolnode->pColumn.mask & GVIF_DROPDOWNLIST)) &&
        m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, nrow, ncol) == wyTrue)
    {
		BeginListEdit(nrow, ncol);
    }
	else if(pgvcolnode->pColumn.mask & GVIF_TEXT && m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, nrow, ncol) == wyTrue)
    {
		BeginLabelEdit(nrow, ncol);
    }
	else if(pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE && m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, nrow, ncol) == wyTrue)
    {
		BeginMultipleLableEdit(nrow, ncol);
    }
	else if(pgvcolnode->pColumn.mask & GVIF_TEXTBUTTON ||pgvcolnode->pColumn.mask & GVIF_BUTTON)//(from 6.2) grid button open with space- issue  reported here http://code.google.com/p/sqlyog/issues/detail?id=149
	{
		//Onchardown is true
		ProcessButtonClick(nrow, ncol, wyTrue);
	} 
	else if(pgvcolnode->pColumn.mask & GVIF_BOOL)
		ToggleBoolValue(nrow, ncol);
 
	return wyTrue;
}

wyBool
CCustGrid::ToggleBoolValue(wyInt32 nrow, wyInt32 ncol)
{
	wyInt32		ret;
	wyBool		state;
	PGVCOLUMN	pgvcol;

	pgvcol = GetSubItemStruct(nrow, ncol);

	if(!pgvcol || pgvcol->uIsReadOnly)
		return wyFalse;

	ret = m_lpgvwndproc(m_hwnd, GVN_BEGINLABELEDIT, nrow, ncol);
	if(!ret)
		return wyFalse;

	m_pgvcurcolnode = GetColNodeStruct(ncol);

	if(ColTrueOrFalse(nrow, ncol))	
    {
		SetSubItemText(nrow, ncol, GV_FALSE);
		state = wyFalse;
	}
    else 
    {
		state = wyTrue;
		SetSubItemText(nrow, ncol, GV_TRUE);
	}

	ret = m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(nrow, ncol), 
                        (LPARAM)((state == wyTrue)?(GV_TRUE):(GV_FALSE)));

    ret = m_lpgvwndproc(m_hwnd, GVN_FINISHENDLABELEDIT, MAKELONG(nrow, ncol), 
                        (LPARAM)((state == wyTrue)?(GV_TRUE):(GV_FALSE)));

    //m_lpgvwndproc(m_hwnd, GVN_FINISHENDLABELEDIT, NULL, NULL); 

	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return wyTrue;
}

wyBool
CCustGrid::ProcessButtonClick(wyInt32 nrow, wyInt32 ncol, wyBool onchardown)
{
	LRESULT		ret=1;
	PGVCOLUMN	pgvcol=NULL;

	if(GetOwnerData() == wyFalse)
    {
		VERIFY(pgvcol = GetSubItemStruct(nrow, ncol));
		if(pgvcol->uIsButtonVis)
			ret = m_lpgvwndproc(m_hwnd, GVN_BUTTONCLICK, nrow, ncol);
	} 
    else 
	{
		//To uncheck NULL checkbox in Blob when it is opened through keyboard
		//m_lpgvwndproc(m_hwnd, GVN_BUTTONCLICK, nrow, ncol);
		m_lpgvwndproc(m_hwnd, GVN_BUTTONCLICK, MAKELONG(nrow, ncol), (onchardown == wyTrue)?1:0);
	}

	return wyTrue;
}

wyBool
CCustGrid::BeginLabelEdit(wyInt32 row, wyInt32 col)
{
	wyInt32		ret;
	RECT		rect;
	PGVCOLUMN	pgvcol=0;
	wyString	pgvcoltextstr;
    HDC         hdc;
    HFONT       hfont;
    RECT        rectbutton = {0};

	GetSubItemRect(row, col, &rect);
	
	if(GetOwnerData() == wyFalse)
    {
        pgvcol = GetSubItemStruct(row, col);
		if(!pgvcol)
			return wyFalse;
	}

	// First we get the text of the row, col and sore it in a temporary variable so that if the user
	// presses ESC we can revert back to the original text.
	GetOriginalText(row, col);

    if(m_pgvcurcolnode->pColumn.mask & GVIF_BROWSEBUTTON)
    {
        hdc = GetDC(m_hwnd);
        hfont = SelectFont(hdc, m_hfont);
        DrawText(hdc, BROWSEBUTTONTEXT, -1, &rectbutton, DT_CALCRECT | DT_SINGLELINE);
        rectbutton.right += 4;
        rectbutton.right = max(rectbutton.right, 25);
        MoveWindow(m_hwndbrowsebutton, rect.right - rectbutton.right, rect.top + 1, rectbutton.right - 1, (rect.bottom - rect.top) - 2, TRUE);
        rect.right -= (rectbutton.right - 1);
        SelectFont(hdc, hfont);
        ReleaseDC(m_hwnd, hdc);
    }

	VERIFY(ret = MoveWindow(m_hwndedit, rect.left, rect.top + 1, (rect.right - rect.left)-1,
                                                        (rect.bottom - rect.top)-2, TRUE));

	VERIFY(ret = SetWindowPos(m_hwndedit, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));

	if(GetOwnerData() == wyFalse)
	{
		pgvcoltextstr.SetAs(pgvcol->text);
		SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)pgvcoltextstr.GetAsWideChar());
	}
	else
	{
		SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)m_oldcoltext);
	}

    SetWindowPos(m_hwndbrowsebutton, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(m_hwndedit, SW_SHOW);

    if(m_pgvcurcolnode->pColumn.mask & GVIF_BROWSEBUTTON)
    {
        ShowWindow(m_hwndbrowsebutton, SW_SHOW);
    }

	SendMessage(m_hwndedit, EM_SETSEL, 0, -1);

	SetFocus(m_hwndedit);

	m_isediting = wyTrue;
	m_iscomboediting = wyFalse;

	return wyTrue;
}

wyBool
CCustGrid::BeginListEdit(wyInt32 row, wyInt32 col)
{
	wyInt32     sel = 0;
	PGVCOLUMN   pgvcol;
    GVDISPINFO	disp={0};
	wyChar		usrdata[512]={0};
    wyString    textstr;

    m_lpgvwndproc(m_hwnd, GVN_NCSHOWLIST, (WPARAM)row, (LPARAM)col);

	/* move the list box correctly so that it is entirely visible */
	ProcessOnListFocus(row, col);

	if(GetOwnerData() == wyFalse)
    {
		pgvcol = GetSubItemStruct(row, col);

        if(pgvcol->text)
        {
            textstr.SetAs(pgvcol->text);
            sel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_FINDSTRING, -1, (LPARAM)textstr.GetAsWideChar());
        }
        else
            sel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_FINDSTRING, -1, (LPARAM)pgvcol->text);
	} 
    else
    {
		disp.nRow = row; 
        disp.nCol = col; 
		disp.text = usrdata; 
        disp.cchTextMax = 512;		
		
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO, (WPARAM)&disp, 0);

        if(usrdata)
        {
            textstr.SetAs(usrdata);
            sel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_FINDSTRING, -1, (LPARAM)textstr.GetAsWideChar());
        }
        else
		    sel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_FINDSTRING, -1, (LPARAM)usrdata);
	}
	
	BeginLabelEdit(row, col);
	SendMessage(m_pgvcurcolnode->hwndCombo, LB_SETCURSEL, sel, 0);

	return wyTrue;
}

wyBool
CCustGrid::BeginMultipleLableEdit(wyInt32 nrow, wyInt32 ncol)
{
	wyInt32     sel, ncount, listcount;
	PGVCOLUMN   pgvcol = NULL;
	RECT        rect = {0}, rectwin = {0};
	RECT        rectitem = {0};
	wyWChar     *temp = NULL;
	wyWChar		*tok;
	wyWChar      seps[] = L",";
	LVFINDINFO	lvif = {0};
	wyString	pgvcoltext;
    HDC         hdc;
    HFONT       hfont;
    RECT        rectbutton = {0};

	GetSubItemRect(nrow, ncol, &rectitem);
	
	if(GetOwnerData() == wyFalse)
		VERIFY(pgvcol = GetSubItemStruct(nrow, ncol));

    if(m_pgvcurcolnode->pColumn.mask & GVIF_BROWSEBUTTON)
    {
        hdc = GetDC(m_hwnd);
        hfont = SelectFont(hdc, m_hfont);
        DrawText(hdc, BROWSEBUTTONTEXT, -1, &rectbutton, DT_CALCRECT | DT_SINGLELINE);
        rectbutton.right += 4;
        rectbutton.right = max(rectbutton.right, 25);
        MoveWindow(m_hwndbrowsebutton, rectitem.right - rectbutton.right, rectitem.top + 1, rectbutton.right - 1, (rectitem.bottom - rectitem.top) - 2, TRUE);
        SelectFont(hdc, hfont);
        ReleaseDC(m_hwnd, hdc);
    }

    if(m_pgvcurcolnode->pColumn.mask & GVIF_BROWSEBUTTON)
    {
        ShowWindow(m_hwndbrowsebutton, SW_SHOW);
    }

	VERIFY(GetClientRect(m_hwnd, &rectwin));

	//if(rectitem.right > rectwin.right)
	//Added for making visible the partial visible column of right most grid
	m_lpgvwndproc(m_hwnd, GVN_NCSHOWLIST, (WPARAM)nrow, (LPARAM)ncol);

	// First we get the text of the row, col and store it in a temporary variable so that if the user
	// presses ESC we can revert back to the original text.
	GetOriginalText(nrow, ncol);

	// decide to show it whether on top or bottom.
	// decide whether to show it on top or bottom.
    rect.left			= rectitem.left;
    rect.right			= rectitem.right;
    if(rectitem.bottom + GV_LISTBOX > rectwin.bottom && (rectitem.top - rectwin.top > rectwin.bottom - rectitem.bottom))
    {
		// it will be on top
		rect.top			= rectitem.top - GV_LISTBOX;
		rect.bottom			= rectitem.top;
	}
    else
    {
		// it will be on bottom.
		rect.top			= rectitem.bottom;
        rect.bottom			= rectitem.bottom + GV_LISTBOX;
	}
	
	VERIFY(m_pgvcurcolnode = GetColNodeStruct(m_curselcol));
	VERIFY(MoveWindow(m_pgvcurcolnode->hwndCombo, rect.left-1, rect.top,(rect.right - rect.left) + 20,
                                                                    (rect.bottom - rect.top), TRUE));
	// change the column width also.
    VERIFY(ListView_SetColumnWidth(m_pgvcurcolnode->hwndCombo, 0,(rect.right - rect.left) - (GetSystemMetrics(SM_CXVSCROLL))-5));
	VERIFY(SetWindowPos(m_pgvcurcolnode->hwndCombo, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
	ncount = ListView_GetItemCount(m_pgvcurcolnode->hwndCombo);

	if(!ncount)			// if no items are then we just return.
		return wyTrue;
	
	for(listcount = 0; listcount < ncount; listcount++)
		ListView_SetCheckState(m_pgvcurcolnode->hwndCombo, listcount, FALSE);

	if(GetOwnerData() == wyTrue)
    {
		temp = (wyWChar*)calloc(sizeof(wyWChar), (wcslen(m_oldcoltext)+ 1) * sizeof(wyWChar));
		wcscpy(temp, m_oldcoltext);
	} 
    else 
    {
		if(pgvcol && pgvcol->text)
        {
			pgvcoltext.SetAs(pgvcol->text);
			temp =AllocateBuffWChar(wcslen(pgvcoltext.GetAsWideChar())+ 1);
			wcscpy(temp, pgvcoltext.GetAsWideChar());
		} 
        else 
        {

			temp =AllocateBuffWChar(8);
			wcscpy(temp, TEXT(STRING_NULL));
		}
	}
	
	tok = wcstok(temp, seps);

	while(tok)
	{
		memset(&lvif, 0, sizeof(lvif));

		lvif.flags	= LVFI_STRING;
		lvif.psz	= tok;

		sel = ListView_FindItem(m_pgvcurcolnode->hwndCombo, -1, &lvif);

        if(sel >= 0)
		    ListView_SetCheckState(m_pgvcurcolnode->hwndCombo, sel, TRUE);
		
		tok = wcstok(NULL, seps);
	}

	free(temp);
	m_isediting = wyFalse;
	m_iscomboediting = wyTrue;

    ListView_SetItemState(m_pgvcurcolnode->hwndCombo, 0, LVIS_SELECTED | LVIS_FOCUSED,  
                                                       LVIS_SELECTED | LVIS_FOCUSED);
	VERIFY(ListView_EnsureVisible(m_pgvcurcolnode->hwndCombo, 0, FALSE));

	ShowWindow(m_pgvcurcolnode->hwndCombo, TRUE);
	m_hwndcurcombo = m_pgvcurcolnode->hwndCombo;

	return wyTrue;
	
}

PGVCOLNODE
CCustGrid::GetColNodeStruct(wyInt32 index)
{
	wyInt32     count = 0;
	PGVCOLNODE	collisttop = m_collist;

	while(collisttop && count < index)
	{
		collisttop = collisttop->pNext;
		count++;
	}

	return collisttop;
}

PGVROWNODE
CCustGrid::GetRowStruct(wyInt32 nrow)
{
	wyInt32		count = 0;
	PGVROWNODE	rowlisttop = m_rowlist;

	if(count >(m_row-1))
		return NULL;

	while(rowlisttop && count < nrow)
	{
		rowlisttop = rowlisttop->pNext;
		count++;
	}

	return rowlisttop;
}

wyBool
CCustGrid::ColTrueOrFalse(wyInt32 nrow, wyInt32 ncol)
{
	wyWChar		temptext[128] = {0};
	PGVCOLNODE	pgvcolnode = GetColNodeStruct(ncol);
	
	_ASSERT(pgvcolnode);

	if(!(pgvcolnode->pColumn.mask & GVIF_BOOL))
		return wyFalse;

	GetItemText(nrow, ncol, temptext);

	if(wcsicmp(temptext, L"true") == 0)
		return wyTrue;
		
   return wyFalse;
}

wyBool
CCustGrid::EndLabelEdit(wyInt32 nrow, wyInt32 ncol)
{
	LONG		length, cursel;
	wyWChar		text[1024]={0};
    wyString    temptext;
	wyString	textstr;
	LRESULT		ret;
	PGVCOLUMN	pgvcol=0;
	PGVCOLNODE	pgvcolnode=NULL;
	
	if(!m_lpgvwndproc)
		return wyFalse;

	// get the column type so that if its listbox type then we can get the complete
	// data.
	VERIFY(pgvcolnode  = GetColNodeStruct(ncol));
	
	if(GetOwnerData() == wyFalse)
		VERIFY(pgvcol = GetSubItemStruct(nrow, ncol));

    GetCurrentCellValue(pgvcolnode, temptext);

    ret = PostEndLabelEditMsg(nrow, ncol, temptext);

	if(!ret)	
		return wyFalse;

	if(pgvcol && pgvcol->issource != 'Y')
    {
		delete[] pgvcol->text;
        pgvcol->text = NULL;
    }
	
	// now if the column is of type list then we have to act differently.
	// if not owner data then keep a copy of the recently updated data
	if(GetOwnerData() == wyFalse)
    {
		if((m_pgvcurcolnode->pColumn.mask & GVIF_LIST))
        {
			cursel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_GETCURSEL, 0, 0);
			SendMessage(m_pgvcurcolnode->hwndCombo, LB_GETTEXT,(WPARAM)cursel, (LPARAM)text);
			textstr.SetAs(text);
			pgvcol->text = new wyChar[textstr.GetLength() + 1];
			strcpy(pgvcol->text, textstr.GetString());
		} 
        else	
        {
			length = temptext.GetLength();
			
			VERIFY(pgvcol->text = new wyChar[length + 2]);
			strcpy(pgvcol->text, temptext.GetString());
		}
	}

	if((m_pgvcurcolnode->pColumn.mask & GVIF_LIST) ||
        (m_pgvcurcolnode->pColumn.mask & GVIF_DROPDOWNNLIST) ||
        (pgvcolnode->pColumn.mask & GVIF_DROPDOWNLIST) ||
        (pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE))
    {		
        ShowWindow(m_pgvcurcolnode->hwndCombo, FALSE);
        m_lpgvwndproc(m_hwnd, GVN_FINISHENDLABELEDIT, (WPARAM)nrow, (LPARAM)ncol);
    }

	if((m_pgvcurcolnode->pColumn.mask & GVIF_TEXT))
    {
        m_lpgvwndproc(m_hwnd, GVN_FINISHENDLABELEDIT, NULL, NULL);
    }
		
    ShowWindow(m_pgvcurcolnode->hwndCombo, FALSE);
	
	if(pgvcol)
		pgvcol->issource = 'N';
	
	m_isediting = wyFalse;
	ShowWindow(m_hwndedit, FALSE);
    ShowWindow(m_hwndbrowsebutton, SW_HIDE);
	SetFocus(m_hwnd);

	return wyTrue;
}

void 
CCustGrid::GetCurrentCellValue(PGVCOLNODE pgvcolnode, wyString &temptext)
{
    wyInt32		ncount, listcount, length, cursel, count = 0;
    wyWChar		text[1024] ={0}, *buff;
	wyString	textstr;

	if(pgvcolnode->pColumn.mask & GVIF_LIST)
    {
		cursel = SendMessage(m_pgvcurcolnode->hwndCombo, LB_GETCURSEL, 0, 0);
		length = SendMessage(m_pgvcurcolnode->hwndCombo, LB_GETTEXT,(WPARAM)cursel, (LPARAM)text);
		temptext.SetAs(text);

	}
    else if(pgvcolnode->pColumn.mask & GVIF_DROPDOWNMULTIPLE)
    {
		ncount	= ListView_GetItemCount(m_pgvcurcolnode->hwndCombo);
        temptext.Clear();

		// now get the checked stuff,
		for(listcount = 0; listcount < ncount; listcount++)
        {
			if(ListView_GetCheckState(pgvcolnode->hwndCombo, listcount))
            {
				ListView_GetItemText(pgvcolnode->hwndCombo, listcount, 0, text, 1024-1);
				textstr.SetAs(text);
				temptext.AddSprintf("%s,", textstr.GetString());
				count++;
			}
		}

		if(count)
        {
			// we remove the extra ,
            temptext.Strip(1);
		}
	} 
	else 
    {
		length = SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, 0);
		buff = (wyWChar*)calloc(sizeof(wyWChar), (length + 2) * sizeof(wyWChar));
		SendMessage(m_hwndedit, WM_GETTEXT, length+1, (LPARAM)buff);
        temptext.SetAs(buff);
        free(buff);
    }
	return;
}

LONG 
CCustGrid::PostEndLabelEditMsg(wyInt32 nrow, wyInt32 ncol, wyString &text)
{
    GVDISPINFO  disp = {0};
    LONG        ret;
	
	if(GetOwnerData() == wyFalse)
    {
		ret = m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(nrow, ncol), (LPARAM)text.GetString());
	} 
    else 
    {
		disp.nRow = nrow; 
        disp.nCol = ncol;
		disp.text = (wyChar*)text.GetString(); 
		disp.cchTextMax = text.GetLength();

		ret = m_lpgvwndproc(m_hwnd, GVN_SETOWNERCOLDATA, (WPARAM)&disp, 0);
        ret = m_lpgvwndproc(m_hwnd, GVN_ENDLABELEDIT, MAKELONG(nrow, ncol), (LPARAM)text.GetString());
	}

    if(!ret)	
    {
		/*ShowWindow(m_hwndedit, FALSE);
		m_isediting = wyFalse;
		SetFocus(m_hwnd);*/
		return 0;
	}
    return 1;
}

wyBool
CCustGrid::EndComboEdit(wyInt32 row, wyInt32 col)
{

	if(m_hwndcurcombo)
    {
		EndLabelEdit(m_curselrow, m_curselcol);
		ShowWindow(m_hwndcurcombo, FALSE);
	}

	ShowWindow(m_hwndcurcombo, FALSE);
	m_iscomboediting = wyFalse;

	SetFocus(m_hwnd);

	return wyTrue;
}

void
CCustGrid::GetSubItemRect(wyInt32 row, wyInt32 col, LPRECT lprect)
{
	wyInt32	    rowcount = 0, colcount = 0;
	wyInt32     x = 0, y = 0;
    RECT        rectwin = {0}, recttemp = {0};
	PGVCOLNODE  pgvnode;
	PGVROWNODE  pgvrownode = m_rowlist;
    wyInt32     totrow = 0, totcol = 0;
    wyInt32     initrow = 0, initcol = 0;
    wyInt32     rcount = 0;

    ///In case of flip we interchange the total number of rows and column.
	if(m_flip == wyTrue)
    {
        /// Swapping the rows and columns for the flip
        totrow = col;
        col = row;
        row = totrow;

        initrow = m_initcol;
        initcol = m_initrow;

        totrow = m_col, totcol = m_row;
    }
    else
    {
        initrow = m_initrow;
        initcol = m_initcol;
        totrow = m_row, totcol = m_col;
    }
	
	ZeroMemory(lprect, sizeof(RECT));
	
	VERIFY(GetClientRect(m_hwnd, &rectwin));
	CopyMemory(&recttemp, &rectwin, sizeof(RECT));
	
	
	x += m_hight;

    if(m_form == wyTrue)
	{
        rcount = totcol;
        totcol = initcol + 1;
    }

	for(rowcount = initrow; rowcount < totrow; rowcount++)
	{
		if(rowcount == row)
		{
            pgvnode = m_collist;
            pgvrownode = m_rowlist;
			// this means we have reached the row the required row.
			// so we can traverse thru thru the columns
			pgvnode = m_collist;

            if(m_flip == wyTrue)
            {
                for(colcount = 0; colcount < initcol; colcount++)
                {
                    if(!pgvrownode)
                        continue;
                    pgvrownode = pgvrownode->pNext;
                }
            }
            else
            {
                for(colcount = 0; colcount < initcol; colcount++)
                {
                    if(!pgvnode)
                        continue;
				pgvnode = pgvnode->pNext;
                }
            }

            if(m_flip)
                y += m_maxwidth;
            else
				y += GV_DEFWIDTH;

			for(;colcount <= totcol && pgvnode != NULL; colcount++)
			{
				recttemp.left = y;
				recttemp.top  = x;
				recttemp.bottom = recttemp.top + m_hight;
                
                if(m_flip == wyTrue)
                {
                    if(pgvrownode)
                        recttemp.right = recttemp.left + pgvrownode->rowcx;
                }
				else if (pgvnode && pgvnode->isshow == wyTrue)
					recttemp.right = recttemp.left + pgvnode->pColumn.cx;

				if(colcount == col)
				{
					CopyMemory(lprect, &recttemp, sizeof(RECT));
                    return;
				}
				
                if(m_flip)
                {
                    y += pgvrownode->rowcx;
                    pgvrownode = pgvrownode->pNext;
                }
                else
                {
					if(pgvnode && pgvnode->isshow == wyTrue)
						y += pgvnode->pColumn.cx;

                    pgvnode = pgvnode->pNext;
                }
			}
		}
		x += m_hight;
	}


}

void
CCustGrid::SetFlip(wyBool enable)
{    
    m_flip = enable;
    InvalidateRect(m_hwnd, NULL, FALSE);
}


void
CCustGrid::SetFormMode(wyBool enable)
{
    m_form = enable;
    InvalidateRect(m_hwnd, NULL, FALSE);
}


wyBool
CCustGrid::GetOriginalText(wyInt32 row, wyInt32 col)
{
	wyInt32     len;
	wyUInt32	length = 1;
	PGVCOLUMN	pgvcol;
    GVDISPINFO  disp = {0};
	wyChar     *usrdata ;
	wyString	pgvcoltext, usrdatastr;

	if(m_oldcoltext)
    {
		delete[] m_oldcoltext;
        m_oldcoltext = NULL;
    }

	// depending upon whether its userdata or not we work accordingly
	if(GetOwnerData() == wyFalse)
    {
		len = GetItemTextLength(row, col);
		VERIFY(pgvcol = GetSubItemStruct(row, col));
		if(pgvcol->text)
        {
			pgvcoltext.SetAs(pgvcol->text);

			m_oldcoltext = new wyWChar[len + 1];
			wmemset(m_oldcoltext, 0, len + 1);
			wcscpy(m_oldcoltext, pgvcoltext.GetAsWideChar());
		} 
        else 
        {
			m_oldcoltext = new wyWChar[8];
			wcscpy(m_oldcoltext, L"(NULL)");
		}
	} 
    else 
    {
		disp.nRow = row; 
        disp.nCol = col; 
       
		disp.text = NULL; 
        disp.cchTextMax = 512 - 1;		
	
        if(m_lpgvwndproc(m_hwnd, GVN_GETDISPLENINFO,(WPARAM)&disp, 0) == FALSE)
            disp.cchTextMax = 512;	
	
        usrdata =(wyChar*)calloc(sizeof(wyChar), (disp.cchTextMax));
        disp.text = usrdata;

		//LPARAM sets for handling the base2 display format in grid
		m_lpgvwndproc(m_hwnd, GVN_GETDISPINFO,(WPARAM)&disp, 1);
					
		usrdatastr.SetAs(usrdata);
		usrdatastr.GetAsWideChar(&length);
		m_oldcoltext = new wyWChar[length + 1];

		wcscpy(m_oldcoltext, usrdatastr.GetAsWideChar());

		free(usrdata);
	}
	return wyTrue;
}

PROWDATA
CCustGrid::GetItemRow(wyInt32 row)
{
	wyInt32     nrow=0, ncol=0, nlen;
	PROWDATA    prowdata = NULL;
	PGVCOLNODE  pgvcolnode;
	PGVROWNODE  pgvrowlist;

	if(row < 0 || row > (m_row-1))
		return NULL;

	pgvrowlist = m_rowlist;

	if(pgvrowlist == NULL)
		return NULL;

	while(nrow < row)
	{
		pgvrowlist = pgvrowlist->pNext;
		nrow++;
	}

	if(pgvrowlist == NULL)
		return NULL;

	pgvcolnode = pgvrowlist->pColumn;
	prowdata = new ROWDATA;

	prowdata->nCol	= m_col;
	prowdata->pszData = new wyChar*[m_col];
	prowdata->source = new wyBool[m_col];

	for(ncol = 0; ncol < m_col; ncol++)
	{
		if(pgvcolnode->pColumn.text)
        {
			nlen = strlen(pgvcolnode->pColumn.text);
			prowdata->pszData[ncol] = new wyChar[nlen+1];

			strcpy(prowdata->pszData[ncol], pgvcolnode->pColumn.text);

			if(pgvcolnode->pColumn.issource == 'Y')
				prowdata->source[ncol] = wyTrue;
			else
				prowdata->source[ncol] = wyFalse;

			pgvcolnode = pgvcolnode->pNext;
		}
        else if(pgvcolnode->pColumn.issource == 'Y')
        {
			prowdata->pszData[ncol] = new wyChar[strlen(STRING_NULL)+1];
			strcpy(prowdata->pszData[ncol], STRING_NULL);
			pgvcolnode = pgvcolnode->pNext;
		}
        else 
        {
			prowdata->pszData[ncol] = new wyChar[2];
			prowdata->pszData[ncol][0] = NULL;
			prowdata->source[ncol] = wyFalse;
			pgvcolnode = pgvcolnode->pNext;			
		}
	}
	return prowdata;
}

LPARAM
CCustGrid::GetLongData()
{
	return m_lparamdata;	
}

wyBool
CCustGrid::GetBoolValue(wyInt32 nrow, wyInt32 ncol)
{
	PGVCOLUMN	pgvcol;

	pgvcol = GetSubItemStruct(nrow, ncol);
	if(!pgvcol)
		return wyFalse;

	if(stricmp(pgvcol->text, GV_TRUE)== 0)
		return wyTrue;
		
    return wyFalse;
}

wyInt32
CCustGrid::SetRowCheckState(wyInt32 nrow, wyBool ustate)
{
	PGVROWNODE      rownode;
	wyInt32         oldstate;
	
	if((ustate != GV_CHEKCED && ustate != GV_UNCHECKED)|| !(m_exstyle & GV_EX_ROWCHECKBOX))
		return GV_ERR;

	if(!GetOwnerData())
	{
		VERIFY(rownode = GetRowStruct(nrow));
		oldstate = rownode->excheck;
		rownode->excheck = ustate;
        if(oldstate != ustate)
        { 
            if( ustate == wyTrue)
                m_checkcount++;
            else
                m_checkcount--;
        }
        SetSelAllState();
	}
	else
	{
		VERIFY(m_rowcheck);
		oldstate = m_rowcheck[nrow];
		m_rowcheck[nrow] = (ustate == wyTrue)?1:0;
	}

	return oldstate;
}

wyInt32
CCustGrid::GetRowCheckState(wyInt32 nrow)
{
	PGVROWNODE		rownode;

	if(!(m_exstyle & GV_EX_ROWCHECKBOX))
		return GV_ERR;

	if(!GetOwnerData())
    {
		VERIFY(rownode = GetRowStruct(nrow));
		return rownode->excheck;
	} 
    else 
    {
		VERIFY(m_rowcheck);
		if(m_rowcheck[nrow] == 1)
			return 1;
	}
    return 0;
}

wyBool
CCustGrid::GetOwnerData()
{
	return m_ownerdata;
}

void
CCustGrid::SetOwnerData(wyBool val)
{
	m_ownerdata = val;
}

void
CCustGrid::SetMaxRowCount(LONG count)
{
	if(GetOwnerData())
    {
		// value for rowcheck
		if(m_rowcheck)
        {
			delete[] m_rowcheck;
            m_rowcheck = NULL;
        }

		m_rowcheck = new wyChar[count+1];
		memset(m_rowcheck, 0, sizeof(wyChar)*(count+1));
		m_row = count;

        m_curselrow = min(m_row - 1, m_curselrow);
	}
    return;
}

PROWDATA
CCustGrid::FreeRowData(PROWDATA prowdata)
{
	wyInt32 colcount;

	if(!prowdata)
		return NULL;

	for(colcount = 0; colcount < prowdata->nCol; colcount++)
		delete[] prowdata->pszData[colcount];
		
	delete[] prowdata->pszData;
	delete[] prowdata->source;

	delete prowdata;

	return NULL;
}

PGVCOLUMN
CCustGrid::GetSubItemStruct(wyInt32 row, wyInt32 col)
{
	wyInt32     rowcount = 0, colcount = 0;
	RECT        rectwin, recttemp;
    PGVCOLNODE  pgvnode = {0};
    PGVROWNODE  pgvrownode = {0};
	
	VERIFY(GetClientRect(m_hwnd, &rectwin));
	CopyMemory(&recttemp, &rectwin, sizeof(RECT));
	
	pgvrownode = m_rowlist;

	if(row == ((m_row)-1))
    {
		pgvrownode = m_rowlast;
    }
	else 
    {
		for(rowcount = 0;(rowcount <= m_row && pgvrownode != NULL); 
                        rowcount++, pgvrownode = pgvrownode->pNext)
		{
			if(rowcount == row)
				break;
		}
	}

	if(!pgvrownode)
		return NULL;

	pgvnode = pgvrownode->pColumn;

	for(colcount = 0;(colcount <= m_col && pgvnode != NULL); 
                        colcount++, pgvnode = pgvnode->pNext)
	{
		if(colcount == col)
			return &pgvnode->pColumn;
	}

	return NULL;
}

wyBool
CCustGrid::IsRowEmpty(wyInt32 nrow)
{
	wyInt32     colcount;
	PROWDATA	prowdata = GetItemRow(nrow);
	PGVCOLNODE	colnode;
	
	for(colcount = 0; colcount < prowdata->nCol; colcount++)
	{
		VERIFY(colnode = GetColNodeStruct(colcount));
		
		if((strlen(prowdata->pszData[colcount]) != 0) && 
          ((stricmp(prowdata->pszData[colcount], GV_FALSE)) != 0))
        {
			if(colnode->pszDefault && 
              ((stricmp(colnode->pszDefault, prowdata->pszData[colcount])) == 0))
				continue;

			FreeRowData(prowdata);
			return wyFalse;
		}
	}

	FreeRowData(prowdata);

	return wyTrue;
}

wyInt32
CCustGrid::RowPerPage()
{
	wyInt32 ncount = 0;
	RECT	rectwin;
	
	VERIFY(GetClientRect(m_hwnd, &rectwin));

	ncount =(wyInt32)( rectwin.bottom / m_hight);

    return ncount - 3;
}

wyBool
CCustGrid::SetRowLongData(wyInt32 nrow, LPARAM lparam)
{
	wyInt32	    nrowtemp=0;;
	PGVROWNODE	pgvrownode = m_rowlist;

	if(nrow == (m_row - 1))
		pgvrownode = m_rowlast;
	else 
    {
		while(pgvrownode && (nrowtemp < nrow))
		{
			pgvrownode = pgvrownode->pNext;
			nrowtemp++;
		}
	}

	pgvrownode->lparam = lparam;

	return wyTrue;
}

LPARAM
CCustGrid::GetRowLongData(wyInt32 nrow)
{
	wyInt32     nrowtemp=0;
	PGVROWNODE	pgvrownode = m_rowlist;

	if(nrow == -1)
		return NULL;

	while(pgvrownode && (nrowtemp < nrow))
	{
		pgvrownode = pgvrownode->pNext;
		nrowtemp++;
	}

	VERIFY(pgvrownode);

	return pgvrownode->lparam;
}

LPARAM
CCustGrid::GetItemLongValue(wyInt32 nrow, wyInt32 ncol)
{
	PGVCOLUMN	pgvcol;

	pgvcol	=	GetSubItemStruct(nrow, ncol);
	
	if(!pgvcol)
		return NULL;
	return pgvcol->lparam;
}

LPARAM
CCustGrid::SetItemLongValue(wyInt32 nrow, wyInt32 ncol, LPARAM lparam)
{
	LPARAM		prev;
	PGVCOLUMN	pgvcol;

	pgvcol	=	GetSubItemStruct(nrow, ncol);
	
	if(!pgvcol)
		return -1;

	prev = pgvcol->lparam;
	pgvcol->lparam = lparam;

	return prev;
}

LPARAM
CCustGrid::GetColumnLongValue(wyInt32 ncol)
{
	PGVCOLNODE	pgvcolnode;

	pgvcolnode	=	GetColNodeStruct(ncol);
	
	if(!pgvcolnode)
		return -1;

	return pgvcolnode->pColumn.lparam;
}

wyInt32
CCustGrid::GetColumnTitle(wyInt32 ncol, wyWChar *buffer)
{
	PGVCOLNODE  pgvcolnode;
	wyString	pgvcoltext;

	pgvcolnode =	GetColNodeStruct(ncol);

	if(!pgvcolnode)
		return -1;
	
	pgvcoltext.SetAs(pgvcolnode->pColumn.text);

	wcscpy(buffer, pgvcoltext.GetAsWideChar());

	return wcslen(buffer);
}


LPARAM
CCustGrid::SetColumnLongValue(wyInt32 ncol, LPARAM lparam)
{
	LPARAM		prev;
	PGVCOLNODE	pgvcolnode;

	pgvcolnode = GetColNodeStruct(ncol);
	
	if(!pgvcolnode)
		return -1;

	prev = pgvcolnode->pColumn.lparam;
	pgvcolnode->pColumn.lparam = lparam;

	return prev;
}

wyInt32
CCustGrid::InsertRowInBetween(wyInt32 nrow)
{
	wyInt32     nrowindex=0;
	PGVCOLNODE	colnode, *temp; 
	PGVROWNODE	pgvrownode = m_rowlist, rownode;

	if(nrow > m_row)
		return Insert_Row();

	if(nrow == -1)
		return -1;

	// create a new row.
	rownode = new GVROWNODE;

	rownode->lparam = NULL;
	rownode->excheck = wyFalse;
	rownode->pNext = NULL;
    rownode->rowcx = m_maxwidth;
	
	colnode = m_collist;

	temp = &rownode->pColumn;
	*temp = NULL;

	while(colnode != NULL)
	{
		*temp = new GVCOLNODE;
		memset(*temp, 0, sizeof(GVCOLNODE));
		
		// now see if the type is of bool or not. if it is then we have to handle
		// it differently.
		if(colnode->pColumn.mask & GVIF_BOOL)
		{
			(*temp)->pColumn.text = new wyChar[6];
			strcpy((*temp)->pColumn.text, "false");
			(*temp)->pColumn.issource = 'N';
		}
        else if(colnode->pszDefault)
        {
			(*temp)->pColumn.text = new wyChar[strlen(colnode->pszDefault)+ 1];
			strcpy((*temp)->pColumn.text, colnode->pszDefault);
			(*temp)->pColumn.issource = 'N';
		} 
        else 
        {
			(*temp)->pColumn.text = new wyChar[2];
			(*temp)->pColumn.text[0] = 0;
			(*temp)->pColumn.issource = 'N';
		}

		(*temp)->pColumn.uIsReadOnly = wyFalse;
		(*temp)->pColumn.cx = colnode->pColumn.cx;
		//(*temp)->pColumn.text = new wyChar[2];
		(*temp)->pColumn.text[0] = 0;
		(*temp)->pColumn.fmt = colnode->pColumn.fmt;
        (*temp)->pColumn.uIsReadOnly = colnode->pColumn.uIsReadOnly;
		(*temp)->pNext = NULL;
		temp = &(*temp)->pNext;
		colnode = colnode->pNext;
	}

	// now if the row is 0 then we have to add it at the beginning.
	if(nrow == 0)
	{
		rownode->pNext = m_rowlist;
		m_rowlist = rownode;
		m_row++;
		return 0;
	}

	while(nrowindex < nrow-1)
	{
		pgvrownode = pgvrownode->pNext;
		nrowindex++;
	}

	rownode->pNext = pgvrownode->pNext;
	pgvrownode->pNext = rownode;

	if((nrowindex+1)==(m_row))
		m_rowlast = rownode;
	else 
    {
		// we have to change the lat row pointer.
		for(pgvrownode = m_rowlist; pgvrownode->pNext != NULL; pgvrownode = pgvrownode->pNext);

		m_rowlast = pgvrownode;

	}
	
	m_row++;
	return	nrowindex+1;
}

wyBool
CCustGrid::ApplyChanges()
{
	if(m_isediting == wyTrue)
		return EndLabelEdit(m_curselrow, m_curselcol);
	else if(m_iscomboediting == wyTrue)
		return EndComboEdit(m_curselrow, m_curselcol);

	return wyTrue;
}

wyBool
CCustGrid::SetCurSelection(wyInt32 nrow, wyInt32 ncol, wyBool sendmsg)
{
    LRESULT ret;

	if((nrow > m_row - 1) || (ncol > m_col - 1))
		return wyFalse;
	
	if(sendmsg == wyTrue)
    {
        if(m_curselrow != nrow && !m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, nrow, ncol))
        {
            return wyFalse;
        }

		ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, nrow, ncol);
		if(!ret)
			return wyFalse;
	}

	m_pgvcurcolnode = GetColNodeStruct(ncol);

	m_curselcol = ncol;
	m_curselrow = nrow;

	if(m_curselrow > (m_initrow + RowPerPage()))
		m_initrow = m_curselrow - RowPerPage();
	if(m_curselrow < m_initrow)
		m_initrow = m_curselrow;

	InvalidateRect(m_hwnd, NULL, FALSE);
    if(sendmsg == wyTrue)
		m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, nrow, ncol);
	return wyTrue;
}

wyBool
CCustGrid::SetCurSelRow(LONG nrow, wyBool sendmsg)
{
    LRESULT ret;

	if(nrow > m_row-1)
		return wyFalse;

	if(sendmsg == wyTrue && m_curselrow >= 0)
    {
        if(m_curselrow != nrow && !m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, nrow, m_curselcol))
        {
            return wyFalse;
        }

		ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);
		if(!ret)
			return wyFalse;
	}

	m_curselrow = nrow;

    if(sendmsg == wyTrue)
    {
        m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
    }

	return wyTrue;
}

wyBool
CCustGrid::SetCurSelCol(LONG ncol, wyBool sendmsg)
{
    LRESULT ret;

	if(ncol > m_col-1)
		return wyFalse;

	if(sendmsg == wyTrue)
    {
	
		ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, ncol);
		if(!ret)
			return wyFalse;
	}

	m_curselcol = ncol;
	return wyTrue;
}

wyBool
CCustGrid::SetInitRow(wyInt32 row)
{
	if(row && (row > (m_row-1)))
		return wyFalse;

	m_initrow = max(0, row);

	return wyTrue;
}

wyBool
CCustGrid::SetInitCol(wyInt32 col)
{
	if(col &&(col > (m_col-1)))
		return wyFalse;

	m_initcol = max(0, col);

	return wyTrue;
}

LONG
CCustGrid::GetCurSelection(wyBool focus)
{
	LONG rowcol = -1;

	if(focus == wyTrue)
	{	
		if(GetFocus() == m_hwnd)
			rowcol = MAKELONG(m_curselrow, m_curselcol);
	}
	else
		rowcol = MAKELONG(m_curselrow, m_curselcol);

	return		rowcol;
}
LONG
CCustGrid::SetExtendedStyle(LONG newstyle)
{
	LONG	oldstyle = m_exstyle;

	m_exstyle = newstyle;

	return oldstyle; 
}

wyBool
CCustGrid::SetColumnReadOnly(wyInt32 nrow, wyInt32 ncol, wyBool ustate)
{
	PGVCOLUMN	pgvcol;

	pgvcol = GetSubItemStruct(nrow, ncol);

	if(!pgvcol)
		return wyFalse;

	pgvcol->uIsReadOnly = ustate;

	return wyTrue;
}

wyBool
CCustGrid::SetButtonVis(wyInt32 nrow, wyInt32 ncol, wyBool ustate)
{
	PGVCOLUMN	pgvcol;

	VERIFY(pgvcol = GetSubItemStruct(nrow, ncol));

	pgvcol->uIsButtonVis = ustate;

	return wyTrue;
}

wyBool
CCustGrid::SetColumnWidth(wyInt32 ncol, wyUInt32 cx)
{
	PGVCOLNODE	pgvnode;

	VERIFY(pgvnode = GetColNodeStruct(ncol));

	pgvnode->pColumn.cx = cx;

	return wyTrue;
}


wyBool
CCustGrid::ProcessOnListFocus(wyInt32 nrow, wyInt32 ncol)
{
	wyInt32 width = GV_LIST_WIDTH, diff, height = GV_LIST_HEIGHT;

	RECT	rectwin, rectlistbox, rectitem;

	VERIFY(GetClientRect(m_hwnd, &rectwin));
	
	// get the position of the rectangle of the item.
	GetSubItemRect(nrow, ncol, &rectitem);

	// decide whether to show it on top or bottom.
	if(rectitem.bottom + height > rectwin.bottom && (rectitem.top - rectwin.top > rectwin.bottom - rectitem.bottom))	
    {
		// it will be on top
		rectlistbox.left = rectitem.left;
		rectlistbox.top	 = rectitem.top - height;
		rectlistbox.right = rectitem.right + width;
		rectlistbox.bottom = rectitem.top;
	} 
    else 	
    {
		// it will be on bottom.
		rectlistbox.left = rectitem.left;
		rectlistbox.top	 = rectitem.bottom;
		rectlistbox.right = rectitem.right + width;
		rectlistbox.bottom = rectitem.bottom + height;
	}

	/*	starting from v4.2 BETA there is a fix for left hand side too i.e. if the listbox is way too much on right
		we show a little left */
	if(rectlistbox.right >= rectwin.right)
    {
		/* we move the right to subitem rect right and move that many pixel on left */
		diff = rectlistbox.right - rectitem.right;
		rectlistbox.right = rectitem.right;
		rectlistbox.left -= diff;
	}
    SendMessage(m_pgvcurcolnode->hwndCombo, WM_SETFONT, (WPARAM)m_hfont, TRUE);
	VERIFY(MoveWindow(m_pgvcurcolnode->hwndCombo, rectlistbox.left, rectlistbox.top, rectlistbox.right-rectlistbox.left, rectlistbox.bottom-rectlistbox.top, TRUE));
	ShowWindow(m_pgvcurcolnode->hwndCombo, TRUE);

	return wyTrue;
}

wyBool
CCustGrid::CalculatePageDown(wyBool isonpgdwnkey)
{
	wyInt32	rowperpage;
	SHORT   state;
	LRESULT ret;
    LONG nrow = 0, initrow = 0;

    if(m_flip == wyTrue)
        return wyTrue;

    if(m_row == 0)
        return wyFalse;

    state = GetKeyState(VK_CONTROL);

	VERIFY(rowperpage = RowPerPage() + GV_PAGEUP_DISP);

	if(m_isediting == wyTrue || m_iscomboediting == wyTrue)
		ApplyChanges();

    if(isonpgdwnkey)
    {
        if(state & 0x8000)
        {
            nrow = m_row-1;
            initrow = (nrow - rowperpage >= 0) ? (nrow - rowperpage):(0);
        }
        else
        {
            nrow = (m_curselrow + rowperpage) >= m_row ? m_row-1 : m_curselrow + rowperpage;
            initrow = (nrow - rowperpage >= 0) ? (nrow - rowperpage) : 0;
        }

        if(m_curselrow == nrow)
            {
            return wyTrue;
	        }

        if(!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, nrow, m_curselcol))
	        {
            return wyFalse;
        }

	    ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);

	    if(!ret)
		    return wyFalse;

        m_curselrow = nrow;
        m_initrow = initrow;
        m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);

        return wyTrue;
	}

	
	if((m_initrow + rowperpage) >= m_row)
    {
		m_initrow = (m_row - rowperpage >= 0) ? (m_row - rowperpage):(0);
	}
	else
	{
		m_initrow += rowperpage;
	}
	

	return wyTrue;
}

wyBool
CCustGrid::CalculatePageUp(wyBool isonpgupkey)
{
	wyInt32	rowperpage;
	SHORT	state;
	LRESULT	ret;
    LONG    nrow = 0, initrow = 0;

    if(m_flip == wyTrue)
        return wyTrue;
    
    if(m_row == 0)
        return wyFalse;

    state = GetKeyState(VK_CONTROL);

	VERIFY(rowperpage = RowPerPage() + GV_PAGEUP_DISP);
	
	if(m_isediting == wyTrue || m_iscomboediting == wyTrue)
		ApplyChanges();

    if(isonpgupkey == wyTrue)
    {
	    if(state & 0x8000)
        {
		    nrow = 0;
		    initrow = 0;
	    }
        else 
        {
            nrow = (m_curselrow - rowperpage) < 0 ? 0 : m_curselrow - rowperpage;
            initrow = nrow;
	    }

        if(m_curselrow == nrow)
		    {
            return wyTrue;
		    }

        if(!m_lpgvwndproc(m_hwnd, GVN_ROWCHANGINGTO, nrow, m_curselcol))
		    {
            return wyFalse;
	    }
	
        ret = m_lpgvwndproc(m_hwnd, GVN_BEGINCHANGEROW, m_curselrow, m_curselcol);

	    if(!ret)
		    return wyFalse;
	
        m_curselrow = nrow;
        m_initrow = initrow;
	    m_lpgvwndproc(m_hwnd, GVN_ENDCHANGEROW, m_curselrow, 0);
        return wyTrue;
    }
	 
    if((m_initrow - rowperpage) <= 0)
	{
		m_initrow = 0;
	}
	else
	{
		m_initrow -= rowperpage;
	}
		
	return wyTrue;
}

wyInt32
CCustGrid::ColumnPerPage()
{
    PGVROWNODE      pgvrownode = m_rowlist;
    wyInt32         count = 0, cx = 0;
    RECT		    rectwin = {0};

    VERIFY(GetClientRect(m_hwnd, &rectwin));

    /// Reach the initial row
    while(count < m_initrow)
    {
        pgvrownode = pgvrownode->pNext;
        count++;
    }

    count = 0;
    cx = m_maxwidth;

    /// Determine the number of rows that can fit(column becomes rows in flip mode)
    while(cx < rectwin.right)
    {
        cx += pgvrownode->rowcx;
        count++;
    }

    return count - 1;
}

// Function to ensure that a cell is visible in the screen.
wyBool
CCustGrid::EnsureVisible(wyInt32 nrow, wyInt32 ncol, wyBool init)
{
	LONG		rowperpage=0;
	wyInt32	    totalcx = 0, width = 0, totalwidth = 0;
	PGVCOLNODE	colnode = NULL;
	RECT		rectwin = {0}, rect = {0};

	if(init == wyTrue)
	{
		m_initcol = 0;
		m_initrow = 0;
	}

	VERIFY(GetClientRect(m_hwnd, &rectwin));

	VERIFY(rowperpage = RowPerPage());

    //..Reorder Columns Issue..
    //..MoveUp was not moving up the Grid scrollbar 1 row..
    
    if(nrow < m_initrow && nrow >= 0)
        m_initrow = nrow;
    else if(nrow > (m_initrow + rowperpage))
        m_initrow = nrow - rowperpage;
    /*
    if(nrow < m_initrow)
        m_initrow = nrow;
    else if(nrow > (m_initrow + rowperpage + 1))
    {
        m_initrow = max(0, nrow - rowperpage - 1);
    }
    */
    /*
    if(nrow < m_initrow || nrow > m_initrow + rowperpage)
    {
	// we make it visible if its not already so
        if(m_initrow != nrow && (nrow > (m_initrow + rowperpage - 1) || nrow < m_initrow))
        {
		    if(nrow < rowperpage)
			    m_initrow = 0;
		    else 
			    m_initrow = max(0, nrow - rowperpage);		// dont make it -1
	    }
	}
    */
	// we calulate width by leaving space on both left and right.
	if(m_flip == wyTrue)
        width = (rectwin.bottom - rectwin.top) - (2 * m_hight);

    else
		width = (rectwin.right - rectwin.left) - (2 * GV_DEFWIDTH);		
	
	// we get the width of the column.
	colnode = GetColNodeStruct(ncol);
	if(!colnode)
        return wyFalse;

	totalcx = colnode->pColumn.cx;

	CustomGrid_GetSubItemRect(m_hwnd, nrow, ncol, &rect);

	//if the column is fully visible in the grid, then no need to move the columns
	if(m_flip == wyFalse && rect.right < rectwin.right)
		return wyFalse;

	if(m_flip == wyTrue && rect.bottom < rectwin.bottom)
		return wyFalse;

	// now we loop thru backwards and keep on calculating the sum width of the preceding
	// columns. when the total becomes more then the width we break.
	while(ncol >= 0)// && m_initcol != ncol) //right arrow issue in grid. http://code.google.com/p/sqlyog/issues/detail?id=282
    {
		VERIFY(colnode = GetColNodeStruct(ncol));
		if(m_flip == wyTrue)
            totalwidth += m_hight;
        else
		totalwidth += colnode->pColumn.cx;

		if(m_flip == wyFalse && totalwidth >= width)
        {
			// dont go beyond the boundary
			m_initcol = min(ncol + 1, m_col - 1);			
			break;
		}

        if(m_flip == wyTrue && totalwidth > width)
        {
			// dont go beyond the boundary
			m_initcol = min(ncol + 1, m_col - 1);
			break;
		}

		ncol--;
	}

	m_initcol = max(m_initcol, 0);		// dont go beyond the boundary

	return wyTrue;
}

// Function to ensure that the last selected column is visble in the screen.
// This is required when the user presses END and focus has to change.

wyBool
CCustGrid::ShowLastCol()
{

	wyInt32     totalcx = 0, width = 0, colcount;
	PGVCOLNODE	colnode = NULL;
	RECT        rectwin = {0}, rectitem = {0};

	VERIFY(GetClientRect(m_hwnd, &rectwin));
    GetSubItemRect(m_curselrow, m_curselcol, &rectitem);

	if(rectwin.right > rectitem.right)
		return wyTrue;

	width = (rectwin.right - rectwin.left)- GV_DEFWIDTH;

	VERIFY(colnode = GetColNodeStruct(m_col - 1));

	totalcx = colnode->pColumn.cx;

	colcount = m_col-2;
	while((totalcx < width) && (colcount > 0))
    {
		VERIFY(colnode = GetColNodeStruct(colcount));
		totalcx += colnode->pColumn.cx;
		colcount -= 1;
	}

	m_initcol = colcount + 2;
	return wyTrue;
}

// Function to work with default values of column.

wyChar*
CCustGrid::GetColumnDefault(wyInt32 ncol)
{
	PGVCOLNODE		colnode;

	VERIFY(colnode = GetColNodeStruct(ncol));

	return colnode->pszDefault;

}

wyBool
CCustGrid::SetColumnDefault(wyInt32 ncol, const wyChar *buf)
{
	PGVCOLNODE		colnode;

	VERIFY(colnode = GetColNodeStruct(ncol));

	if(colnode->pszDefault)
    {
		delete[] colnode->pszDefault;
        colnode->pszDefault = NULL;
    }

	if(buf)
    {
		VERIFY(colnode->pszDefault = new wyChar[strlen(buf)+ 1 ]);
		strcpy(colnode->pszDefault, buf);
	} 
    else 
    {
		VERIFY(colnode->pszDefault = new wyChar[8]);
		strcpy(colnode->pszDefault, "(NULL)");
	}

	return wyTrue;
}

/* Check whether the text is in utf8 or not, if it is then we have to display it by using drawTextW*/

LONG 
CCustGrid::GetCodePage(wyChar *buff)
{
	wyInt32 len = strlen(buff);
	wyInt32 nos = 1;
	DetectEncodingInfo	pencode[1]	= {0};
	IMultiLanguage2 *pimlang2;
	IMultiLanguage *pimlang;

	/* Creates a single uninitialized object of the class associated with a specified CLSID */
	CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_ALL,IID_IMultiLanguage,(void **)&pimlang); 

	/*	Returns a pointer to a specified interface on an object to which a client currently holds an interface pointer */
	pimlang->QueryInterface(IID_IMultiLanguage2,(void **)&pimlang2); 

    /* Detects the code page of the given string. */
	pimlang2->DetectInputCodepage(MLDETECTCP_8BIT, 0, buff, &len, pencode, &nos);

	/* return the code page of the buffer supplied*/
	return pencode[0].nCodePage;
}

void
CCustGrid::ShowGrid(wyBool isvisible)
{
    if(isvisible == wyTrue)
    {
        // create all the GDI objects here 
        m_isvisible = wyTrue;
        VERIFY(CreateFonts());
		m_crbkgnd = RGB(255, 255, 255);
        OnCreate(m_lparam);
        ShowWindow(m_hwnd, TRUE);
    }
    else
    {
        m_isvisible = wyFalse;
        
        DestroyWindow(m_hwndedit);
        m_hwndedit = NULL;

        DestroyWindow(m_hwndbrowsebutton);
        m_hwndbrowsebutton = NULL;

        DestroyWindow(m_hwndsplitter);
        m_hwndsplitter = NULL;

        // destroy all the GDI objects here
        if(m_hfont)
            VERIFY(DeleteFont(m_hfont));
        m_hfont = NULL;

        if(m_hItalicsFont)
            VERIFY(DeleteFont(m_hItalicsFont));
        m_hItalicsFont = NULL;

        if(m_htopfont)
	        VERIFY(DeleteFont(m_htopfont));
        m_htopfont = NULL;

        ShowWindow(m_hwnd, FALSE);
    }
}

wyUInt32
CCustGrid::GetLinesToScrollUserSetting()
{
    wyUInt32 usersetting;

    // Retrieve the lines-to-scroll user setting.
    BOOL success = SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &usersetting, 0);

    // Use a default value if the API failed.
    if( success == FALSE )
        usersetting = DEF_WHEELSCROLLLINES;

    return usersetting;
}


wyInt32
CCustGrid::GetRowHeader(POINT *pnt, RECT* rect)
{
    PGVCOLNODE  pgvnode = m_collist;
    PGVROWNODE  pgvrownode = m_rowlist;
    wyInt32     x = 0;
    wyInt32     count = 0;
    RECT        rectwin;

    VERIFY(GetClientRect(m_hwnd, &rectwin));

    if(pnt->y > m_hight)
        return -1;

    if(m_flip == wyTrue)
    {
        x = m_maxwidth;

        while(count < m_initrow)
        {
            pgvrownode = pgvrownode->pNext;
            count++;
        }

        count = 0;
        while(pgvrownode != NULL)
	    {
		    if(pnt->x >= x && pnt->x <= x + pgvrownode->rowcx) {
				if(rect) {
					rect->left = x;
					rect->right = x + pgvrownode->rowcx;
					rect->top = 0;
					rect->bottom = m_hight;
				}

                return m_initrow + count;
			}
            
            count++;

            x += pgvrownode->rowcx;
            pgvrownode = pgvrownode->pNext;
	    }
    }
    else
    {
        x = GV_DEFWIDTH;

        while(count < m_initcol)
        {
            pgvnode = pgvnode->pNext;
            count++;
        }

        count = 0;
        while(pgvnode != NULL)
	    {
			if(pgvnode && pgvnode->isshow == wyTrue && pnt->x >= x && pnt->x <= x + pgvnode->pColumn.cx) {
				if(rect) {
					rect->left = x;
					rect->right = x + pgvnode->pColumn.cx;
					rect->top = 0;
					rect->bottom = m_hight;
				}

                return m_initcol + count;
			}
            
            count++;

            if(pgvnode && pgvnode->isshow == wyTrue)
				x += pgvnode->pColumn.cx;

            pgvnode = pgvnode->pNext;
	    }
    }


	return -1;
}

wyInt32
CCustGrid::ColumnVisibleCount()
{
	wyInt32     totalcx = 0, width = 0, colcount=0, ncol;
	PGVCOLNODE	colnode = NULL;
	RECT        rectwin = {0};

	VERIFY(GetClientRect(m_hwnd, &rectwin));
	// we calculate width by leaving space on both left and right 
	width = (rectwin.right - rectwin.left) - (GV_DEFWIDTH);

	ncol = m_initcol - 1 ;
	VERIFY(colnode = GetColNodeStruct(ncol));
	 // we get the width of the column.
	totalcx = colnode->pColumn.cx;
	
	// now we loop thru backwards and keep on calculating the sum width of the preceding
	// columns. when the total becomes more then the width we break.	
	while((totalcx < width) && (ncol >= 0))
    {
		ncol -= 1;
		VERIFY(colnode = GetColNodeStruct(ncol));
		totalcx += colnode->pColumn.cx;
		colcount++;
	}
	
	if(!colcount)
		colcount ++;
	
	return (colcount);
}

void
CCustGrid::SetGradient(HDC hdcmem, TRIVERTEX *vertex, wyBool vertical)
{
	// Create a GRADIENT_RECT structure that
	// references the TRIVERTEX vertices.
	GRADIENT_RECT gRect;
	
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

	if(vertical == wyTrue)
		GradientFill(hdcmem, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

	else
		GradientFill(hdcmem, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
}

wyBool
CCustGrid::GetColumnWidth(wyInt32 col, wyUInt32 * width)
{
    PGVCOLNODE pgvnode;
    wyBool ret = wyTrue;

    pgvnode = GetColNodeStruct(col);

    if(pgvnode)
        *width = pgvnode->pColumn.cx;
    else
        ret = wyFalse;

    return ret;
}

//VOID
//CCustGrid::ShowOrHideScollBar(wyInt32 scrollid, wyBool status)
//{	
//	BOOL state;
//
//	state = (status == wyTrue) ? TRUE : FALSE;
//
//	ShowScrollBar(m_hwnd, scrollid, status);
//
//	return;
//}

VOID	
CCustGrid::ShowOrHideColumn(wyInt32 column, wyBool ishide)
{
	PGVCOLNODE	pgvnode = NULL;
	
	pgvnode = GetColNodeStruct(column);

	if(pgvnode)
		(ishide == wyTrue) ? pgvnode->isshow = wyTrue: pgvnode->isshow = wyFalse;

	VERIFY(InvalidateRect(m_hwnd, NULL, TRUE));

	return;
}

void
CCustGrid::GetActiveControlText(wyWChar* text, wyInt32 length)
{
    if(IsWindowVisible(m_hwndedit))
    {
        GetWindowText(m_hwndedit, text, length);
    }
    else if(m_pgvcurcolnode && m_pgvcurcolnode->hwndCombo && IsWindowVisible(m_pgvcurcolnode->hwndCombo))
    {
        wyInt32 count, i;
        wyWChar buffer[1024];
        wyString temp, str;

        for(i = 0, count = ListView_GetItemCount(m_pgvcurcolnode->hwndCombo); i < count; ++i)
        {
            if(ListView_GetCheckState(m_pgvcurcolnode->hwndCombo,i))
            {
                if(str.GetLength())
                {
                    str.Add(",");
                }

                ListView_GetItemText(m_pgvcurcolnode->hwndCombo, i, 0, buffer, 1024);
                temp.SetAs(buffer);
                str.Add(temp.GetString());
            }
        }

        wcsncpy(text, str.GetAsWideChar(), length);
    }
}

wyInt32
CCustGrid::GetActiveControlTextLength()
{
    wyInt32 count, i, size = 0;

    if(IsWindowVisible(m_hwndedit))
    {
        return GetWindowTextLength(m_hwndedit);
    }
    else if(m_pgvcurcolnode && m_pgvcurcolnode->hwndCombo && IsWindowVisible(m_pgvcurcolnode->hwndCombo))
    {
        wyWChar buffer[1024];

        for(i = 0, count = ListView_GetItemCount(m_pgvcurcolnode->hwndCombo); i < count; ++i)
        {
            if(ListView_GetCheckState(m_pgvcurcolnode->hwndCombo,i))
            {
                if(size)
                {
                    size++;
                }

                ListView_GetItemText(m_pgvcurcolnode->hwndCombo, i, 0, buffer, 1024);
                size += wcslen(buffer) + 1;
            }
        }
    }

    return size;
}

GVWNDPROC 
CCustGrid::GetGridProc()
{
    return m_lpgvwndproc;
}

wyInt32 
CCustGrid::GetSelAllState()
{
    return m_selallinfo.checkstate;
}
void
CCustGrid::SetSelAllState(wyInt32 state)
{
    PGVROWNODE	pgvrownode = NULL;
    pgvrownode = m_rowlist;
    
    if(state == -1)
    {
        if(m_checkcount <= 0)
            m_selallinfo.checkstate = BST_UNCHECKED;
        else if(m_checkcount == m_row)
            m_selallinfo.checkstate = BST_CHECKED;
        else
            m_selallinfo.checkstate = BST_INDETERMINATE;
    }
    else
    {
        m_selallinfo.checkstate = state;
        if(!GetOwnerData())
        {
            switch(m_selallinfo.checkstate)
            {
                case BST_CHECKED:
                    m_checkcount = m_row;
                    while(m_flip == wyFalse && pgvrownode)
	                {
                        pgvrownode->excheck = wyTrue;
                        pgvrownode = pgvrownode->pNext;
                    }
                    break;

                case BST_UNCHECKED:
                    m_checkcount = 0;
                    while(m_flip == wyFalse && pgvrownode)
	                {
                        pgvrownode->excheck = wyFalse;
                        pgvrownode = pgvrownode->pNext;
                    }
                    break;
            }
        }
        
    }
}

