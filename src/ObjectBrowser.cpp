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

#include "scintilla.h"
#include "MDIWindow.h"
#include "Global.h"
#include "ObjectBrowser.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "ExportMultiFormat.h"

#ifndef COMMUNITY
#include "pcre.h"
#endif

//#include "IndexManager.h"
#include "ExportMultiFormat.h"
#include "OtherDialogs.h"
#include "CopyDatabase.h"
//#include "RelationShipMgmt.h"
#include "PreferenceBase.h"
#include "ExportBatch.h"
#include "Global.h"
#include "CommonHelper.h"
#include "GUIHelper.h"

#ifndef COMMUNITY
#include "SCIFormatter.h"
#include "TabQueryBuilder.h"
#include "TabSchemaDesigner.h"
#include "DatabaseSearch.h"
//#include "htmlayout.h"
//#include "QueryAnalyzerBase.h"
//#include "htmlrender.h"
#endif

extern	PGLOBALS		pGlobals;

#define     DEF_NODE_BUFF_SIZE  64

#define		DROPDBMESSAGE(a,b)		{ a.Sprintf(_("Dropping database %s"), b); pGlobals->m_pcmainwin->AddTextInStatusBar(a.GetAsWideChar()); }
#define		CSVTUNNELIMPORTERROR	_(L"This option cannot be used in HTTP Tunneling mode. \
\n\nIf you directly want to upload a local CSV file to a Remote Server, you can use SQLyog's Migration Toolkit. \
\nThe ODBC driver that you could use is Microsoft Text Driver (*.txt, *.csv) ODBC driver that ships with the OS.")
#define    LOADXMLERROR    _(L"This option is not supported by your server. Minimum supported server is MySQL 5.5.0")
#define		HTTPNOTWORKINGOPTIONS	_(L"This option cannot be used in HTTP Tunneling mode.")
#define     TT_REMOVE_DB_FILTER     _(L"Remove database level filter")
#define     CUEBANNERTEXT           _(L"Filter (Ctrl+Shift+B)")


/*
void GetItemInfo__(HTREEITEM hitem, HWND hwnd)
{
	wyWChar		database[SIZE_512] = {0};
	TVITEM		tvi={0};

	tvi.mask		=	TVIF_TEXT;
	tvi.hItem		=	hitem;
	tvi.cchTextMax	=	((sizeof(database)-1)*2);
	tvi.pszText		=	database;

	VERIFY(TreeView_GetItem(hwnd, &tvi));

}
*/


CQueryObject::CQueryObject(HWND hwndparent, wyChar *filterdb)
{
    m_isenable = wyTrue;
	m_hwndparent = hwndparent;
	m_toexpand	 = wyFalse;
	m_seltype	 = -1;
	m_isrefresh  = wyFalse;
    m_nodebuffsize = DEF_NODE_BUFF_SIZE;
    m_expandednodes = (wyWChar**) calloc(sizeof(wyWChar*), m_nodebuffsize);
    m_nodecount     = 0;
	m_isrefresh		= wyFalse;
    
	m_filterdb.SetAs(filterdb);

	m_dragged		= wyFalse;	
	m_isexpcollapse = wyFalse;
	m_tablesmenusub = NULL;

	m_isnoderefresh = wyFalse;

	#ifndef COMMUNITY
	m_isRegexChecked = wyFalse;
	#endif

    m_hfont         = NULL;
    m_hwndFilter    = NULL;

    memset(m_strOrig, 0, sizeof(m_strOrig));
    m_isSelValid = wyFalse;
    m_isClearVisible = wyFalse;
    m_AllowRename = wyFalse;
    m_isBkSpcOrDelOrClick = wyFalse;
    m_startCaret = 0;
    m_endCaret = 0;
    m_OBFilterWorkInProgress = wyFalse;
}

CQueryObject::~CQueryObject()
{
    wyInt32 count = 0;

    if(m_expandednodes)
    {
        for(count = 0; count < m_nodecount; count++)
        {
            if(m_expandednodes[count])
                free(m_expandednodes[count]);
            m_expandednodes[count] = NULL;
        }

        if(m_expandednodes)
           free(m_expandednodes);
    }

    if(m_hfont)
		DeleteObject(m_hfont);

    if(m_himl)
        ImageList_Destroy(m_himl);

    m_expandednodes = NULL;
}

wyBool 
CQueryObject::Create()
{
	CreateObjectBrowser(m_hwndparent);
	return wyTrue;
}

// Function to create the object browser.
// This window is used to show details about database, tables, columns, fields etc in an easy to 
// understand tree format.
HWND
CQueryObject::CreateObjectBrowser(HWND hwndparent)
{
 	//wyUInt32    exstyles =	WS_EX_STATICEDGE;
	wyUInt32    styles   =  WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS |TVS_FULLROWSELECT/*| TVS_TRACKSELECT*/;
	HWND	    hwndobject, hwndFilter, hwndStaticParent = NULL, hwndSt = NULL;
	#ifndef COMMUNITY
	HWND hwndRegex;
	HWND hwndRegexText;
	#endif

	COLORREF	backcolor, forecolor;
    /*HICON       hicon;
    TOOLINFO    tf;
    */
	SendMessage(hwndparent, WM_SETREDRAW, 0, FALSE);

	VERIFY(hwndStaticParent = CreateWindowEx(0, WC_STATIC, L"", WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN , 
                                        2, 2, 16, 16, hwndparent, (HMENU)-1, pGlobals->m_pcmainwin->GetHinstance(), this));
	
	VERIFY(hwndobject	= CreateWindowEx(0 , WC_TREEVIEW, NULL, styles, 5, 30, 150, 150,
									  hwndparent,(HMENU)IDC_OBJECTTREE, pGlobals->m_pcmainwin->GetHinstance(), this));

    m_hwnd	= hwndobject;
    
    //Set color for object browser
	if(pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor >= 0)
		backcolor  = pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor;
	else
		backcolor = COLOR_WHITE;

	if(pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor >= 0)
		forecolor  = pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor;
	else
		forecolor = backcolor ^ 0xFFFFFF;

	TreeView_SetBkColor(m_hwnd, backcolor);
	
    //Set the foreground color
	TreeView_SetTextColor(m_hwnd, forecolor);

    VERIFY(hwndSt = CreateWindowEx(0, WC_STATIC, _(L"Filter Databases"), WS_VISIBLE | WS_CHILD | SS_ENDELLIPSIS,
                                        2, 2, 16,16, hwndStaticParent, (HMENU)-1, pGlobals->m_pcmainwin->GetHinstance(), this));
    m_hwndStMsg    = hwndSt;
  VERIFY(hwndFilter	= CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_VISIBLE | WS_CHILD | ES_LEFT | WS_TABSTOP | ES_AUTOHSCROLL, 16, 2, 150, 30,
									  hwndStaticParent,(HMENU)IDC_OBJECTFILTER, pGlobals->m_pcmainwin->GetHinstance(), this));

#ifndef COMMUNITY
	VERIFY(hwndRegex	= CreateWindowEx(0, WC_BUTTON, NULL, WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_TABSTOP, 16, 30, 20, 28,
									  hwndStaticParent,(HMENU)IDC_REGEXFILTER, pGlobals->m_pcmainwin->GetHinstance(), this));
	VERIFY(hwndRegexText= CreateWindowEx(0, WC_STATIC, _(L"Search As Regex"), WS_VISIBLE | WS_CHILD | SS_ENDELLIPSIS | SS_NOTIFY, 16, 30, 120, 28,
									  hwndStaticParent, (HMENU)(IDC_REGEXFILTERTEXT), pGlobals->m_pcmainwin->GetHinstance(), this));
	//CheckDlgButton(hwndRegex, 1, BST_CHECKED);
#endif


   SetWindowPos(hwndobject, hwndFilter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
   SetWindowPos(hwndFilter, hwndStaticParent, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

   #ifndef COMMUNITY
   SetWindowPos(hwndobject, hwndRegex, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
   SetWindowPos(hwndobject, hwndRegexText, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
   m_hwndRegex = hwndRegex;
   m_hwndRegexText= hwndRegexText;
   #endif
   
   //ShowWindow(hwndFilter, SW_HIDE);
    
	m_hwndparent = hwndparent;
    m_hwndFilter =  hwndFilter;
    m_hwndStParent = hwndStaticParent;

	

    SendMessage(m_hwndFilter, EM_LIMITTEXT, 64, NULL);

    SetWindowLongPtr(m_hwndStParent, GWLP_USERDATA, (LONG_PTR)this);
    m_stWndProc = (WNDPROC)SetWindowLongPtr(m_hwndStParent, GWLP_WNDPROC, (LONG_PTR) CQueryObject::StCtrlProc);

    SetWindowLongPtr(m_hwndFilter, GWLP_USERDATA, (LONG_PTR)this);
    m_FilterProc = (WNDPROC)SetWindowLongPtr(m_hwndFilter, GWLP_WNDPROC, (LONG_PTR) CQueryObject::FilterProc);	

	//..Setting font
    SetFont();
    
	m_wporigproc = (WNDPROC)SetWindowLongPtr(hwndobject, GWLP_WNDPROC,(LONG_PTR)CQueryObject::WndProc);	
	SetWindowLongPtr(hwndobject, GWLP_USERDATA,(LONG_PTR)this);

	CreateImageList();
	return hwndobject;
}

void
CQueryObject::PaintFilterWindow(HWND hwnd)
{
    wyWChar str[70];
    RECT rect, temprect;
    HDC hdc = NULL;
    HBRUSH hbr = NULL;
    HICON hicon = NULL;

    memset(str, 0, sizeof(str));

    GetWindowText(hwnd, str, 64);
    GetWindowRect(hwnd, &rect);
    hdc = GetWindowDC(hwnd);
    
    hbr = IsWindowEnabled(hwnd)? GetSysColorBrush(COLOR_WINDOW): GetSysColorBrush(COLOR_BTNFACE);            
    
    rect.right -= rect.left;
    rect.bottom -= (rect.top + 2);
    rect.top = 2;
    temprect = rect;
    
    if(rect.right > 24)
    {
        rect.left = rect.right - 20;
        rect.right -= 2;
        FillRect(hdc, &rect, hbr);

        if(wcslen(str))
        {
            hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE ( IDI_REMOVE_FILTER ), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
            DrawIconEx(hdc, rect.left + 2, (rect.top+rect.bottom)/2 - 8, hicon, ICON_SIZE, ICON_SIZE, 0, NULL, DI_NORMAL);
            DestroyIcon(hicon);
            m_isClearVisible = wyTrue;
        }
    }
    else
    {
        m_isClearVisible = wyFalse;
    }

    ReleaseDC(hwnd, hdc);
}


LRESULT CALLBACK
CQueryObject::FilterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CQueryObject    *pcqueryobject	= (CQueryObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HTREEITEM           hti = NULL, hti_child= NULL;
    NCCALCSIZE_PARAMS    *rct = NULL;
    RECT                *rect = NULL;
    LRESULT             ret;
    wyInt32             image = 0;
    MDIWindow           *wnd = NULL;
    DWORD               start = 0, end = 0;
    wyWChar             str[70];
    RECT                rectWnd;
    POINT               pt;
    wyInt32             textLen = 0;
    HWND                curHwnd = NULL;
    HDC                 hdc = NULL;

	/*
		#ifndef COMMUNITY
		wyWChar strTemp[70];
		#endif
	*/
	
    VERIFY(wnd = GetActiveWin());
    
    memset(str, 0, sizeof(str));
    
    switch(message)
	{
		case WM_NCCREATE:
			pcqueryobject->PaintFilterWindow(hwnd);
			break;
		
		case WM_NCHITTEST:
			{   
            GetWindowRect(hwnd, &rectWnd);
            
            if((rectWnd.right - rectWnd.left) > 24)
            {
                rectWnd.left = rectWnd.right - 20;
                rectWnd.top += 2;
                rectWnd.bottom -= 2;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);

                if(PtInRect(&rectWnd, pt))
                {
                    return HTCAPTION;
                }
            }
        }
        break;

    case WM_NCLBUTTONDOWN:
        {
            GetWindowRect(hwnd, &rectWnd);
            if((rectWnd.right - rectWnd.left) > 24 && pcqueryobject->m_isClearVisible)
            {
                rectWnd.left = rectWnd.right - 20;
                rectWnd.top += 2;
                rectWnd.bottom -= 2;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            
                if(PtInRect(&rectWnd, pt))
                {
				   if(pcqueryobject->HandleOnEscapeOBFilter())
                    {
                        SetFocus(pcqueryobject->m_hwndFilter);
                    }
                }
            }
        }
        return 0;
        break;
    case WM_NCLBUTTONDBLCLK:
        return 0;
        break;
    case WM_LBUTTONDOWN:
        if(pcqueryobject->m_isSelValid)
        {
            pcqueryobject->m_isBkSpcOrDelOrClick = wyTrue;
            pcqueryobject->HandleOBFilter();
            pcqueryobject->m_isBkSpcOrDelOrClick = wyFalse;
        }
        pcqueryobject->m_isSelValid = wyFalse;
        break;
    case WM_NCCALCSIZE:
        if(wParam == TRUE)
        {
            rct = (NCCALCSIZE_PARAMS *)lParam;
            rct->rgrc[2] = rct->rgrc[1];
            rct->rgrc[1] = rct->rgrc[0];
            
            rct->rgrc[0].top += 2;
            rct->rgrc[0].bottom -=2;
            rct->rgrc[0].right += rct->rgrc[0].right > 28? -20: -2; 
            rct->rgrc[0].left += 2;
            
            return WVR_REDRAW;
        }
        else
        {
            rect = (RECT *)lParam;
            rect->top += 2;
            rect->bottom -= 2;
            rect->right += rect->right > 28? -20: -2;
            rect->left += 2;
            return 0;
        }
        break;
    case WM_NCPAINT:
        {   
            ret = CallWindowProc(pcqueryobject->m_FilterProc, hwnd, message, wParam, lParam);
            pcqueryobject->PaintFilterWindow(hwnd);
            return 0;
        }
        break;
    case WM_PAINT:
        {
            textLen = SendMessage(hwnd, WM_GETTEXTLENGTH, NULL, NULL);
            curHwnd = GetFocus();

            if(textLen > 0 || curHwnd == hwnd)
            {
                ret = CallWindowProc(pcqueryobject->m_FilterProc, hwnd, message, wParam, lParam);
            }
            else
            {
                ret = CallWindowProc(pcqueryobject->m_FilterProc, hwnd, message, wParam, lParam);
                GetClientRect(hwnd, &rectWnd);
                rectWnd.left += 2;
                rectWnd.right -= 2;
                rectWnd.top += 1;
                rectWnd.bottom -= 2;
                hdc = GetDC(hwnd);
                SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                SetBkColor(hdc, IsWindowEnabled(hwnd)? GetSysColor(COLOR_WINDOW): GetSysColor(COLOR_BTNFACE));
                if(pcqueryobject->m_hfont)
                    SelectObject(hdc, (HGDIOBJ)pcqueryobject->m_hfont);
                DrawText(hdc, CUEBANNERTEXT, -1, &rectWnd, DT_SINGLELINE | DT_END_ELLIPSIS | DT_LEFT);
                ReleaseDC(hwnd, hdc);
            }
            return 0;
        }
        break;
    case WM_KILLFOCUS:
        if(pcqueryobject->m_isSelValid)
            {
                GetWindowText(pcqueryobject->m_hwndFilter, str, 64);
                SendMessage(pcqueryobject->m_hwndFilter, EM_GETSEL, (WPARAM)&start,(LPARAM)&end);
                str[start] = '\0';
                pcqueryobject->m_isSelValid = wyFalse;
                pcqueryobject->HandleOBFilter(str, wyFalse);
                SetWindowText(pcqueryobject->m_hwndFilter, str);
                SendMessage(pcqueryobject->m_hwndFilter, EM_SETSEL, start, -1); 
            }
        break;
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_RETURN:
			#ifndef COMMUNITY
			if(pcqueryobject->m_isRegexChecked==wyTrue)
			{

				//GetWindowText(pcqueryobject->m_hwndFilter, strTemp, 65);
				//pcqueryobject->HandleOnEscapeOBFilter();
				pcqueryobject->HandleOBFilter(L"", wyFalse);
				//SetWindowText(pcqueryobject->m_hwndFilter,strTemp);
				//CEdit* e = (CEdit*)GetDlgItem();
				//SetFocus(pcqueryobject->m_hwndFilter);
				pcqueryobject->HandleOBFilter();
				break;
			}
			#endif
            if(pcqueryobject->m_isSelValid)
            {
                pcqueryobject->HandleOBFilter();
                pcqueryobject->m_isSelValid= wyFalse;
            }
            hti = TreeView_GetSelection(pcqueryobject->m_hwnd);
            image = pcqueryobject->GetSelectionImage();

            if(hti)
            {
                switch(image)
                {
                case NTABLES:
                case NSP:
                case NFUNC:
                case NEVENTS:
                case NTRIGGER:
                case NVIEWS:
                    hti_child = TreeView_GetChild(pcqueryobject->m_hwnd, hti);
                    if(hti_child)
                        TreeView_SelectItem(pcqueryobject->m_hwnd, hti_child);
                break;
                case NSERVER:
                    hti_child = TreeView_GetChild(pcqueryobject->m_hwnd, hti);
                    if(hti_child)
                    {
                        TreeView_Expand(pcqueryobject->m_hwnd, hti_child, TVE_EXPAND);
                        hti_child = TreeView_GetChild(pcqueryobject->m_hwnd, hti_child);
                        TreeView_SelectItem(pcqueryobject->m_hwnd, hti_child);
                    }
                    break;
                }
            }

            SetFocus(pcqueryobject->m_hwnd);
            return 0;
            break;
        case VK_DOWN:
        case VK_TAB:
            if(pcqueryobject->m_isSelValid)
            {
                GetWindowText(pcqueryobject->m_hwndFilter, str, 64);
                SendMessage(pcqueryobject->m_hwndFilter, EM_GETSEL, (WPARAM)&start,(LPARAM)&end);
                str[start] = '\0';
                pcqueryobject->m_isSelValid = wyFalse;
                pcqueryobject->m_prevString.SetAs(str);
                SetWindowText(pcqueryobject->m_hwndFilter, str);
                SendMessage(pcqueryobject->m_hwndFilter, EM_SETSEL, start, -1); 
            }

            hti = TreeView_GetSelection(pcqueryobject->m_hwnd);
            image = pcqueryobject->GetSelectionImage();

            if(hti)
            {
                switch(image)
                {
                case NTABLES:
                case NSP:
                case NFUNC:
                case NEVENTS:
                case NTRIGGER:
                case NVIEWS:
                case NSERVER:
                    hti_child = TreeView_GetChild(pcqueryobject->m_hwnd, hti);
                    if(hti_child)
                        TreeView_SelectItem(pcqueryobject->m_hwnd, hti_child);
                break;
                }
            }
            SetFocus(pcqueryobject->m_hwnd);
            break;
        case VK_ESCAPE:
            if(pcqueryobject->HandleOnEscapeOBFilter())
            {
                SetFocus(pcqueryobject->m_hwndFilter);
            }
			pcqueryobject->PaintFilterWindow(pcqueryobject->m_hwndFilter);
            break;
		case VK_DELETE:
        case VK_BACK:
            GetWindowText(pcqueryobject->m_hwndFilter, str, 64);

            SendMessage(pcqueryobject->m_hwndFilter, EM_GETSEL, (WPARAM)&start,(LPARAM)&end);
            if(wcslen(str) && wcslen(str) == (end-start))
            {
                if(pcqueryobject->HandleOnEscapeOBFilter())
                {
                    SetFocus(hwnd);
                    return 0;
                }
            }
            break;
        }

        if(wParam == VK_BACK || wParam == VK_DELETE)
        {
            pcqueryobject->m_isBkSpcOrDelOrClick = wyTrue;
        }
        else
        {
            pcqueryobject->m_isBkSpcOrDelOrClick = wyFalse;
        }

        if(pcqueryobject->m_isSelValid && (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP))
        {
            SendMessage(pcqueryobject->m_hwndFilter, EM_GETSEL,(WPARAM)&start,(LPARAM)&end);
            switch(wParam)
            {
            case VK_LEFT:
            case VK_UP:
                pcqueryobject->m_startCaret = start;
                pcqueryobject->m_endCaret = 0;
                break;
            case VK_RIGHT:
                pcqueryobject->m_startCaret = 0;
                pcqueryobject->m_endCaret = end;
                break;
            }

            SetTimer(pcqueryobject->m_hwndStParent, OBFILTERTIMER, 200, CQueryObject::OBFilterTimerProc);
            
            pcqueryobject->m_isSelValid = wyFalse;
            return 0;
        }
        else
        {
            pcqueryobject->m_startCaret = 0;
            pcqueryobject->m_endCaret = 0;
        }

        pcqueryobject->m_isSelValid = wyFalse;
        
        break;
    }
    return CallWindowProc(pcqueryobject->m_FilterProc, hwnd, message, wParam, lParam);
}

void CALLBACK 
CQueryObject::OBFilterTimerProc(HWND hwnd, UINT message, UINT_PTR id, DWORD time)
{    
    CQueryObject    *pcqueryobject	= (CQueryObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    KillTimer(hwnd, id);
#ifndef COMMUNITY
	wyWChar strTemp[10];
	if(pcqueryobject->m_isRegexChecked==wyTrue)
	{
		GetWindowText(pcqueryobject->m_hwndFilter, strTemp, 10);
		if(!strTemp[0] )
		{	
			pcqueryobject->HandleOnEscapeOBFilter();
			 pcqueryobject->HandleOBFilter();
			return;
		}
		else
			return;
	}
#endif
    if(!(pcqueryobject->m_isSelValid))
        pcqueryobject->HandleOBFilter();
}


LRESULT CALLBACK
CQueryObject::StCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CQueryObject    *pcqueryobject	= (CQueryObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HDC             hdc = NULL;
    /*HBRUSH          hbr = NULL;*/
    MDIWindow       *wnd = NULL;
    wyWChar         str[70];
/*
#ifndef COMMUNITY
	wyWChar			 strTemp[70];
#endif
*/

/*HTREEITEM       hti = NULL, htiSel = NULL;*/
    
    memset(str, 0, sizeof(str));

    VERIFY(wnd = GetActiveWin());

    switch(message)
	{
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            case IDC_OBJECTFILTER:
                switch(HIWORD(wParam))
                {
                    case EN_UPDATE:
						if(wnd->m_executing == wyFalse && !(pcqueryobject->m_isSelValid))
                            {
                                SetTimer(hwnd, OBFILTERTIMER, 200, CQueryObject::OBFilterTimerProc);
							}
#ifndef COMMUNITY	
						pcqueryobject->PaintFilterWindow(pcqueryobject->m_hwndFilter);
						SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif
                    break;
                }
            break;
#ifndef COMMUNITY
			case IDC_REGEXFILTER:
				switch(HIWORD(wParam))
				{
				case BN_CLICKED:
				BOOL checked = IsDlgButtonChecked(pcqueryobject->m_hwndStParent, IDC_REGEXFILTER) ;
				/*BOOL checked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);*/
				SetFocus(pcqueryobject->m_hwndFilter);
				if (checked)
					{
						pcqueryobject->m_isRegexChecked=wyFalse;
						CheckDlgButton(pcqueryobject->m_hwndStParent, IDC_REGEXFILTER, BST_UNCHECKED);
						pcqueryobject->HandleOnEscapeOBFilter();
						
					}
					else 
					{	
						
						pcqueryobject->m_isRegexChecked=wyTrue;
						CheckDlgButton(pcqueryobject->m_hwndStParent, IDC_REGEXFILTER, BST_CHECKED);
						//GetWindowText(pcqueryobject->m_hwndFilter, strTemp, 65);
						//pcqueryobject->HandleOnEscapeOBFilter();
						pcqueryobject->HandleOBFilter(L"", wyFalse);
						//SetWindowText(pcqueryobject->m_hwndFilter,strTemp);
						pcqueryobject->HandleOBFilter();
					}
				break;
				}
				break;
#endif
		}
		break;

    case WM_CTLCOLORSTATIC:
        if((HWND)lParam == pcqueryobject->m_hwndStMsg /*|| (HWND)lParam == pcqueryobject->m_hwndStClear*/)
        {
            hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, TreeView_GetTextColor(pcqueryobject->m_hwnd));
            SetDCBrushColor(hdc, TreeView_GetBkColor(pcqueryobject->m_hwnd));
            return (wyInt32)GetStockBrush(DC_BRUSH);
        }
#ifndef COMMUNITY
		if((HWND)lParam == pcqueryobject->m_hwndRegexText)
       {
			hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, TreeView_GetTextColor(pcqueryobject->m_hwnd));
            SetDCBrushColor(hdc, TreeView_GetBkColor(pcqueryobject->m_hwnd));
            return (wyInt32)GetStockBrush(DC_BRUSH);
        }
#endif

        break;

    }

    return CallWindowProc(pcqueryobject->m_stWndProc, pcqueryobject->m_hwndStParent, message, wParam, lParam);
}




wyBool
CQueryObject::SetFont()
{
    HDC			dc;
	wyUInt32    fontheight;
	wyInt32     ret;
	wyWChar     directory[MAX_PATH+1] = {0}, *lpfileport = 0;
	wyInt32		px, height, high, fontitalics;
	HDC         hdc = NULL;
	wyString	dirstr, fontnamestr;
    
	dc		= GetDC(GetParent(m_hwnd));

    fontheight = -MulDiv(9, GetDeviceCaps(dc, LOGPIXELSY), 75);
	
    VERIFY(ReleaseDC(GetParent(m_hwnd), dc));
	
    if(m_hfont)
		DeleteObject(m_hfont);

    m_hfont = NULL;

    // get the font name from the .ini file.
	// now search the font given in the .ini.
	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;
	
	dirstr.SetAs(directory);
    wyIni::IniGetString("OBFont", "FontName", "", &fontnamestr, dirstr.GetString());

    if(fontnamestr.GetLength())
    {
	    px = wyIni::IniGetInt("OBFont", "FontSize", 10, dirstr.GetString());
	    fontitalics = wyIni::IniGetInt("OBFont", "FontItalic", 0, dirstr.GetString());
	
	    VERIFY(hdc = GetDC(m_hwnd));
	    high = GetDeviceCaps(hdc, LOGPIXELSY);
	
	    height = (wyInt32)((- px) * high/ 72.0);
	    m_height = -(height);
	    VERIFY(m_hfont = CreateFont (height, 0, 0, 0, 0, fontitalics, 0, 0, 0, 0, 0, 0, 0, fontnamestr.GetAsWideChar()));
    }
	
    if(!m_hfont)
    {
        m_hfont = GetStockFont(DEFAULT_GUI_FONT);
        VERIFY(hdc = GetDC(m_hwnd));
	    high = GetDeviceCaps(hdc, LOGPIXELSY);
        LOGFONT lf;
        GetObject ( m_hfont, sizeof(LOGFONT), &lf );
        m_height = -lf.lfHeight;
    }

    SendMessage(m_hwnd, WM_SETFONT,(WPARAM)m_hfont, TRUE);
    SendMessage(m_hwndStMsg, WM_SETFONT, (WPARAM)m_hfont, TRUE);
    SendMessage(m_hwndFilter, WM_SETFONT, (WPARAM)m_hfont, TRUE);
#ifndef COMMUNITY
	SendMessage(m_hwndRegex,WM_SETFONT,(WPARAM)m_hfont,TRUE);
	SendMessage(m_hwndRegexText,WM_SETFONT,(WPARAM)m_hfont, TRUE);
#endif

    //SetWindowFont(m_hwnd, m_hfont, TRUE);
	UpdateWindow(m_hwnd);
    UpdateWindow(m_hwndFilter);

    if(hdc)
	ReleaseDC(m_hwnd, hdc);

    if(!pGlobals->m_isannouncementopen)
		Resize();
	else
		Resize(wyTrue);
    
    return wyTrue;
}

wyBool 
CQueryObject::IsHitOnItem(LPARAM lparam)
{
    TVHITTESTINFO tvhi = {0};

    tvhi.flags = TVHT_ONITEMLABEL;
    tvhi.pt.x = GET_X_LPARAM(lparam);
	tvhi.pt.y = GET_Y_LPARAM(lparam);

    if(TreeView_HitTest(m_hwnd, &tvhi))
    {
        return wyTrue;
    }

    return wyFalse;
}

wyBool
CQueryObject::HandleOnEscapeOBFilter()
{
    wyWChar     str[70] = {0};
    
    if(GetWindowText(m_hwndFilter, str, 64))
    {
        m_isSelValid = wyFalse;
        HandleOBFilter(L"", wyFalse);
        SetWindowText(m_hwndFilter, L"");
        return wyTrue;
    }

    return wyFalse;
}


// callback function for the object browser.
LRESULT	CALLBACK 
CQueryObject::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CQueryObject    *pcqueryobject	= (CQueryObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	MDIWindow	    *wnd;
    wyInt32         ret;
    	
    VERIFY(wnd = GetActiveWin());

    if(pcqueryobject)
    {
        if(pcqueryobject->m_isenable == wyFalse)
        {
            if((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) || (message >= WM_KEYFIRST && message <= WM_KEYLAST))
            {
                return 1;
            }
        }
    }

	switch(message)
	{   
        case WM_CHAR:
            if(IsCharAlphaNumeric((WCHAR)wparam))
            //if((WCHAR)wparam != 13 && (WCHAR)wparam != '\t')
            {
                wyWChar str[2];
                str[0] = (WCHAR)wparam;
                str[1] = '\0';
                pcqueryobject->m_isSelValid = wyFalse;
                SetWindowText(pcqueryobject->m_hwndFilter, str);
                SendMessage(pcqueryobject->m_hwndFilter, EM_SETSEL, 1, -1);
                SetFocus(pcqueryobject->m_hwndFilter);
                return 0;
            }
            break;
        case WM_ENABLE:
            if(pcqueryobject)
            {
                pcqueryobject->m_isenable = wparam ? wyTrue : wyFalse;
                EnableWindow(pcqueryobject->m_hwndFilter, wparam);
                pcqueryobject->PaintFilterWindow(pcqueryobject->m_hwndFilter);
                return 1;
            }
            break;

		case WM_DESTROY:
			ImageList_Destroy(pcqueryobject->m_himl);
            pcqueryobject->DeAllocateLParam(NULL, NSERVER);
			break;

		case WM_LBUTTONDBLCLK:
            if(pcqueryobject->IsHitOnItem(lparam) == wyTrue && IsInsertTextOnDBClick())
            {
                if(pcqueryobject->ProcessTable() == wyTrue)
                {
                    return 1;
                }

				if(pcqueryobject->InsertNodeText() == wyTrue)
                {
                    return 1;
                }
            }          
            else
            {
                ret = GetKeyState(VK_CONTROL);
                if(ret & 0x8000)
                    pcqueryobject->ShowTable(wyTrue);
                else
                    pcqueryobject->ShowTable(wyFalse);
            }
			break;
				
		case WM_KEYUP:
			if(wparam == VK_APPS)
				SendMessage(hwnd, WM_CUSTCONTEXTMENU, 0, -1); 
				break;
		
		//In windows it will not send WM_CONTEXTMENU for rightbutton click. But wine will send WM_CONTEXTMENU for right button click.
		//Because of this .we are explicitly sending WM_CUSTCONTEXTMENU for right button click in Treeview Control.
		case WM_CUSTCONTEXTMENU:
			// we change focus to the parent of this object browser.
			SetFocus(GetParent(hwnd));
			pcqueryobject->m_isrefresh = wyTrue;
			pcqueryobject->OnContextMenu(lparam);
			pcqueryobject->m_isrefresh = wyFalse;
			
			//post 8.01
			//InvalidateRect(pcqueryobject->m_hwndparent, NULL, FALSE);
			break;

		case WM_MOUSEMOVE:
					
			ImageList_DragMove(LOWORD(lparam), HIWORD(lparam));  
			if ((wparam & MK_LBUTTON)  && (pcqueryobject->m_dragged == wyTrue))
			{							
				pcqueryobject->OnBeginDrag(lparam);								
				if(wnd->m_dragged == wyTrue)
				{		
					ImageList_DragMove(LOWORD(lparam), HIWORD(lparam));
					pcqueryobject->m_dragged =  wyFalse;
				}
			}

			break;

        case WM_LBUTTONDOWN:

#ifndef COMMUNITY
			{

				TabQueryBuilder		* ptabqb;
				wyInt32				  imageid;

				/// For closing the grid listbox if the tab is Query Builder
				imageid = wnd->m_pctabmodule->GetActiveTabImage();
	           
				if(imageid == IDI_QUERYBUILDER_16)
				{
					ptabqb = (TabQueryBuilder*)wnd->m_pctabmodule->GetActiveTabType();
					CustomGrid_ApplyChanges(ptabqb->m_hwndgrid, wyTrue);
				}
			}
#endif
			/*if(wnd->GetActiveTabEditor())
				CustomGrid_ApplyChanges(wnd->GetActiveTabEditor()->m_pctabmgmt->m_insert->m_hwndgrid, wyTrue);	*/
                        //For just lbutton down make this flag as wyFalse
			pcqueryobject->m_isexpcollapse = wyFalse;
            break;

		case WM_MOUSELEAVE:
			ImageList_DragLeave(hwnd);   
			ImageList_EndDrag();
			break;

		case WM_LBUTTONUP:
            //Dropping dragged Table image : for QB
			pcqueryobject->OnDropTable();
			break;

		case UM_REEDITLABEL:
			SetFocus(pcqueryobject->m_hwnd);
			TreeView_EditLabel(pcqueryobject->m_hwnd, (HTREEITEM)lparam);
			break;
		
		case WM_HELP:
			ShowHelp("http://sqlyogkb.webyog.com/article/39-object-browser");
			return 1;

		case WM_KEYDOWN:
			{
				if(wnd->m_executing == wyTrue || wnd->m_pingexecuting == wyTrue)
					break;

				switch(wparam)
				{
                case VK_UP:
                    ret = GetKeyState(VK_CONTROL);
                    if(ret & 0x8000)
                    {
                        TreeView_SelectItem(pcqueryobject->m_hwnd, TreeView_GetRoot(pcqueryobject->m_hwnd));
                        SetFocus(pcqueryobject->m_hwnd);
                        return 0;
                    }
                    break;
                case VK_ESCAPE:
                    if(pcqueryobject->HandleOnEscapeOBFilter())
                    {
                        SetFocus(pcqueryobject->m_hwnd);
                    }
#ifndef COMMUNITY
					pcqueryobject->PaintFilterWindow(pcqueryobject->m_hwndFilter);
#endif
                    break;
				case VK_INSERT:
					pcqueryobject->ProcessInsert();
					break;

				case VK_DELETE:
					{
						wyInt32 ret;
						ret = GetKeyState(VK_SHIFT);
						if(ret & 0x8000)
						{
							pcqueryobject->ProcessDelete(wyTrue);
						}
						else
							pcqueryobject->ProcessDelete(wyFalse);
					}
					break;

				case VK_F2:
					pcqueryobject->ProcessF2();
					break;	

				case VK_F6:
					pcqueryobject->ProcessF6();
					break;

				case VK_RETURN:
					{
                		wyInt32 ret, retctrl;
						ret = GetKeyState(VK_SHIFT);
						retctrl = GetKeyState(VK_CONTROL);
						if(ret & 0x8000)
							pcqueryobject->ProcessReturn(wyTrue);
						if(!wnd->GetActiveTabEditor())
							break;
						if(retctrl & 0x8000)
                            wnd->m_pctabmodule->CreateTabDataTab(wnd, wyFalse, wyTrue);
						else
							pcqueryobject->ProcessReturn(wyFalse);
					}
					break;
                case VK_TAB:
                    pcqueryobject->m_isSelValid = wyFalse;
                    SetFocus(pcqueryobject->m_hwndFilter);
                    SendMessage(pcqueryobject->m_hwndFilter, EM_SETSEL, 0, -1);
                    break;

				case VK_SUBTRACT:
				case VK_OEM_MINUS:
					wyInt32 ret;
					ret = GetKeyState(VK_SHIFT);
					if (ret & 0x8000)
						pGlobals->m_pcmainwin->HandleOnCollapse(wnd);
					break;
                }
			}
			break;

		case WM_COMMAND:
			pcqueryobject->OnWmCommand(wparam);					
			break;
			
		case UM_FOCUS:
			SetFocus(pcqueryobject->m_hwnd);
			break;
        //case UM_FILTERTREE:
        //    {
        //        wyWChar str[70];
        //        GetWindowText(pcqueryobject->m_hwndFilter, str, 65);
        //        SendMessage(pcqueryobject->m_hwnd, WM_SETREDRAW, FALSE, 0);
        //        //LockWindowUpdate(pcqueryobject->m_hwnd);
        //        pcqueryobject->FilterTreeView(NULL, str);
        //        //LockWindowUpdate(NULL);
        //        //InvalidateRect(pcqueryobject->m_hwnd,NULL, TRUE);
        //        //UpdateWindow(pcqueryobject->m_hwnd);
        //        SendMessage(pcqueryobject->m_hwnd, WM_SETREDRAW, TRUE, 0);
        //        
        //    }
        //    break;
		case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);
			
		case WM_MEASUREITEM:
			return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, pcqueryobject->m_tablesmenusub);
	}
	return CallWindowProc(pcqueryobject->m_wporigproc, hwnd, message, wparam, lparam);
}

HWND
CQueryObject::GetHwnd()
{
	return m_hwnd;
}

// function to resize the object browser. It resizes itself with respect to the position of
// the verical, horizontal splitter and the status bar at the bottom.

void
CQueryObject::Resize(wyBool isannouncements, wyBool isstart)
{
	MDIWindow			*wnd			= (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	FrameWindowSplitter		*pcqueryvsplitter	= wnd->GetVSplitter();

	RECT                rcmain, rcvsplitter;
	wyInt32             ret;
	wyInt32             hpos, vpos, width, height, widthEditFilter, htmlh;
//	wyInt32				widthRegexFilter;
	
	if(isannouncements && wnd->m_isanncreate == wyFalse)
		pcqueryvsplitter->Resize(wyTrue);

	VERIFY(GetClientRect(m_hwndparent, &rcmain));
	VERIFY(GetWindowRect(pcqueryvsplitter->GetHwnd(), &rcvsplitter));
	VERIFY(MapWindowPoints(NULL, m_hwndparent,(LPPOINT)&rcvsplitter, 2));
	
	hpos			=	2;
	vpos			=	(m_height + 10) > 22 ? m_height + 10: 22;
	width			=	rcvsplitter.left-2;
    widthEditFilter =   (width - 26) > 0 ? (width - 26) : 0;
	
	//widthRegexFilter=	width < 200 ? width : (width-250);
	//if(widthRegexFilter<=220)
		//widthRegexFilter=150;

#ifndef COMMUNITY
	height			=	rcmain.bottom - (2 * vpos)-vpos-9;
#else
	height			=	rcmain.bottom - (2 * vpos) - 10;
#endif
    htmlh			=   (wyInt32)((0.3 * height));
	htmlh+=2;

#ifndef COMMUNITY
    MoveWindow(m_hwndStParent, 2, 2, width, vpos * 3 + 7, TRUE);
    MoveWindow(m_hwndStMsg, 6, 4, width, vpos - 5, TRUE); 
	//MoveWindow(m_hwndStMsg, 6, 4, width - 28, vpos - 5, TRUE);
    MoveWindow(m_hwndFilter, 2, vpos, widthEditFilter+22, vpos - 2, TRUE); 
	//MoveWindow(m_hwndRegex, 2, (vpos*2)+5, widthEditFilter-1, vpos-2, TRUE );
	MoveWindow(m_hwndRegex, 2, (vpos*2)+1, 20, vpos+1, TRUE );
	MoveWindow(m_hwndRegexText,24,(vpos*2)+1+5, widthEditFilter-22, vpos-2, TRUE);
#else
    MoveWindow(m_hwndStParent, 2, 2, width, vpos * 2 + 6, TRUE);
    MoveWindow(m_hwndStMsg, 6, 4, width - 28, vpos - 5, TRUE);
    MoveWindow(m_hwndFilter, 2, vpos, widthEditFilter + 22, vpos - 2, TRUE);
#endif
    
	if(!isannouncements)
#ifndef COMMUNITY
		VERIFY(ret = MoveWindow(m_hwnd, hpos, (vpos * 3) + 7, width, height, TRUE));
#else
		VERIFY(ret = MoveWindow(m_hwnd, hpos, (vpos * 2) + 8, width, height, TRUE));
#endif
	else
#ifndef COMMUNITY
		VERIFY(ret = MoveWindow(m_hwnd, hpos, (vpos * 3) + 10, width, height - htmlh, TRUE));
#else
		VERIFY(ret = MoveWindow(m_hwnd, hpos, (vpos * 2) + 8, width, height - htmlh, TRUE));
#endif
		
#ifndef COMMUNITY
	InvalidateRect(m_hwndRegex, NULL, TRUE);
	InvalidateRect(m_hwndRegexText, NULL, TRUE);
#endif		
    InvalidateRect(m_hwndStMsg, NULL, TRUE);
	InvalidateRect(m_hwndFilter, NULL, TRUE);
    InvalidateRect(m_hwnd, NULL, TRUE);
   
	_ASSERT(ret != 0);
}

// function creates imagelist for the various items in the treeview.
void
CQueryObject::CreateImageList()
{
	wyInt32     i, j;
	HICON		icon;
    wyInt32     imgres[] = { IDI_SERVER, IDI_DATABASE, IDI_COLUMN, IDI_TABLE, IDI_COLUMN, 
		                     IDI_INDEX, IDI_PROCEDURE,IDI_PROCEDURE, IDI_FUNCTION, IDI_FUNCTION, 
							 IDI_VIEW, IDI_VIEW, IDI_TRIGGER, IDI_TRIGGER, IDI_TABLE, IDI_EVENT, IDI_EVENT, IDI_PRIMARYKEY, IDI_PRIMARYKEY, IDI_OVERLAYFILTER};

	VERIFY(m_himl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK , 6, 0));
    
	for(j=0; j<(sizeof(imgres)/sizeof(imgres[0])); j++)
	{
		VERIFY(icon =(HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(imgres[j]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
		VERIFY(i = ImageList_AddIcon(m_himl, icon)!= -1);
		VERIFY(DestroyIcon(icon));
	}

    VERIFY(ImageList_SetOverlayImage(m_himl, 19, 1) != 0);
	
	TreeView_SetImageList(m_hwnd, m_himl, TVSIL_NORMAL);
    
    return;
}

// This function is called to refresh object browser. This function is called also when the data
// is added for the first time.
// Basically it executes various SQL queries to get data about the server.
void
CQueryObject::RefreshObjectBrowser(Tunnel * tunnel, PMYSQL mysql,MDIWindow* wnd)
{
	wyBool              ret;
	wyString            server, oldserver;
	TVITEM				tvi;
	HCURSOR				hcursornew;
	HTREEITEM			hserver, hti = NULL;
	TVINSERTSTRUCT		tvins;
	//MDIWindow*			wnd;
	wyWChar				*tempwchar = {0};
	wyUInt32			length = 1;
    wyWChar             filterText[70];
    
	
    memset(filterText, 0, sizeof(filterText));

	_ASSERT(mysql != NULL);

	//VERIFY(wnd = GetActiveWin());
		
	hcursornew	= LoadCursor(NULL, IDC_WAIT);
	
	wnd->m_pcquerystatus->ShowInformation(_(L" Refreshing ObjectBrowser"));
	wnd->m_statusbartext.SetAs(_(L" Refreshing ObjectBrowser"));

	GetOldDatabase(oldserver);

	m_isrefresh = wyTrue;

	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);
	
    hti = TreeView_GetRoot(m_hwnd);

    if(hti != NULL)
    {
        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        TreeView_GetItem(m_hwnd, &tvi);
        if(tvi.lParam != NULL)
        {
            wcscpy(filterText, ((OBDltElementParam *)tvi.lParam)->m_filterText);
        }
    }

    DeAllocateLParam(NULL, NSERVER);
    
    m_prevString.SetAs("");

    // Delete everything.
	TreeView_DeleteAllItems(m_hwnd);
	
	// Create the server name.
	if(!tunnel->IsTunnel())
    { 
		if(wnd->m_conninfo.m_isssh)
        {
			server.Sprintf("%s@%s", (*mysql)->user, wnd->m_conninfo.m_sshhost.GetString());
		} 
		else 
        {
			server.Sprintf("%s@%s",(*mysql)->user, (strlen((*mysql)->host)!= 0)?(*mysql)->host : "localhost");
		}
	} 
	else 
    {
		server.Sprintf("%s@%s", tunnel->GetUser(), (strlen(tunnel->GetHost())!= 0)? tunnel->GetHost(): "localhost");
	}

	tempwchar = server.GetAsWideChar(&length);

    memset(&tvi, 0, sizeof(TVITEM));
	tvi.mask			= TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
	tvi.pszText			= tempwchar;
	tvi.cchTextMax		= length;
	tvi.iImage			= NSERVER;
	tvi.iSelectedImage	= NSERVER;
    
	tvins.hParent       = TVI_ROOT;
	tvins.hInsertAfter	= TVI_FIRST;
	tvins.item			= tvi;

	// Add it as the root element.
	VERIFY(hserver = TreeView_InsertItem(m_hwnd, &tvins));
	ret = InsertDatabases(tunnel, mysql, hserver);

	if(ret == wyFalse)	
    {
		yog_message(m_hwnd, _(L"Could not Refresh ObjectBrowser"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
		m_isrefresh = wyFalse;
		return;
	}

	hcursornew	= LoadCursor(NULL, IDC_ARROW);
	SetOldDatabase(tunnel, mysql, oldserver);
	wnd->m_pcquerystatus->ShowInformation(_(L" ObjectBrowser Refreshed"));
	wnd->m_statusbartext.SetAs(_(L" ObjectBrowser Refreshed"));

	TreeView_SelectItem(m_hwnd, hserver);
	TreeView_Expand(m_hwnd, hserver, TVE_EXPAND);
		
	//Sorting the items
	TreeView_SortChildren(m_hwnd, hserver, TRUE);

	SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
	m_isrefresh = wyFalse;

    m_prevString.SetAs(L"");
    HandleOBFilter(filterText, wyFalse);

	return;
}

void
CQueryObject::CollapseObjectBrowser(HWND hTree)
{
	HTREEITEM	 hti = NULL;

	hti = TreeView_GetRoot(m_hwnd);

	if (TreeView_GetChild(m_hwnd, hti) != NULL)
	{
		//	TreeView_Expand(m_hwnd, hti, TVE_COLLAPSE);
		hti = TreeView_GetChild(m_hwnd, hti);
		do
		{
			CollapseNode(m_hwnd, hti);
		} while ((hti = TreeView_GetNextSibling(m_hwnd, hti)) != NULL);
	}
}

void
CQueryObject::CollapseNode(HWND hTree, HTREEITEM hti)
{
	if (TreeView_GetChild(hTree, hti) != NULL)
	{
		TreeView_Expand(hTree, hti, TVE_COLLAPSE);
		hti = TreeView_GetChild(hTree, hti);
		do
		{
			CollapseNode(hTree, hti);
		} while ((hti = TreeView_GetNextSibling(hTree, hti)) != NULL);
	}
}

LRESULT
CQueryObject::GetOldDatabase(wyString &olddb)
{
	wyInt32         Index;
	LRESULT			ret;
	COMBOBOXEXITEM	cbi;
    wyWChar         olddbname[SIZE_512] = {0};

	VERIFY(Index	=	SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CB_GETCURSEL, 0, 0)!= CB_ERR);

	memset(&cbi, 0, sizeof(COMBOBOXEXITEM));

	cbi.mask		=	CBEIF_TEXT;
	cbi.iItem		=	Index;
	cbi.pszText		=	olddbname;
	cbi.cchTextMax	=	SIZE_512 - 1;

	ret = SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CBEM_GETITEM, 0,(LPARAM)&cbi);

    olddb.SetAs(olddbname);

	return ret;
}

// Function to add all the databases as child of the root element,
wyBool
CQueryObject::InsertDatabases(Tunnel * tunnel, PMYSQL mysql, HTREEITEM hserver)
{
	wyInt32             dbcount = 0;
	wyString            query, myrowstr;
	wyBool				tosearch = wyFalse, ismysql41 = IsMySQL41(tunnel, mysql);
	HWND				hwnd = m_hwnd;
	HTREEITEM			hdatabase;
	MDIWindow			*pcquerywnd;

	MYSQL_RES			*myres=NULL;
	MYSQL_ROW			myrow;
	
	// now we get the databases to be filtered before it is added to teh object browser.
	if(m_filterdb.GetLength())
		tosearch = wyTrue;
    else 
		tosearch = wyFalse;

	if(tosearch == wyFalse)
    { 
		query.Sprintf("show databases");
		// execute query to get all the database names.

		SetCursor(LoadCursor(NULL, IDC_WAIT));

		VERIFY(pcquerywnd  = (MDIWindow *)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA));

        myres = ExecuteAndGetResult(pcquerywnd, tunnel, mysql, query);

		if(!myres)
		{
			ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
			goto cleanup;
		}
		
		if(tunnel->mysql_error(*mysql)[0] != '\0')
			goto cleanup;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if(tosearch == wyFalse)
    {
		while(myrow = tunnel->mysql_fetch_row(myres))	
		{
			myrowstr.SetAs(myrow[0], ismysql41);
			dbcount++;
			hdatabase = InsertNode(m_hwnd, hserver, myrowstr.GetAsWideChar(), NDATABASE, NDATABASE, 0);

			// Now add a dummy item so that we get a + arrow even if nothing is added.
			// dummy is added only in object browser but not in gds	
            InsertDummyNode(m_hwnd, hdatabase);
		}
	} 
    else 
        InsertFilterDatabases(hserver, m_filterdb);

	if(tosearch == wyFalse)
		tunnel->mysql_free_result(myres);

	return wyTrue;

cleanup:
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyFalse;
}
void 
CQueryObject::InsertFilterDatabases(HTREEITEM hserver, wyString &filterdb)
{
    wyWChar      *tempfilter, *filterdatabase;
    HTREEITEM   hdatabase;
	wyUInt32	length = 1;

	filterdb.GetAsWideChar(&length);
    tempfilter = AllocateBuffWChar(length + 1); 
	wcsncpy(tempfilter, filterdb.GetAsWideChar(), length);
	filterdatabase = wcstok(tempfilter, L";");

	while(filterdatabase)
	{
        hdatabase = InsertNode(m_hwnd, hserver, (wyWChar*)filterdatabase, NDATABASE, NDATABASE, 0);
		InsertDummyNode(m_hwnd, hdatabase);	
		filterdatabase = wcstok(NULL, L";");
	}
    
    free(tempfilter);
}

LRESULT
CQueryObject::SetOldDatabase(Tunnel * tunnel, PMYSQL mysql, wyString &olddb)
{
	wyString    query;
    MYSQL_RES	*res;

	if(!(stricmp(olddb.GetString(), _("No database selected"))))
		return 0;

	if(olddb.GetLength()== 0)
		return 0;

	query.Sprintf("use `%s`", olddb.GetString());

    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
	if(!res && tunnel->mysql_affected_rows(*mysql) == -1)
		return 0;

    if(res)
	    tunnel->mysql_free_result(res);

	tunnel->SetDB(olddb.GetString());
	return 1;
}

// Function to implement context menu of the object browser.
// It checks on which node the menu was selected and according to that it selects the menu.
// In this way we dont have any disabled menu items
wyBool
CQueryObject::OnContextMenu(LPARAM lParam)
{
    wyInt32         ret;
	POINT			pnt;
	TVITEM			tvi;
	wyInt32			incr=0;
	HTREEITEM		hitem, hprevitem;
	MDIWindow		*wnd;
	wyWChar			str[SIZE_256]={0};
    TabEditor       *ptabeditor = NULL;
   // EditorBase      *peditorbase = NULL;
	RECT			rect3;
	
	VERIFY(wnd = GetActiveWin());
	
	SetFocus(m_hwnd);
	hprevitem	= TreeView_GetSelection(m_hwnd);
	
	//Select an item and press context button.then lParam is -1.so we need to find the point of item selected in the treeview
	if(lParam == -1)
	{
		hitem = hprevitem;
		TreeView_GetItemRect(m_hwnd, hitem, &rect3, TRUE);

		pnt.x = (rect3.left + rect3.right)/2;
		pnt.y = rect3.bottom;

		MapWindowPoints(m_hwnd, NULL, &pnt, 1);
	}
	else
	{	
		//It returns screen points.
		pnt.x			= GET_X_LPARAM(lParam); 
		pnt.y			= GET_Y_LPARAM(lParam); 

		//for finding the item selected we are converting screen points to client area points.
		VERIFY(ret	= ScreenToClient(m_hwnd, &pnt));
		hitem		= SelectOnContext(&pnt);
		
        // for drawing the menu, we need screen points.
		VERIFY(ret	= ClientToScreen(m_hwnd, &pnt));
	}
	
    if(IsWindowMaximised() && wyTheme::IsSysmenuEnabled(wnd->m_hwnd))
		incr++;

	//VERIFY(ret = ClientToScreen(m_hwnd, &pnt));

	//Post 8.01
	/*if(wnd)
		UpdateWindow(wnd->m_pctabmodule->GetHwnd());
	
	SciRedraw(wnd, wyFalse);*/

    ptabeditor  = wnd->GetActiveTabEditor();
   
	//Post 8.01
	/*if(ptabeditor)
    {
        peditorbase = ptabeditor->m_peditorbase;
	    SendMessage(peditorbase->m_hwnd, WM_SETREDRAW, FALSE, FALSE);  
    }*/
	
	if(hitem != NULL)
	{
		tvi.mask	=	TVIF_IMAGE | TVIF_TEXT;
		tvi.pszText	=	str;
		tvi.cchTextMax = SIZE_256 - 1;
		tvi.hItem   =	hitem;

		TreeView_GetItem(m_hwnd, &tvi);
		
		// this will change the currently selected databases 
		m_isrefresh = wyFalse;

		if(hprevitem != hitem)
			OnSelChanged(hitem);

		m_isrefresh = wyTrue;

        ShowContextMenu(wnd, tvi.iImage, &pnt, incr, str);
       
		//post 8.01
		// RepaintTabModule();
	}

	//post 8.01
	/*SciRedraw(wnd, wyTrue);
	if(ptabeditor)
	{
	    SendMessage(peditorbase->m_hwnd, WM_SETREDRAW, TRUE, TRUE);
	    //InvalidateRect(peditorbase->m_hwnd, NULL, FALSE);
	    UpdateWindow(peditorbase->m_hwnd);
	}*/
	
	SetFocus(m_hwnd);

	//post 8.01
	//InvalidateRect(m_hwndparent, NULL, FALSE);

	return wyTrue;
}


void 
CQueryObject::ShowContextMenu(MDIWindow *wnd, wyInt32 image, POINT *pnt, wyInt32 incr, wyWChar *str)
{
    HMENU hmenu = NULL, htrackmenu, htrackmenu1;

    switch(image)
	{
	case NSERVER:
		VERIFY(hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_OTHEROPTION)));
        LocalizeMenu(hmenu);
		VERIFY(htrackmenu =	GetSubMenu(hmenu, 0));

		if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO)
		{
			RemoveMenu(htrackmenu, ID_IMPORT_EXTERNAL_DATA, MF_BYCOMMAND);
			RemoveMenu(htrackmenu, ID_POWERTOOLS_SCHEDULEEXPORT2, MF_BYCOMMAND);
			DrawMenuBar(m_hwnd);
		}

		if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO 
			|| pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
		{
			RemoveMenu(htrackmenu, ID_DATASEARCH, MF_BYCOMMAND);
			DrawMenuBar(m_hwnd);
		}

		ChangeMenuItemOnPref(hmenu, MNU_OTHEROPT_INDEX); //setting objectbrowser refresh menu on  switch F5 and F9 functionalitiesmenu based on 

		// Set menu draw property for drawing icon
		wyTheme::SetMenuItemOwnerDraw(htrackmenu);

		// Keeps for drwing icon
		m_tablesmenusub = htrackmenu;
		
		VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, m_hwnd, NULL));
        FreeMenuOwnerDrawItem(htrackmenu);
		VERIFY(DestroyMenu(hmenu));
		break;

	case NDATABASE:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUDB_INDEX - 1 +incr));
		SyncObjectDB(wnd);
		EnableDBItems(htrackmenu);

		//RemoveMenu(htrackmenu,ID_DATASEARCH, MF_BYCOMMAND);
		
		if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
			&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
			&& GetMenuItemID(htrackmenu, 3) != ID_DB_DATASEARCH)
		{
			InsertMenu(htrackmenu,3,MF_BYPOSITION, ID_DB_DATASEARCH, _(L"New Data &Search\tCtrl+Shift+D"));
            wyTheme::SetMenuItemOwnerDraw(htrackmenu);
            DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
		
			VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,  pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
        /*
			FreeMenuOwnerDrawItem(htrackmenu, 3);
			RemoveMenu(htrackmenu,ID_DATASEARCH, MF_BYCOMMAND);*/
		}
		else
		{
            wyTheme::SetMenuItemOwnerDraw(htrackmenu);
			VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,  pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		}


		break;

	case NTABLES:
		LoadTablesMenu(pnt);
		break;

	case NTABLE:
		LoadTableMenu(pnt, incr);       	
		break;

	case NPRIMARYKEY:
	case NCOLUMN:
        VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
        VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 0));
		EnableColumnItems(htrackmenu1);

		
		if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
			&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
			&& GetMenuItemID(htrackmenu1, 1) != ID_DATASEARCH)
		{
			InsertMenu(htrackmenu1,1,MF_BYPOSITION, ID_DATASEARCH, _(L"New Data Searc&h\tCtrl+Shift+D"));
			wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
            DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
				
			VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		}
		else
		{
            wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
			VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		}
        
		break;

	case NPRIMARYINDEX:
	case NINDEX:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
        VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 1));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	case NSP:
	case NSPITEM:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
		VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 4));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	case NFUNC:
	case NFUNCITEM:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
		VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 5));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	case NEVENTITEM:
	case NEVENTS:	
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
		VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 7));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	case NVIEWS:
	case NVIEWSITEM:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
		VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 3));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	case NTRIGGER:
	case NTRIGGERITEM:
		VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
		VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 6));
		EnableColumnItems(htrackmenu1);
        wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		break;

	/* starting from v4.1 we show a context menu on folder right click too */
	case NFOLDER:
		if(0 == wcsicmp(str, TXT_COLUMNS))
        {
            
            VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		    VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
            VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 0));
          	EnableColumnItems(htrackmenu1);
			/*FreeMenuOwnerDrawItem(htrackmenu1, 1);
			RemoveMenu(htrackmenu1,ID_DATASEARCH, MF_BYCOMMAND);*/
			
			if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
			&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
			&& GetMenuItemID(htrackmenu1, 1) != ID_DATASEARCH)
			{
				InsertMenu(htrackmenu1,1,MF_BYPOSITION, ID_DATASEARCH, _(L"New Data Searc&h\tCtrl+Shift+D"));
				wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
				DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
						
				VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
			}
			else
			{
                wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
				VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
			}

		} 
        else if(0 == wcsicmp(str, TXT_INDEXES))
        {
            VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
		    VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUOBJ_INDEX - 1 + incr));
            VERIFY(htrackmenu1 = GetSubMenu(htrackmenu, 1));
		    EnableColumnItems(htrackmenu1);
            wyTheme::SetMenuItemOwnerDraw(htrackmenu1);
		    VERIFY(TrackPopupMenu(htrackmenu1, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
		}

		break;
	}
}

// Loads the menu for the 'Tables'
void
CQueryObject::LoadTablesMenu(POINT *pnt)
{
	HMENU	hmenu, htrackmenu;

    VERIFY(hmenu      =	LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_TABLESMENU)));
    LocalizeMenu(hmenu);
	VERIFY(htrackmenu = GetSubMenu(hmenu, 0));
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO 
			|| pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
	{
			RemoveMenu(htrackmenu,ID_DATASEARCH, MF_BYCOMMAND);
	}
	
	// Set menu draw property for drawing icon
	wyTheme::SetMenuItemOwnerDraw(htrackmenu);
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableMenuItem(hmenu, ID_OPEN_COPYTABLE, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_TABLE_MAKER, MF_GRAYED | MF_BYCOMMAND);
	}
#endif

    VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));

	m_tablesmenusub = htrackmenu;
    FreeMenuOwnerDrawItem(htrackmenu);
   	VERIFY(DestroyMenu(hmenu));	
}

VOID
CQueryObject::LoadTableMenu(POINT *pnt, wyInt32 incr)
{
	HMENU	hmenu, htrackmenu;

	VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
	VERIFY(htrackmenu =	GetSubMenu(hmenu, MNUTBL_INDEX - 1 + incr));
	
	EnableTableItems(htrackmenu);
	/*if(GetMenuItemID(htrackmenu, 2) == ID_DATASEARCH)
	{
		FreeMenuOwnerDrawItem(htrackmenu, 2);
		RemoveMenu(htrackmenu,ID_DATASEARCH, MF_BYCOMMAND);
	}*/

	if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
		&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL
		&& GetMenuItemID(htrackmenu, 2) != ID_DATASEARCH)
	{
		InsertMenu(htrackmenu,2,MF_BYPOSITION, ID_DATASEARCH, _(L"New Data Searc&h\tCtrl+Shift+D"));
		wyTheme::SetMenuItemOwnerDraw(htrackmenu);
		DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
	
		VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON , 
			pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
	}
	else
	{
        wyTheme::SetMenuItemOwnerDraw(htrackmenu);
		VERIFY(TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON , 
		pnt->x, pnt->y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));
	}
}

// Function idetify whether the selected htreeitem of NFOLDER is index or column.
wyBool
CQueryObject::IsSelectedIndex(HTREEITEM hitem)
{
	TVITEM		tvi;
	wyWChar      text[SIZE_64]={0};

	memset(&tvi, 0, sizeof(TVITEM));

	tvi.mask		= TVIF_TEXT;
	tvi.pszText		= text;
	tvi.cchTextMax	= SIZE_64-1;
	tvi.hItem		= hitem;

	VERIFY(TreeView_GetItem(m_hwnd, &tvi));

	if(wcsicmp(text, L"Indexes") == 0)
		return wyTrue;
	else
		return wyFalse;
}

// Get in which item the mouse is selected.
HTREEITEM
CQueryObject::SelectOnContext(LPPOINT pnt)
{
	TVHITTESTINFO	tvht;
	HTREEITEM		hprevitem;

	tvht.pt		=	*(pnt);
	
	TreeView_HitTest(m_hwnd, &tvht);

	hprevitem = TreeView_GetSelection(m_hwnd);

	if(tvht.hItem == NULL)
		tvht.hItem	=	TreeView_GetRoot(m_hwnd);

	if(hprevitem != tvht.hItem)
		TreeView_SelectItem(m_hwnd, tvht.hItem);

	return tvht.hItem;
}

// Function to drop database when the user selects the option from the context menu of the 
// object browser.
wyBool
CQueryObject::DropDatabase(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32	        ret, item;
	wyString        query, message;
	HTREEITEM		hitem;
    MYSQL_RES		*res;
	MDIWindow		*wnd = GetActiveWin();
	wyInt32			isintransaction = 1;
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif

	item	= GetSelectionImage();
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	switch(item)
	{
	case NDATABASE:
		GetDatabaseName(hitem);
		break;

	case NTABLE:			// now we see which of the item was sent as id.
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetTableDatabaseName(hitem);
		break;

	case NPRIMARYINDEX:
	case NINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));		
		GetDatabaseName(hitem);
		break;
	}	

	message.Sprintf(_("Do you really want to drop the database (%s)?\n\nWarning: You will lose all data!"), m_seldatabase.GetString());

	ret	= yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 );

	switch(ret)
	{
	case IDYES:
		break;

	default:
		//Post 8.01
		//RepaintTabModule();
		return wyFalse;
	}

	DROPDBMESSAGE(message, m_seldatabase.GetString())
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// prepare drop database query
	query.Sprintf("drop database `%s`", m_seldatabase.GetString());

    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		message.Sprintf(_("Error dropping %s"), m_seldatabase.GetString()); 
		pGlobals->m_pcmainwin->AddTextInStatusBar(message.GetAsWideChar());		
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	tunnel->mysql_free_result(res);

	message.Sprintf(_("Dropping %s successful"), m_seldatabase.GetString()); 
	
	/* fixed a bug for issue id# . You need to call change db in combo */
    pGlobals->m_pcmainwin->ChangeDBInCombo(tunnel, mysql);
	pGlobals->m_pcmainwin->AddTextInStatusBar(message.GetAsWideChar());		
	if(wnd)
	{
		wnd->m_statusbartext.SetAs(message.GetAsWideChar());
	}

	TreeView_DeleteItem(m_hwnd, hitem);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

// Function to drop table when the user selects the option from the context menu of the 
// object browser.
wyBool
CQueryObject::DropTable(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32         ret, item;
	wyString        query, message;
	HTREEITEM		hitem, hitemdb = NULL;
    MYSQL_RES		*res;
	wyInt32 isintransaction = 1;

#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		hitemdb = GetTableDatabaseName(hitem);
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		hitemdb = GetTableDatabaseName(hitem);
		break;		

	case NTABLE:
		hitemdb = GetTableDatabaseName(hitem);
		break;

	default:
		return wyFalse;
	}

	if(!hitemdb)
		return wyFalse;
	
	message.Sprintf(_("Do you really want to drop the table (%s)?\n\nWarning: You will lose all data!"), m_seltable.GetString());

	ret = yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 );

	switch(ret)
	{
	case IDYES:
		break;
	default:
		return wyFalse;
	}


	// Set cursor to wait and lockwindow update for less flickering.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	// prepare drop table query.
	query.Sprintf("drop table `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());

    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	tunnel->mysql_free_result(res);

	TreeView_DeleteItem(m_hwnd, hitem);

	//Make sure all triggers for this dropped table got deleted
	ReorderTriggers(hitemdb);

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}

// Function to drop a field when the user selects the option from the context menu of the 
// object browser.
wyBool
CQueryObject::DropField(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32			ret, tabicon = 0, isintransaction = 1;
	wyString        query, message, fieldstr;
    wyWChar         field[SIZE_512] = {0};
	TVITEM			tvi;
	HTREEITEM		hitem = NULL, hitemparent = NULL, hDBNode;
    MYSQL_RES		*res;
    wyString        fieldname;
	HTREEITEM		sibitemprev, sibitemnext;
    MDIWindow   *wnd = GetActiveWin();
    TabObject	*pinfotab   = NULL;
	TabMgmt     *ptabmgmt   = NULL;
	TabEditor   *ptabeditor = NULL;
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));

	//Checks whether there is only one column in the table if yes cant delete that.		
	sibitemprev = TreeView_GetPrevSibling(m_hwnd, hitem);
	sibitemnext = TreeView_GetNextSibling(m_hwnd, hitem);
	
	if(!sibitemprev && !sibitemnext)
	{
		message.Sprintf(_("You can't delete all columns "));

		yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), 
			MB_ICONERROR | MB_OK);

		return wyFalse;
	}
	
	// now get the column name.
	tvi.mask        = TVIF_TEXT;
	tvi.hItem       = hitem;
	tvi.cchTextMax  = SIZE_512 - 1;
	tvi.pszText     = field;

	TreeView_GetItem(m_hwnd, &tvi);

	GetColumnName(field);
    fieldname.SetAs(field);

	// Get the database and the table name.
	GetTableDatabaseName(TreeView_GetParent(m_hwnd, TreeView_GetParent(m_hwnd, hitem)));

	// Confirmation message
	message.Sprintf(_("Do you really want to drop the column (%s)?\n\nWarning: You will lose all data for the column!"), 
                        fieldname.GetString());

	ret = yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

	switch(ret)
	{
	case IDYES:
		break;

	default:
		return wyFalse;
	}

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// prepare drop table query.
	query.Sprintf("alter table `%s`.`%s` drop `%s`", m_seldatabase.GetString(), m_seltable.GetString(), fieldname.GetString());
    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	hitemparent = TreeView_GetParent(m_hwnd, hitem);

	tunnel->mysql_free_result(res);
	TreeView_DeleteItem(m_hwnd, hitem);
	SetFocus(m_hwnd);
    
    hDBNode = GetDatabaseNode();
    
	RefreshIndexesOrColumnsFolder(m_hwnd, hitemparent, NFOLDER, tunnel, mysql, m_seldatabase.GetString(),
		m_seltable.GetString(), wyTrue, wyFalse);
    
    hitemparent = TreeView_GetChild(m_hwnd, GetTreeObjectItem(TreeView_GetChild(m_hwnd, GetNode(hDBNode, NTABLES)),m_seltable.GetAsWideChar()));

    if(hitemparent)
		TreeView_SelectItem(m_hwnd, hitemparent);

    tabicon = wnd->m_pctabmodule->GetActiveTabImage();

	if(tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_DATASEARCH && tabicon != IDI_TABLEINDEX)
	{
        ptabeditor = (TabEditor*)wnd->GetActiveTabEditor();
	}

	if(ptabeditor)
        ptabmgmt   = ptabeditor->m_pctabmgmt;

    if(tabicon == IDI_TABLEINDEX)
	{
	    pinfotab = (TabObject*)wnd->m_pctabmodule->GetActiveTabType();	
        if(pinfotab && pinfotab->m_pobjinfo)
		{						
			pinfotab->m_pobjinfo->Refresh(wyTrue);
		}
	}
	else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
    {
        ptabmgmt->m_pqueryobj->Refresh(wyTrue);
    }
    
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

// Function to drop a index when the user presses delete on a index item in the object
// browser.
wyBool
CQueryObject::DropIndex(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32			ret, indexcounter, tabicon = 0, isintransaction = 1;
	wyString        query, message, indexstr;
    wyWChar         index[SIZE_512] = {0}; 
	TVITEM			tvi;
	HTREEITEM		hitem, indexparent;
    MYSQL_RES		*res;
    MDIWindow   *wnd = GetActiveWin();
    TabObject	*pinfotab   = NULL;
	TabMgmt     *ptabmgmt   = NULL;
	TabEditor   *ptabeditor = NULL;
    
	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));

	// now get the indexname,
	tvi.mask        = TVIF_TEXT;
	tvi.hItem       = hitem;
	tvi.cchTextMax  = SIZE_512-1;
	tvi.pszText     = index;

	VERIFY(TreeView_GetItem(m_hwnd, &tvi));

    /// parse to get remove the table name and get the index name only.
    indexstr.SetAs(index);
    indexstr.SetAs(strtok((wyChar *)indexstr.GetString(), "("));
    /// Erase the one blank that appears in the index name.
    indexstr.Strip(1);

	// we remove the comma from the index name
	// now move till [ so that we get only the table name and strip all.
	for(indexcounter = 0; index[indexcounter] != 0; indexcounter++)
	{
		if(index[indexcounter] == C_COMMA)
			break;
	}

	message.Sprintf(_("Do you really want to drop the index (%s)?)"), indexstr.GetString());

	ret = yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

	switch(ret)
	{
	case IDYES:
		break;

	default:
		return wyFalse;
	}

	indexparent = TreeView_GetParent(m_hwnd, hitem);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// Get the database and the table name.
	GetTableDatabaseName(TreeView_GetParent(m_hwnd, indexparent));
	
	// prepare drop table query.
	query.Sprintf("alter table `%s`.`%s` drop index `%s`", m_seldatabase.GetString(), m_seltable.GetString(), indexstr.GetString());
		
    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
    	SetFocus(m_hwnd);
		return wyFalse;
	}

    MessageBox( m_hwnd, _(L"Index deleted successfully"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
	tunnel->mysql_free_result(res);
 
	TreeView_DeleteItem(m_hwnd, hitem);
	
    tabicon = wnd->m_pctabmodule->GetActiveTabImage();

	if(tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_DATASEARCH && tabicon != IDI_TABLEINDEX)
	{
        ptabeditor = (TabEditor*)wnd->GetActiveTabEditor();
	}

	if(ptabeditor)
        ptabmgmt   = ptabeditor->m_pctabmgmt;

    if(tabicon == IDI_TABLEINDEX)
	{
	    pinfotab = (TabObject*)wnd->m_pctabmodule->GetActiveTabType();	
        if(pinfotab && pinfotab->m_pobjinfo)
		{						
			pinfotab->m_pobjinfo->Refresh(wyTrue);
		}
	}
	else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
    {
        ptabmgmt->m_pqueryobj->Refresh(wyTrue);
    }

	SetCursor(LoadCursor(NULL, IDC_ARROW));
    SetFocus(m_hwnd);

	return wyFalse;
}

// Function to clear a table when the user selects the option from the context menu of the 
// object browser.
wyBool
CQueryObject::EmptyTable(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32		    ret,item, isintransaction = 1;
	wyString        query, message;
	HTREEITEM		hitem;
    MYSQL_RES*      res;
	MDIWindow*		wnd;
	TabEditor*      ptabeditor = NULL;
    TabTableData*   ptabdata = NULL;

	wnd = GetActiveWin();
	ptabeditor = wnd->GetActiveTabEditor();	
    ptabdata = wnd->GetActiveTabTableData();
	hitem =	TreeView_GetSelection(m_hwnd);
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetTableDatabaseName(hitem);
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetTableDatabaseName( hitem);
		break;

	case NTABLE:
		GetTableDatabaseName(hitem);
		break;

	default:
		return wyFalse;
	}

	message.Sprintf(_("Do you really want to truncate the table (%s)?\n\nWarning: You will lose all data!"), m_seltable.GetString());

	ret = yog_message(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 );

	switch(ret)
	{
	case IDYES:
		break;

	default:
		return wyFalse;
	}

	// prepare empty table query.
	query.Sprintf("truncate table `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());

	SetCursor(LoadCursor(NULL, IDC_WAIT));
    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

    tunnel->mysql_free_result(res);

	//issue reported here http://code.google.com/p/sqlyog/issues/detail?id=596
	//select "Tabledata" tab, truncate the table, it is not refreshing in "Tabledata" tab.
	if(ptabeditor && ptabeditor->m_pctabmgmt)
	{
        if(ptabeditor->m_pctabmgmt->GetSelectedItem() == IDI_TABLE)
        {
            ptabeditor->m_pctabmgmt->m_ptabletab->Refresh();
        }
	}
    else if(ptabdata && ptabdata->m_istabsticky == wyFalse)
    {
        ptabdata->ExecuteTableDataQuery();
    }

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

// This function creates the insert statement of the selected table.
// When executed it gets the current selected tablename and its database start traversing thru its 
// colnames and prepares the insert query statement.
// first we expand it because if the user has not expanded it once then we wont get the column names.

wyBool
CQueryObject::CreateInsertStmt()
{
	wyUInt32	ncount = 0, item;
	wyInt32		ret;
	wyString    query, strdbname, strtable, strcolname;
    wyWChar     colname[SIZE_512];
	wyWChar*	strtemp;
    wyString    columns, columns1, compquery; 
	HTREEITEM	hitem, htempitem;
	MDIWindow*	wnd;
	EditorBase	*peditorbase = NULL;
    wyBool      isbacktick;
	wyString	execquery;
	MYSQL_RES  *fieldres = NULL;
	MYSQL_ROW   row;
        
	VERIFY(wnd = GetActiveWin());

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NTABLE:
		break;

	default:
		return wyFalse;
	}

	/// Gets Db & Table names
	GetTableDatabaseName(hitem);

	strdbname.SetAs(m_seldatabase);
	strtable.SetAs(m_seltable);	
		
	SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);

	// but first we expand it. if its expanded already nothing really happens.
	ret = TreeView_Expand(m_hwnd, hitem, TVE_EXPAND); 

	// if the user does not have permission then we return wyFalse;
	if(!ret)
		return wyFalse;
	 
	hitem	=	TreeView_GetChild(m_hwnd, hitem);
	
	 //Read the .ini file
    isbacktick = AppendBackQuotes();

    query.Sprintf("\r\ninsert into %s%s%s.%s%s%s \r\n\t(", IsBackTick(isbacktick), strdbname.GetString(), IsBackTick(isbacktick), 
                    IsBackTick(isbacktick), strtable.GetString(), IsBackTick(isbacktick));

    ncount = 0;
	execquery.Sprintf("show full fields from `%s`.`%s`", strdbname.GetString(), strtable.GetString());
	fieldres = ExecuteAndGetResult(wnd,wnd->m_tunnel, &wnd->m_mysql, execquery);
	if(fieldres == NULL)
	{
		return wyFalse;
	}

	wyString temp;
	for(hitem = TreeView_GetChild(m_hwnd, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(m_hwnd, hitem))
	{
			GetNodeText(m_hwnd ,hitem, colname, SIZE_512);
			strtemp = colname;
			GetColumnName(strtemp);
			strcolname.SetAs(strtemp);
			
			row = wnd->m_tunnel->mysql_fetch_row(fieldres);
			temp.SetAs(row[6]);
			if(temp.CompareI("VIRTUAL") == 0 || temp.CompareI("Stored") == 0 || temp.CompareI("Persistent") == 0 || temp.CompareI("virtual generated") == 0 || temp.CompareI("stored generated") == 0 )
			{
				continue;
			}
				
			columns.AddSprintf("%s%s%s", IsBackTick(isbacktick), strcolname.GetString(), IsBackTick(isbacktick));
			columns1.AddSprintf("'%s'", strcolname.GetString());
			
			// now we check whether there is any more column.
			htempitem = TreeView_GetNextSibling(m_hwnd, hitem);	

			if(htempitem)
			{
				columns.Add(", ");
				columns1.Add(", ");
			}

			columns.Add("\r\n\t");
			columns1.Add("\r\n\t");
	}	
	///if temp is virtual,last col is virtual..hence remove the leading commas
	/// 5 for : ', ' characters added above and "\r\n\t" added above.

	if(temp.CompareI("VIRTUAL") == 0 || temp.CompareI("Stored") == 0 || temp.CompareI("Persistent") == 0 || temp.CompareI("virtual generated") == 0 || temp.CompareI("stored generated") == 0)
	{
		columns.Strip(5);
		columns1.Strip(5);
	}
	wnd->m_tunnel->mysql_free_result(fieldres);
    
	compquery.Add(query.GetString());
	compquery.Add(columns.GetString());
	compquery.Add(")\r\n\tvalues\r\n\t(");
    
	compquery.Add(columns1.GetString());
    compquery.Add(");\r\n");

	if(wnd->GetActiveTabEditor() == NULL)
        {
           SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);        
           return wyFalse;
        }

	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	/* add it to the current editor */
	if(peditorbase->GetAdvancedEditor())
	{
		wnd->m_pctabmodule->CreateQueryEditorTab(wnd);
		peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
		SetFocus(peditorbase->m_hwnd);
		wnd->SetQueryWindowTitle();
	}
		
	//Format query
	#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(wnd, 
														   peditorbase->m_hwnd,
														   (wyChar*)compquery.GetString(), PASTE_QUERY);
	

	#else
	wyInt32 selstart;
	VERIFY(selstart = SendMessage(peditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0));
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE, (LPARAM)compquery.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONSTART, selstart, 0);
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONEND, selstart + compquery.GetLength() , 0);	
	Format(peditorbase->m_hwnd, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_SELECTED_QUERY, GetIndentation());
	
	#endif
	SetFocus(peditorbase->m_hwnd);
	
    SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
	
	//adds * at the end of title to show the content of the edit box has change.
	wnd->SetDirtyTitle();
	
	//Post 8.01
	InvalidateRect(m_hwnd, NULL, FALSE);
	//RepaintTabModule();
	return wyTrue;
}

// This function creates the Update statement of the selected table.
// When executed it gets the current selected tablename and its database start traversing thru its 
// colnames and prepares the update query statement.
// first we expand it because if the user has not expanded it once then we wont get the column names.

wyBool
CQueryObject::CreateUpdateStmt()
{
	wyUInt32	item;
	wyInt32		ret;
	wyString    query, strdbname, strtable, strcolname;
    wyWChar     colname[SIZE_512];
	wyWChar		coltype[SIZE_512];
    wyString    columns, columns1, compquery, primary, colvalue; 
	TVITEM		tvi;
	HTREEITEM	hitem, htempitem;
	MDIWindow*	wnd;
	EditorBase	*peditorbase = NULL;
    wyBool      isbacktick;
	wyString	execquery;
	MYSQL_RES  *fieldres = NULL;
	MYSQL_ROW   row;
    int x;   // Flag variable to modify the where clause if it contains JSON    
	VERIFY(wnd = GetActiveWin());

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NTABLE:
		break;

	default:
		return wyFalse;
	}

	/// Gets Db & Table names
	GetTableDatabaseName(hitem);

	strdbname.SetAs(m_seldatabase);
	strtable.SetAs(m_seltable);	
			
	 SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);
	// but first we expand it. if its expanded already nothing really happens.
	ret = TreeView_Expand(m_hwnd, hitem, TVE_EXPAND); 

	// if the user does not have permission then we return wyFalse;
	if(!ret)
		return wyFalse;

	hitem	=	TreeView_GetChild(m_hwnd, hitem);
	
	 //Read the .ini file
    isbacktick = AppendBackQuotes();

    query.Sprintf("\r\nupdate %s%s%s.%s%s%s \r\n\tset\r\n\t", IsBackTick(isbacktick), strdbname.GetString(), IsBackTick(isbacktick), 
                    IsBackTick(isbacktick), strtable.GetString(), IsBackTick(isbacktick));
   
	//if there is no primary key then we need to add all fields. if primary key exists then we will add primary key
	GetPrimaryKeyInfo(wnd, primary, isbacktick); 
	execquery.Sprintf("show full fields from `%s`.`%s`", strdbname.GetString(), strtable.GetString());
	fieldres = ExecuteAndGetResult(wnd,wnd->m_tunnel, &wnd->m_mysql, execquery);
	if(fieldres == NULL)
	{
		return wyFalse;
	}
	wyString    temp;
	for(hitem = TreeView_GetChild(m_hwnd, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(m_hwnd, hitem))
		{
			TreeView_GetItem(m_hwnd, &tvi);

            GetNodeText(m_hwnd, hitem, colname, SIZE_512);

			
			GetColumnName(colname);
			strcolname.SetAs(colname);

			GetNodeText(m_hwnd, hitem, coltype, SIZE_512);
			x=IsColumnTypeJson(coltype);
			row = wnd->m_tunnel->mysql_fetch_row(fieldres);
			temp.SetAs(row[6]);
			if(temp.CompareI("VIRTUAL") == 0 || temp.CompareI("Stored") == 0 || temp.CompareI("Persistent") == 0 || temp.CompareI("virtual generated") == 0 || temp.CompareI("stored generated") == 0)
			{
				continue;
			}
			colvalue.Sprintf("%s%s%s = '%s'", IsBackTick(isbacktick), strcolname.GetString(), IsBackTick(isbacktick), strcolname.GetString());
			columns.Add(colvalue.GetString());
			if(x==1)
			{    // changing the where clause in the pasted statement if it is JSON
				colvalue.Sprintf("JSON_CONTAINS('%s',`%s`)",strcolname.GetString(), strcolname.GetString());
			}
			// columns1 is for where clause
			columns1.Add(colvalue.GetString());
			// now we check whether there is any more column.
			htempitem = TreeView_GetNextSibling(m_hwnd, hitem);				
		
			if(htempitem)
			{
				columns.Add(", ");
				columns1.Add("and ");
			}

			columns.Add("\r\n\t");
			columns1.Add("\r\n\t");
		}
	///if temp is virtual,last col is virtual..hence remove the leading commas
	/// 5 for : ', ' characters added above and "\r\n\t" added above.
	/// 7 for : 'and ' characters added above and "\r\n\t" added above.
	if(temp.CompareI("VIRTUAL") == 0 || temp.CompareI("Stored") == 0 || temp.CompareI("Persistent") == 0 || temp.CompareI("virtual generated") == 0 || temp.CompareI("stored generated") == 0)
	{
		columns.Strip(5);
		columns1.Strip(7);
	}
	wnd->m_tunnel->mysql_free_result(fieldres);

    compquery.Add(query.GetString());
    compquery.Add(columns.GetString());
	compquery.Add("\r\n\twhere\r\n\t");

    if(primary.GetLength() != 0)
		compquery.Add(primary.GetString());
	else
		compquery.Add(columns1.GetString());

    compquery.Add(";\r\n");
    
	if(wnd->GetActiveTabEditor() == NULL)
        {
           SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);        
           return wyFalse;
        }

	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	/* add it to the current editor */
	if(peditorbase->GetAdvancedEditor())
	{
		wnd->m_pctabmodule->CreateQueryEditorTab(wnd);
		peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
		SetFocus(peditorbase->m_hwnd);
		wnd->SetQueryWindowTitle();
	}
	
	//Format query
	#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(wnd, 
														   peditorbase->m_hwnd,
														   (wyChar*)compquery.GetString(), PASTE_QUERY);

	#else
	wyInt32 selstart;
	VERIFY(selstart = SendMessage(peditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0));		
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE, (LPARAM)compquery.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONSTART, selstart, 0);
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONEND, selstart + compquery.GetLength() , 0);
	Format(peditorbase->m_hwnd, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_SELECTED_QUERY, GetIndentation());
	#endif	
	
	SetFocus(peditorbase->m_hwnd);
	
    SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
		
	//adds * at the end of title to show the content of the edit box has change.
	wnd->SetDirtyTitle();
		
	//Post 8.01
	InvalidateRect(m_hwnd, NULL, FALSE);
	//RepaintTabModule();
	return wyTrue;
}


// This function creates the Delete statement of the selected table.
// When executed it gets the current selected tablename and its database start traversing thru its 
// colnames and prepares the delete query statement.
// first we expand it because if the user has not expanded it once then we wont get the column names.

wyBool
CQueryObject::CreateDeleteStmt()
{
	wyUInt32	item;
	wyInt32		ret;
	wyString    query, strdbname, strtable, strcolname,primary;
    wyWChar     colname[SIZE_512],coltype[SIZE_512];
    wyString    columns, columns1, compquery; 
	TVITEM		tvi;
	HTREEITEM	hitem, htempitem;
	MDIWindow*	wnd;
	EditorBase	*peditorbase = NULL;
    wyBool      isbacktick;
	int x;   // Flag variable to modify the where clause if it contains JSON
        
	VERIFY(wnd = GetActiveWin());

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NTABLE:
		break;

	default:
		return wyFalse;
	}

	/// Gets Db & Table names
	GetTableDatabaseName(hitem);

	strdbname.SetAs(m_seldatabase);
	strtable.SetAs(m_seltable);	
		
	 SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);

	// but first we expand it. if its expanded already nothing really happens.
	ret = TreeView_Expand(m_hwnd, hitem, TVE_EXPAND); 

	// if the user does not have permission then we return wyFalse;
	if(!ret)
		return wyFalse;

	hitem	=	TreeView_GetChild(m_hwnd, hitem);
	
	 //Read the .ini file
    isbacktick = AppendBackQuotes();

    query.Sprintf("\r\ndelete from %s%s%s.%s%s%s \r\n\twhere\r\n\t", IsBackTick(isbacktick), strdbname.GetString(), IsBackTick(isbacktick), 
                    IsBackTick(isbacktick), strtable.GetString(), IsBackTick(isbacktick));

   	//if there is no primary key then we need to add all fields. if primary key exists then we will add primary key
	if(GetPrimaryKeyInfo(wnd, columns, isbacktick) == wyFalse) 
	{
		for(hitem = TreeView_GetChild(m_hwnd, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(m_hwnd, hitem))
		{
			TreeView_GetItem(m_hwnd, &tvi);

            GetNodeText(m_hwnd, hitem, colname, SIZE_512);
			
			GetColumnName(colname);
			strcolname.SetAs(colname);

			GetNodeText(m_hwnd, hitem, colname, SIZE_512);
			x=IsColumnTypeJson(coltype);
			if(x==0)
			columns.AddSprintf("%s%s%s = '%s'", IsBackTick(isbacktick), strcolname.GetString(), IsBackTick(isbacktick), strcolname.GetString() );
			if(x==1)
			{    // changing the where clause in the pasted statement if it is JSON
				columns.AddSprintf("JSON_CONTAINS('%s',`%s`)",strcolname.GetString(), strcolname.GetString());
			}
			// now we check whether there is any more column.
			htempitem = TreeView_GetNextSibling(m_hwnd, hitem);				
		
			if(htempitem)
			{
				columns.Add(" and ");
			}

			columns.Add("\r\n\t");
		}
	}

    compquery.Add(query.GetString());
    compquery.Add(columns.GetString());
	compquery.Add(";\r\n");
    
	if(wnd->GetActiveTabEditor() == NULL)
        {
           SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);        
           return wyFalse;
        }

	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	/* add it to the current editor */
	if(peditorbase->GetAdvancedEditor())
	{
		wnd->m_pctabmodule->CreateQueryEditorTab(wnd);
		peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
		SetFocus(peditorbase->m_hwnd);
		wnd->SetQueryWindowTitle();
	}
	
	//Format query
	#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(wnd, 
														   peditorbase->m_hwnd,
														   (wyChar*)compquery.GetString(), PASTE_QUERY);
	#else
	wyInt32 selstart;
	VERIFY(selstart = SendMessage(peditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0));		
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE, (LPARAM)compquery.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONSTART, selstart, 0);
	SendMessage(peditorbase->m_hwnd, SCI_SETSELECTIONEND, selstart + compquery.GetLength() , 0);
	Format(peditorbase->m_hwnd, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_SELECTED_QUERY, GetIndentation());
	#endif
		
	SetFocus(peditorbase->m_hwnd);
	
    SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
	
	//adds * at the end of title to show the content of the edit box has change.
	wnd->SetDirtyTitle();
	
	InvalidateRect(peditorbase->m_hwnd, NULL, FALSE);
	
	//Post 8.01
	//RepaintTabModule();
	return wyTrue;
}

/**
   This function creates the select statement of the selected table.
   When executed it gets the current selected tablename and its database start traversing thru its 
   colnames and prepares the select query statement.
   first we expand it because if the user has not expanded it once then we wont get the column names.
*/
wyBool
CQueryObject::CreateSelectStmt()
{
	wyUInt32	ncount = 0, item;
	wyInt32     ret;
	wyString    query, strdbname, strtable, highlimitstr, colnamestr;
    wyWChar     colname[SIZE_512];
    wyString    columns, compquery, dirstr;
    wyWChar		directory[MAX_PATH+1] = {0}, *lpfileport; 
	wyString	backquote;
	HTREEITEM	hitem, htempitem;
	MDIWindow*	wnd;
    wyBool      isbacktick;
	
	VERIFY(wnd	=	GetActiveWin());

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();
	
	switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
	case NCOLUMN:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NTABLE:
		break;

	default:
		return wyFalse;
	}

    /// Gets Db & Table names
	GetTableDatabaseName(hitem);

	strdbname.SetAs(m_seldatabase);
	strtable.SetAs(m_seltable);	

	SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);

	// but first we expand it. if its expanded already nothing really happens.
	ret = TreeView_Expand(m_hwnd, hitem, TVE_EXPAND); 

	// if the user does not have permission then we return wyFalse;
	if(!ret)
		return wyFalse;

	hitem = TreeView_GetChild(m_hwnd, hitem);

	//Read the .ini file
    isbacktick = AppendBackQuotes();
   
    query.Sprintf(" \r\n\tfrom \r\n\t%s%s%s.%s%s%s", IsBackTick(isbacktick),  strdbname.GetString(), IsBackTick(isbacktick),
                  IsBackTick(isbacktick), strtable.GetString(), IsBackTick(isbacktick));

	// allocate some buffer for the columns text and the for the complete querytext.
	ncount = 0;
	
	for(hitem = TreeView_GetChild(m_hwnd, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(m_hwnd, hitem))
	{
            GetNodeText(m_hwnd, hitem, colname, SIZE_512);
			
			GetColumnName(colname);
			
			colnamestr.SetAs(colname);

			columns.AddSprintf("%s%s%s", IsBackTick(isbacktick), colnamestr.GetString(), IsBackTick(isbacktick));

			// now we check whether there is any more column.
			htempitem = TreeView_GetNextSibling(m_hwnd, hitem);				
		
			if(htempitem)
				columns.Add(", ");

			columns.Add("\r\n\t");
	}

	compquery.Add("\r\nselect \t");
	compquery.Add(columns.GetString());
	compquery.Add(query.GetString());
	compquery.Add(" \r\n\tlimit 0, ");

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	dirstr.SetAs(directory);
    
	wyIni::IniGetString(GENERALPREFA, "ResulttabPageRows", STR_HIGHLIMIT_DEFAULT, &highlimitstr, dirstr.GetString());

	if(highlimitstr.GetAsInt32() <= 0)
		compquery.Add(STR_HIGHLIMIT_DEFAULT);

	else
		compquery.Add(highlimitstr.GetString());

	compquery.Add(";\r\n");

	if(wnd->GetActiveTabEditor()->m_peditorbase->GetAdvancedEditor())
	{
		wnd->m_pctabmodule->CreateQueryEditorTab(wnd);
		SetFocus(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd);
		wnd->SetQueryWindowTitle();
	}
	
	//Format query
	#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(wnd, 
														   wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd,
														   (wyChar*)compquery.GetString(), PASTE_QUERY);
	
	
	#else
	wyInt32 selstart;
	VERIFY(selstart = SendMessage(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0));		
	SendMessage(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, SCI_REPLACESEL, TRUE, (LPARAM)compquery.GetString());
	SendMessage(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, SCI_SETSELECTIONSTART, selstart, 0);
	SendMessage(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, SCI_SETSELECTIONEND, selstart + compquery.GetLength() , 0);
	Format(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_SELECTED_QUERY, GetIndentation());
	#endif
	
	
	SetFocus(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd);
	SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
   
	//adds * at the end of title to show the content of the edit box has change.
	wnd->SetDirtyTitle();
   
	//Post 8.01
	InvalidateRect(wnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, NULL, FALSE);
	// RepaintTabModule();
	return wyTrue;
}

// Function calculates and returns the number of character which can come in the width of
// the edit box. This is required because when we create a insert statement and there are
// lot of columns then we can divide them in multiple lines.
wyInt32
CQueryObject::GetMaxLimit(MDIWindow * wnd)
{
	HDC		    hdc;
	wyInt32	    ret;
	RECT	    rc;
	wyInt32		maxlimit = 0;
	wyInt32		dx[8192];					// 8192 is some big number.
	wyWChar     tempstring[8192];
	SIZE	    sz;
    TabEditor   *ptabeditor = NULL;
    EditorBase  *peditorbase = NULL;
	
	ptabeditor = wnd->GetActiveTabEditor();	
	
    peditorbase = ptabeditor->m_peditorbase;

	ret = GetClientRect(peditorbase->m_hwnd, &rc);
	_ASSERT(ret);

	// get the width of the edit control and also its dc.
    hdc = GetDC(peditorbase->m_hwnd);
	_ASSERT(hdc);

	// now fill the string with some values.
	memset(tempstring, 'W', 8190);
	tempstring[8190] = 0;

	ret = GetTextExtentExPoint(hdc, tempstring, 8190, rc.right-rc.left, &maxlimit, dx, &sz);
	_ASSERT(ret);

	ReleaseDC(peditorbase->m_hwnd, hdc);	

	return maxlimit;
}

wyBool
CQueryObject::GetPrimaryKeyInfo(MDIWindow * wnd, wyString &primary, wyBool isbacktick)
{
	wyString	query,colname, uniquekey,myrowstr ,keyname;
	MYSQL_RES	*keyres;
	MYSQL_ROW	myrow;
	wyBool		isprimary = wyFalse, flag = wyFalse;
	wyInt32		fldindex;
	wyBool		ismysql41 = IsMySQL41(wnd->m_tunnel, &wnd->m_mysql);


	query.Sprintf("show keys from `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());
	
    keyres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	if(!keyres)
	{
		return wyFalse;
	}

	wnd->m_tunnel->mysql_data_seek(keyres, 0);

	while(myrow = wnd->m_tunnel->mysql_fetch_row(keyres))
	{
		fldindex = GetFieldIndex(wnd->m_tunnel, keyres, "Non_unique"); 
		if(fldindex >= 0)
			myrowstr.SetAs(myrow[fldindex], ismysql41);

		fldindex = GetFieldIndex(wnd->m_tunnel, keyres, "Key_name"); 
		if(fldindex >= 0)
			keyname.SetAs(myrow[fldindex], ismysql41);
		
		fldindex = GetFieldIndex(wnd->m_tunnel, keyres, "Column_name"); 
		if(fldindex >= 0)
			colname.SetAs(myrow[fldindex], ismysql41);

		if(isprimary == wyFalse && !keyname.CompareI("PRIMARY"))//primary key is there or not
			isprimary = wyTrue;

		if(isprimary == wyTrue)//primary key is single or a concatenated
		{
			if(!keyname.CompareI("PRIMARY"))
			{
				flag = wyTrue;
				primary.AddSprintf("%s%s%s = '%s' and ",IsBackTick(isbacktick), colname.GetString(), IsBackTick(isbacktick), colname.GetString());
				
			}	
		}
		else if(isprimary == wyFalse && !myrowstr.CompareI("0") )//if there is no primary key , then we need to check unique key is there or not.
		{
			if(!uniquekey.GetLength())
				uniquekey.SetAs(keyname);
			
			if(!keyname.CompareI(uniquekey)) //unique key is concatenated or not
			{
				flag = wyTrue;
				primary.AddSprintf("%s%s%s = '%s' and ", IsBackTick(isbacktick), colname.GetString(), IsBackTick(isbacktick), colname.GetString());
			}
		}
	}
	
	wnd->m_tunnel->mysql_free_result(keyres);
	
	if(flag)
		primary.Strip(4);
	
	return flag;
}

// Function is called when the user wants to create a new table.
// Calls CTableMaker which encapsulates in it the table maker window.
wyBool
CQueryObject::CreateTableMaker()
{
	wyInt32	    image;
	HTREEITEM   hselected;
	MDIWindow	*wnd;

	VERIFY(wnd = GetActiveWin());
	VERIFY(hselected = TreeView_GetSelection(m_hwnd)); 

	//TableMakerCreateTable pctablemaker;
	image = GetSelectionImage();
	
	switch(image)
	{
	case NDATABASE:
		break;

	case NSPITEM:
	case NVIEWSITEM:
	case NFUNCITEM:
	case NTRIGGERITEM:
	case NTABLE:
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		break;

	case NFOLDER:
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		break;

	case NTABLES:
	case NTRIGGER:
	case NSP:
	case NVIEWS:
	case NFUNC:
	case NEVENTS:
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		break;
		
	case NCOLUMN:
	case NPRIMARYKEY:
	case NPRIMARYINDEX:
	case NINDEX:
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		VERIFY(hselected = TreeView_GetParent(m_hwnd, hselected));
		break;
	}

	//Context db name
	GetDatabaseName(hselected);
    wnd->m_pctabmodule->CreateTableTabInterface(wnd);

    /*
    ret = pctablemaker.Create(wnd, wnd->m_tunnel, &wnd->m_mysql,  m_seldatabase.GetString(), NULL);
	if(ret == wyTrue)
		RefreshObjectBrowserOnCreateAlterTable();
	*/

	return wyTrue;
}

/// Refresh the current DB and select table that is created / altered
void
CQueryObject::RefreshObjectBrowserOnCreateAlterTable()
{
	wyInt32	    image;
	wyBool		ret;
	HTREEITEM   hselected, hdbselected, hitem;;
	MDIWindow	*wnd;

	VERIFY(wnd = GetActiveWin());
	VERIFY(hselected = TreeView_GetSelection(m_hwnd)); 
	if(!hselected)
		return;
	
	image = GetSelectionImage();
	/// If selecton on table, then refresh the 'Tables' node and select particular item
	if(image == NTABLE)
	{
		hselected = TreeView_GetParent(m_hwnd, hselected);
		ret = RefreshParentNode(hselected);
		if(ret == wyTrue)
		{
			hselected = TreeView_GetChild(m_hwnd, hselected);		
			if(!hselected)
			{
				m_isrefresh = wyFalse;
				return;
			}
		}
	}

	else
	{
		hdbselected = GetDatabaseNode();
		if(hdbselected)
		{	
			hselected = GetNode(hdbselected, NTABLES); 
			RefreshParentNode(hselected); //refresh Tables folder
			hselected = TreeView_GetChild(m_hwnd, hselected);
			if(!hselected)
			{
				m_isrefresh = wyFalse;
				return;
			}
		}

		else
		{			
			//Expand db
			//retval = TreeView_Expand(m_hwnd, hselected, TVE_EXPAND);
			RefreshParentNode(hselected);

			//Expand 'Tables'
			hselected = TreeView_GetChild(m_hwnd, hselected);
			//retval = TreeView_Expand(m_hwnd, hselected, TVE_EXPAND);
			RefreshParentNode(hselected);
			
			/// Gets the first item in the db object 
			hselected = TreeView_GetChild(m_hwnd, hselected);		
			if(!hselected)
			{
				m_isrefresh = wyFalse;
				return;
			}
		}
	}
	
	// Get the item to be selected and select that particular item
	hitem = GetTreeObjectItem(hselected, m_seltable.GetAsWideChar());
	if(hitem)
	{
		TreeView_SelectItem(m_hwnd, hitem);
		m_isrefresh = wyFalse;            
	}	
}

// This function implements on item expanding message.
// We dont store all the data about the database in one shot.
// When the user exapands for the first time we retrieve the data and then we show.
wyBool
CQueryObject::OnItemExpandingHelper(LPNMTREEVIEW pnmtv, MDIWindow *wnd)
{
	TVITEM			tvi;
	TREEVIEWPARAMS	treeviewparam = {0};

	tvi = pnmtv->itemNew;

	treeviewparam.database = NULL;
	treeviewparam.hwnd = m_hwnd;
	treeviewparam.isopentables = IsOpenTablesInOB();
	treeviewparam.isrefresh = m_isrefresh;
	treeviewparam.issingletable = wyFalse;
	treeviewparam.mysql = &wnd->m_mysql;
	treeviewparam.tunnel = wnd->m_tunnel;
	treeviewparam.tvi = &tvi;
	treeviewparam.checkboxstate = wyFalse;

	//m_isexpcollapse = OnItemExpanding(m_hwnd, &tvi, wnd->m_tunnel, wnd->m_mysql, NULL, wyFalse, IsOpenTablesInOB(), m_isrefresh);
	m_isexpcollapse = OnItemExpanding(treeviewparam, m_isnoderefresh);

	return wyFalse;
}

// change toolbars depending upon selection.
wyBool
CQueryObject::OnSelChanged(HTREEITEM hitem, LPNMTREEVIEW pnmtv)
{
    HTREEITEM   hitemtemp, hroot, hparent;
	wyInt32     image, size, nodecount;
	wyInt32     nid[] = {  ID_OPEN_COPYDATABASE, /*ID_DATABASE_REBUILDTAGS, vgladcode*/ ID_OBJECT_CREATESCHEMA,  ID_EXPORT_AS, ID_OBJECT_COPYTABLE, ID_OBJECT_INSERTUPDATE, ID_OBJECT_TABLEEDITOR, ID_OBJECT_MAINMANINDEX, ACCEL_MANREL };
	wyInt32     nidd[] = { ID_EXPORT_AS, ID_OBJECT_COPYTABLE, ID_OBJECT_INSERTUPDATE, ID_OBJECT_TABLEEDITOR, ID_OBJECT_MAINMANINDEX, ACCEL_MANREL, ID_OBJECT_COPYTABLE};
    wyInt32     viewdata[] = {ID_OBJECT_VIEWDATA, ID_TABLE_OPENINNEWTAB};
	MDIWindow   *wnd = GetActiveWin();
	wyChar      trigger[SIZE_512] = {0};
    TabEditor   *ptabeditor = NULL;	
    TabMgmt     *ptabmgmt   = NULL;
	TabObject	*pinfotab   = NULL;
	wyInt32		tabicon = 0, state;


    if(m_OBFilterWorkInProgress && pnmtv->action == TVC_UNKNOWN && pnmtv->itemNew.hItem)
    {
        image = GetItemImage(m_hwnd, pnmtv->itemNew.hItem);
        switch(image)
        {
        case NTABLE:
        case NVIEWSITEM:
        case NSPITEM:
        case NFUNCITEM:
        case NTRIGGERITEM:
        case NEVENTITEM:
            hitemtemp = TreeView_GetParent(m_hwnd, pnmtv->itemNew.hItem);
            TreeView_SelectItem(m_hwnd, hitemtemp);
            break;
        case NCOLUMN:
        case NINDEX:
        case NPRIMARYINDEX:
        case NPRIMARYKEY:
            hitemtemp = TreeView_GetParent(m_hwnd, pnmtv->itemNew.hItem);
            hitemtemp = TreeView_GetParent(m_hwnd, hitemtemp);
            TreeView_SelectItem(m_hwnd, hitemtemp);
            break;
        }
        return wyTrue;
    }

    hroot = TreeView_GetRoot(m_hwnd);

    if(pnmtv && hroot && pnmtv->itemNew.hItem && pnmtv->itemNew.hItem != hroot)
    {
        for(hitemtemp = TreeView_GetChild(m_hwnd, hroot); hitemtemp; hitemtemp = TreeView_GetNextSibling(m_hwnd, hitemtemp))
        {
            state = TreeView_GetItemState(m_hwnd, hitemtemp, TVIS_BOLD);

            if(state & TVIS_BOLD)
            {
                TreeView_SetItemState(m_hwnd, hitemtemp, 0, TVIS_BOLD);
            }
        }

        for(hitemtemp = pnmtv->itemNew.hItem;
            hitemtemp && (hparent = TreeView_GetParent(m_hwnd, hitemtemp)) != hroot; 
            hitemtemp = hparent)
        {
            if(!hparent)
            {
                hitemtemp = NULL;
                break;
            }
        }

        if(hitemtemp)
        {
            TreeView_SetItemState(m_hwnd, hitemtemp, TVIS_BOLD, TVIS_BOLD);
        }
    }

	image	= GetSelectionImage();

	tabicon = wnd->m_pctabmodule->GetActiveTabImage();

	if(tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_SCHEMADESIGNER_16 && tabicon != IDI_DATASEARCH && tabicon != IDI_TABLEINDEX)
	{
        ptabeditor = (TabEditor*)wnd->GetActiveTabEditor();
	}
	if(tabicon == IDI_TABLEINDEX)
	{
		pinfotab = (TabObject*)wnd->m_pctabmodule->GetActiveTabType();
	}
	size = sizeof(nid)/ sizeof(nid[0]);

	// if we are between a refresh option then we return back.
	/*if(m_isrefresh == wyTrue)
		return wyTrue;*/
    
    if(ptabeditor)
        ptabmgmt   = ptabeditor->m_pctabmgmt;
    
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	// first enable all the buttons.
	for(nodecount = 0; nodecount < size; nodecount++)
		SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nid[nodecount], TBSTATE_ENABLED);
   	   
	if(image == NSERVER)
    {
		for(nodecount = 0; nodecount < size; nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nid[nodecount], TBSTATE_INDETERMINATE);
		if(pinfotab)
        {
            pinfotab->OnSelectInfoTab(wnd);
        }
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
    else if(image == NDATABASE)
    {
		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		GetDatabaseName(hitem);
				
		if(pinfotab)
		{						
			pinfotab->m_pobjinfo->Refresh();
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }

#ifndef COMMUNITY
		TabSchemaDesigner *ptabsd = NULL;

		//Enable / Disable canvas toool buttons.
		if(m_seldatabase.GetLength())
		{
			if(wnd->m_pctabmodule->GetActiveTabImage() == IDI_SCHEMADESIGNER_16)
			{
				ptabsd = (TabSchemaDesigner*)wnd->m_pctabmodule->GetActiveTabType();
				ptabsd->EnableDisableCanvasToolbarIconsOnDBSelected(wyTrue);				
			}
		}
#endif


	} 
    else if((image == NTABLE))
    {
		GetTableDatabaseName(hitem);
        
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_ENABLED);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_ENABLED);
        		
        if(OpenTable() == wyFalse && pinfotab)
        {
            pinfotab->m_pobjinfo->Refresh();
        }
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
#ifndef COMMUNITY
		if(wnd->m_conninfo.m_isreadonly == wyTrue)
		{
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,ID_OBJECT_MAINMANINDEX, TBSTATE_INDETERMINATE);
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,ACCEL_MANREL, TBSTATE_INDETERMINATE);
		}
#endif
    }

    else if(image == NSPITEM)
    {
		GetTableDatabaseName(hitem);
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		
        if(ptabeditor)
		{
            SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
            SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);
        }
		
        else
        {
            SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], FALSE);
            SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], FALSE);
        }

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
		
		if(pinfotab)
		{
			pinfotab->m_pobjinfo->Refresh();
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	
	}
    else if(image == NVIEWSITEM)
    {
		GetTableDatabaseName(hitem);
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);
        
		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		// tabe data or show value
						
		if(OpenTable() == wyFalse && pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
    else if(image == NFUNCITEM)
    {
		GetTableDatabaseName(hitem);
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
	
		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
    else if(image == NTRIGGERITEM)
    {
		GetTableDatabaseName(hitem);
		strcpy(trigger, m_seltable.GetString());
		
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
		
		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NEVENTITEM)
    {
		GetTableDatabaseName(hitem);
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
	
		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NTABLES)
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);
		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NVIEWS)
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		if(pinfotab)
		{						
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NSP)
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NFUNC)
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
		
		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NTRIGGER )
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);

		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);
		
		if(pinfotab)
		{
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}
	else if(image == NEVENTS)
	{
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		
		for(nodecount = 0; nodecount < (sizeof(nidd)/ sizeof(nidd[0])); nodecount++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[nodecount], TBSTATE_INDETERMINATE);

		if(pinfotab)
		{			
			pinfotab->OnSelectInfoTab(wnd);
		}
        else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
        {
            ptabmgmt->m_pqueryobj->Refresh();
        }
	}

	
	// now change the default database if its not same.
	if(image != NSERVER)
    {
		HTREEITEM		htempitem;
		TVITEM tvItem;
		wyWChar column[SIZE_512];
		wyChar tempcolumn[SIZE_64+1];
		
		if(image == NFOLDER)// || image == NTRIGGER) 
        {
			VERIFY(htempitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(htempitem);

			if(OpenTable() == wyFalse && pinfotab)
			{			
				pinfotab->OnSelectInfoTab(wnd);
			}
            else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
            {
                ptabmgmt->m_pqueryobj->Refresh();
            }
		} 
		else if(image == NCOLUMN || image == NPRIMARYKEY)
		{
			tvItem.pszText     = column;
			tvItem.cchTextMax  = SIZE_512 - 1;
			tvItem.hItem = hitem;
			tvItem.mask = TVIF_TEXT;
			TreeView_GetItem(m_hwnd,&tvItem);
			m_selcolumn.SetAs(tvItem.pszText);
			wyInt32 pos = m_selcolumn.FindChar(',');
			strcpy(tempcolumn, m_selcolumn.Substr(0,pos));
			m_selcolumn.SetAs(tempcolumn);
			VERIFY(htempitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(htempitem = TreeView_GetParent(m_hwnd, htempitem));
			GetTableDatabaseName(htempitem);

			if(OpenTable() == wyFalse && pinfotab)
			{			
				pinfotab->OnSelectInfoTab(wnd);
			}
            else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
            {
                ptabmgmt->m_pqueryobj->Refresh();
            }
		} 
        else if(image == NINDEX ||image == NPRIMARYINDEX) 
        {
			VERIFY(htempitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(htempitem = TreeView_GetParent(m_hwnd, htempitem));
			GetTableDatabaseName(htempitem);

			if(OpenTable() == wyFalse && pinfotab)
			{			
				pinfotab->OnSelectInfoTab(wnd);
			}
            else if(ptabmgmt && ptabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
            {
                ptabmgmt->m_pqueryobj->Refresh();
            }
		} 
#ifndef COMMUNITY
		if(wnd->m_conninfo.m_isreadonly == wyTrue)
		{
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,ID_OBJECT_MAINMANINDEX, TBSTATE_INDETERMINATE);
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,ACCEL_MANREL, TBSTATE_INDETERMINATE);
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,ID_OPEN_COPYDATABASE, TBSTATE_INDETERMINATE);
		}
#endif
	}

	// now change the default db.
	if(image != NSERVER)
    {
		wnd->m_tunnel->SetDB(m_seldatabase.GetString());
		SyncObjectDB(wnd);
	}

    if(tabicon == IDI_DATASEARCH)
	{
#ifndef COMMUNITY
		TabDbSearch *tabsearch = NULL; 

		tabsearch = dynamic_cast<TabDbSearch*>(wnd->m_pctabmodule->GetActiveTabType());
		
        if(tabsearch)
		{
			tabsearch->SetSearchScope();
		    tabsearch->EnableDisableSearchButton();
		}
#endif
	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));
	SetFocus(m_hwnd);
	return wyTrue;
}

// This function selects the database which is passed as parameter and then expands the tables also.
// This is helpful when the user creates, deletes or refreshes the object browser we can select the database.
wyBool
CQueryObject::ExpandDatabase()
{
	wyInt32		ret;
	wyWChar		database[SIZE_512] = {0}, table[SIZE_512] = {0};
	HTREEITEM	hdatabaseitem, hitemroot, hitemtable;

	hitemroot	=	TreeView_GetRoot(m_hwnd);

	// check if we can proceed.
	if((hitemroot == NULL)||(m_seldatabase.GetLength() == 0)||(m_seltype == NSERVER)||(m_seltype == -1))
		return wyFalse;

	// Now get the first child and recurse till a database is found.
	// if found then we expand depending on the global flag used in the class.
	// we set the refresh flag to true so that we dont execute queries two times.
	m_isrefresh = wyTrue;
	hdatabaseitem = TreeView_GetChild(m_hwnd, hitemroot);	
	while(hdatabaseitem != NULL)
	{
		memset(database, 0, sizeof(database));
        ret = GetNodeText(m_hwnd, hdatabaseitem, database, SIZE_512);
		
		if(!ret)
			break;

		if((wcsicmp(database, m_seldatabase.GetAsWideChar())) == 0)
			break;

		hdatabaseitem = TreeView_GetNextSibling(m_hwnd, hdatabaseitem);
	}

	if(!hdatabaseitem)
	{
		// now this means that the database things didnt match.
		// so we have to see whether the selected item was status, varibales or a processlist item or not.
		while(hitemroot)
		{
			memset(database, 0, sizeof(database));
            ret = GetNodeText(m_hwnd, hitemroot, database, SIZE_512);
			
			if(!ret)
				break;

			if((wcscmp(database, m_seldatabase.GetAsWideChar())) == 0)
			{
				TreeView_SelectItem(m_hwnd, hitemroot);
				m_isrefresh = wyFalse;
				return wyTrue;
			}

			hitemroot = TreeView_GetNextSibling(m_hwnd, hitemroot);
		}

		return wyFalse;
	}	
	
	// now check if the previous selection type is of type TABLE, COLUMN or INDEX then only 
	// we select the database otherwise we do not select it.
	if((m_seltype == NTABLE)||(m_seltype == NCOLUMN) ||(m_seltype == NPRIMARYKEY)|| 
		(m_seltype == NINDEX)||(m_seltype == NPRIMARYINDEX)||(m_seltype == NFOLDER)||(m_seltype == NVIEWS)|| 
		(m_seltype == NSP)||(m_seltype == NFUNC)||(m_seltype == NTRIGGER)||
		(m_seltype == NVIEWSITEM)||(m_seltype == NSPITEM)||(m_seltype == NFUNCITEM)||(m_seltype == NTRIGGERITEM))
	{
		// now we select the table and expand everything but first we expand this database.
		ret = TreeView_Expand(m_hwnd, hdatabaseitem, TVE_EXPAND);
		
		if(!ret)
			return wyFalse;
		
		/// Gets the 'Tables' node and expand it
		hitemtable = TreeView_GetChild(m_hwnd, hdatabaseitem);
		if(!hitemtable)
        {
			m_isrefresh = wyFalse;
			return wyFalse;
		}

		ret = TreeView_Expand(m_hwnd, hitemtable, TVE_EXPAND);
		if(!ret)
		{
			m_isrefresh = wyFalse;
			return wyFalse;
		}
		
		/// Gets the first table in the group 'Tables'
		hitemtable = TreeView_GetChild(m_hwnd, hitemtable);		
		if(!hitemtable)
        {
			m_isrefresh = wyFalse;
			return wyFalse;
		}

		while(hitemtable != NULL)
		{
			memset(table, 0, sizeof(table));
            ret = GetNodeText(m_hwnd, hitemtable, table, SIZE_512);
			
			if(!ret)
				break;

			if((wcsicmp(table, m_seltable.GetAsWideChar())) == 0)
				break;

			hitemtable = TreeView_GetNextSibling(m_hwnd, hitemtable);
		}

		if(!hitemtable) 
        {
			m_isrefresh  = wyFalse;
			return wyFalse;
		}

		if((m_seltype == NTABLE)||(m_seltype == NSP)|| 
			(m_seltype == NFUNC)||(m_seltype == NVIEWS))
        {
            m_isrefresh = wyFalse;
			TreeView_SelectItem(m_hwnd, hitemtable);
			return wyTrue;
		}
		
		// now expand it and then expand its column node.
		ret = TreeView_Expand(m_hwnd, hitemtable, TVE_EXPAND);
		if(ret == wyFalse)
			return wyFalse;

		// first check if the sel type is index then we expand the INDEX data only and return.
		if(m_seltype == NINDEX || m_seltype == NPRIMARYINDEX)
		{
			hitemtable = TreeView_GetChild(m_hwnd, hitemtable);
			hitemtable = TreeView_GetNextSibling(m_hwnd, hitemtable);
			TreeView_Expand(m_hwnd, hitemtable, TVE_EXPAND);
			TreeView_SelectItem(m_hwnd, hitemtable);
			m_isrefresh = wyFalse;
			return wyTrue;
		}
		else
		{
			TreeView_Expand(m_hwnd, TreeView_GetChild(m_hwnd, hitemtable), TVE_EXPAND);
			TreeView_SelectItem(m_hwnd, TreeView_GetChild(m_hwnd, hitemtable));
			m_isrefresh = wyFalse;
			return wyTrue;
		}
	}
	else
	{
		m_isrefresh = wyFalse;
		// we just expand the database and return.
		TreeView_Expand(m_hwnd, hdatabaseitem, TVE_EXPAND);
		TreeView_SelectItem(m_hwnd, hdatabaseitem);
		
		return wyTrue;
	}
}

/// Search a particular child item and return if found, else return NULL
HTREEITEM	
CQueryObject::GetTreeObjectItem(HTREEITEM hitemobject, wyWChar *itemname)
{
	wyWChar table[SIZE_512] = {0};
	wyInt32	ret;
	
	while(hitemobject != NULL)
	{
		memset(table, 0, sizeof(table));
        ret = GetNodeText(m_hwnd, hitemobject, table, SIZE_512);
		
		if(!ret)
			break;

		if((wcsicmp(table, m_seltable.GetAsWideChar())) == 0)
			return hitemobject;

		hitemobject = TreeView_GetNextSibling(m_hwnd, hitemobject);
        
	}

	return NULL;
}

void 
CQueryObject::RestoreTreeState()
{
    HTREEITEM	hitemroot, hitemdb;
    wyWChar     buff[SIZE_512] = {0}, **retname, *nodename;    
    TVITEMEX    item;
	wyString	buffstr;

	hitemroot = TreeView_GetRoot(m_hwnd);

	if(!hitemroot)
        return;

	nodename = AllocateBuffWChar(512);
    qsort((void *)m_expandednodes, (size_t)m_nodecount, sizeof(wyWChar *), (wyInt32 (*)(const void *, const void *))Comparestring);

    hitemdb = TreeView_GetChild(m_hwnd, hitemroot);

    while(hitemdb)
    {
		item.hItem      = hitemdb;
        item.mask       = TVIF_TEXT;
        item.cchTextMax	= (SIZE_512 - 1);
	    item.pszText    = buff;
		
		TreeView_GetItem(m_hwnd, &item); 

        if(buff == NULL)
            break;

		wcscpy(nodename , buff);
		
		retname = (wyWChar **)bsearch((wyWChar *)&nodename, (wyWChar *)m_expandednodes, m_nodecount, 
                  sizeof(wyWChar*), (wyInt32 (*)(const void *, const void *))Comparestring);

        if(retname)
        {
            TreeView_Expand(m_hwnd, hitemdb, TVE_EXPAND);
            ExpandTable(hitemdb, buff);
        }
    
        hitemdb = TreeView_GetNextSibling(m_hwnd, hitemdb);
    }
	free(nodename);
    return;
}

void
CQueryObject::ExpandTable(HTREEITEM hitemdb, wyWChar *nodename)
{
    TVITEMEX    item;
    HTREEITEM   hitemtable;
    wyWChar     buff[SIZE_512] = {0}, **retname;
	wyString	nodenameasstring(nodename);
	wyWChar		*nodetabledb = 0;
	wyString	buffstr, tempstr;
    
	hitemtable  = TreeView_GetChild(m_hwnd, hitemdb);
	
	nodetabledb = AllocateBuffWChar(wcslen(nodename) + SIZE_512 + 1);

    while(hitemtable)
    {
		item.hItem      = hitemtable;
        item.mask       = TVIF_TEXT | TVIF_IMAGE;
        item.cchTextMax	= SIZE_512 - 1;
	    item.pszText    = buff;

        TreeView_GetItem(m_hwnd, &item); 

        if(buff != NULL && item.iImage == NTABLE)
        {	
			swprintf(nodetabledb, L"%s/%s", buff, nodename);
			
			retname = (wyWChar **)bsearch((wyWChar *)(&nodetabledb), (wyWChar *)m_expandednodes, m_nodecount, 
                    sizeof(wyWChar*), (wyInt32 (*)(const void *, const void *))Comparestring);

            if(retname)
            {
                TreeView_Expand(m_hwnd, hitemtable, TVE_EXPAND);
				ExpandColumn(hitemtable, nodetabledb);
            }
        }
        hitemtable = TreeView_GetNextSibling(m_hwnd, hitemtable);
    }

    if(nodetabledb)
        free(nodetabledb);

    nodetabledb = NULL;
    return;
}

void
CQueryObject::ExpandColumn(HTREEITEM hitemtable, wyWChar *nodename)
{
    TVITEMEX    item;
    HTREEITEM   hitemcolumn;
    wyWChar     buff[SIZE_512] = {0}, **retname;
	wyWChar		*nodecoltabledb = 0;
    
	hitemcolumn = TreeView_GetChild(m_hwnd, hitemtable);
	
	nodecoltabledb = AllocateBuffWChar(wcslen(nodename) + SIZE_512 + 1);
	
    while(hitemcolumn)
    {	
		item.hItem      = hitemcolumn;
        item.mask       = TVIF_TEXT | TVIF_IMAGE;
        item.cchTextMax	= SIZE_512 - 1;
	    item.pszText    = buff;

        TreeView_GetItem(m_hwnd, &item); 

        if(buff != NULL)
        {
			swprintf(nodecoltabledb, L"%s/%s", buff, nodename);
			
			retname = (wyWChar **)bsearch((wyWChar *)(&nodecoltabledb), (wyWChar *)m_expandednodes, m_nodecount, 
                    sizeof(wyWChar*), (wyInt32 (*)(const void *, const void *))Comparestring);

            if(retname)
            {
                TreeView_Expand(m_hwnd, hitemcolumn, TVE_EXPAND);
            }
        }
        hitemcolumn = TreeView_GetNextSibling(m_hwnd, hitemcolumn);
    }

    if(nodecoltabledb)
        free(nodecoltabledb);

    nodecoltabledb = NULL;
    return;
}

void
CQueryObject::GetTreeState()
{
    HTREEITEM	hitemroot;
    wyInt32     count =0;

    for(count = 0; count < m_nodecount; count++);
    {
        if(m_expandednodes[count])
            free(m_expandednodes[count]);

        m_expandednodes[count] = NULL;
    }

    m_nodecount = 0;

	hitemroot = TreeView_GetRoot(m_hwnd);

    if(!hitemroot)
        return;
    
    GetAllExpandedNodes(hitemroot);
    return;
}

void
CQueryObject::GetAllExpandedNodes(HTREEITEM hitem)
{
    wyInt32     result;
    TVITEMEX    item;
    HTREEITEM   hitemchild;
    wyWChar      nodename[512]={0};
    wyString    nodedetails, nodenamestr;
    
    if(!hitem)
        return;

    item.hItem      = hitem;
    item.stateMask  = TVIS_EXPANDED;
    item.mask       = TVIF_STATE | TVIF_TEXT | TVIF_IMAGE;
    item.cchTextMax	= SIZE_512 - 1;
	item.pszText    = nodename;
	
    result = TreeView_GetItem(m_hwnd, &item); 

    nodenamestr.SetAs(nodename);

    if(item.state & TVIS_EXPANDED)
    {
        if(item.iImage == NDATABASE)
        {
			nodedetails.Sprintf("%s", nodenamestr.GetString());
        }
        else if(item.iImage == NTABLE)
        {
            nodedetails.Sprintf("%s", nodename);
            item.hItem = TreeView_GetParent(m_hwnd, hitem);
            result = TreeView_GetItem(m_hwnd, &item); 
			nodedetails.AddSprintf("/%s", nodenamestr.GetString());
        }
        else if(item.iImage == NFOLDER)
        {
			nodedetails.Sprintf("%s", nodenamestr.GetString());
            item.hItem = TreeView_GetParent(m_hwnd, hitem);
            result = TreeView_GetItem(m_hwnd, &item); 
			nodedetails.AddSprintf("/%s", nodenamestr.GetString());
            item.hItem = TreeView_GetParent(m_hwnd, TreeView_GetParent(m_hwnd, hitem));
            result = TreeView_GetItem(m_hwnd, &item); 
			nodedetails.AddSprintf("/%s", nodenamestr.GetString());
        }

        if(nodedetails.GetLength())
        {
            if(m_nodecount >= m_nodebuffsize)
            {
                m_nodebuffsize += DEF_NODE_BUFF_SIZE;
                m_expandednodes = (wyWChar**)realloc(m_expandednodes, sizeof(wyWChar*) * m_nodebuffsize);
            }
            m_expandednodes[m_nodecount] = (wyWChar*)calloc(sizeof(wyWChar), ((nodedetails.GetLength() + 1) * sizeof(wyWChar)));
           
			// sprintf(m_expandednodes[m_nodecount], "%s", (wyChar*)nodedetails.GetString());
				wcscpy(m_expandednodes[m_nodecount], nodedetails.GetAsWideChar());

            m_nodecount++;
        }

        hitemchild = TreeView_GetChild(m_hwnd, hitem);

	    if(!hitemchild)
		    return;

	    while(hitemchild != NULL)
	    {
            GetAllExpandedNodes(hitemchild);
		    hitemchild = TreeView_GetNextSibling(m_hwnd, hitemchild);
	    }
    }

    return;
}

// Function to store the info of the selected item so that when we refresh the object browser
// we get the selection back so that the user dosnt have to scroll again.
wyBool
CQueryObject::GetItemInfo()
{
	wyInt32		ret;
	wyWChar     database[SIZE_512] = {0}, table[SIZE_512] = {0};
	TVITEM		tvi;
	HTREEITEM	hselitem, hparent;

    m_seldatabase.Strip(m_seldatabase.GetLength());
    m_seltable.Clear();

	// Get the selected item.
	hselitem = TreeView_GetSelection(m_hwnd);
	if(!hselitem)
		return wyFalse;

	// now get the image type.
	tvi.mask	    = TVIF_IMAGE | TVIF_TEXT;
	tvi.hItem	    = hselitem;
	tvi.cchTextMax	= SIZE_512 - 1;
	tvi.pszText		= database;

	ret = TreeView_GetItem(m_hwnd, &tvi);

	if(!ret)
		return wyFalse;

	m_seltype	=	tvi.iImage;

	// now we check what type of selection it is.
	if(tvi.iImage == NDATABASE)
	{
		m_seldatabase.SetAs(database);
		m_seltable.Clear();

		return wyTrue;
	}
	else if((tvi.iImage == NINDEX)||(tvi.iImage == NPRIMARYINDEX)||(tvi.iImage == NCOLUMN) ||(tvi.iImage == NPRIMARYKEY) ||(tvi.iImage == NTRIGGERITEM)||
			(tvi.iImage == NVIEWSITEM)||(tvi.iImage == NFUNCITEM)||(tvi.iImage == NSPITEM))
	{
		if((tvi.iImage == NVIEWSITEM)||(tvi.iImage == NFUNCITEM)||(tvi.iImage == NSPITEM))
			hparent = TreeView_GetParent(m_hwnd, hselitem);
		else
			hparent = TreeView_GetParent(m_hwnd, TreeView_GetParent(m_hwnd, hselitem));

		if(!hparent)
			return wyFalse;

		// this is the table.
		// so get the info about table;
        ret = GetNodeText(m_hwnd, hparent, table, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seltable.SetAs(table);

		// now get the database.
		hparent = TreeView_GetParent(m_hwnd, hparent);
        ret = GetNodeText(m_hwnd, hparent, database, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seldatabase.SetAs(database);
	}
	else if(tvi.iImage == NTABLE || tvi.iImage == NVIEWS ||tvi.iImage == NFUNC ||tvi.iImage == NSP)
	{
		hparent = hselitem;

		// this is the table.
		// so get the info about table;
        ret = GetNodeText(m_hwnd, hparent, table, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seltable.SetAs(table);

		hparent = TreeView_GetParent(m_hwnd, hparent);
        ret = GetNodeText(m_hwnd, hparent, database, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seldatabase.SetAs(database);
	}
	else if(tvi.iImage == NFOLDER || tvi.iImage == NTRIGGER )
	{
		hparent = TreeView_GetParent(m_hwnd, hselitem);

		if(!hparent)
			return wyFalse;

        ret = GetNodeText(m_hwnd, hparent, table, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seltable.SetAs(table);

		// now get the database.
		hparent = TreeView_GetParent(m_hwnd, hparent);
        ret = GetNodeText(m_hwnd, hparent, database, SIZE_512);

		if(!ret)
			return wyFalse;

		m_seldatabase.SetAs(database);
	}
	else
		m_seldatabase.SetAs(database);
	
	return wyTrue;
}

wyBool
CQueryObject::RefreshParentNodeHelper()
{
	HTREEITEM hselitem = NULL;
	wyInt32	  image = 0;
	MDIWindow *wnd = NULL;
	wyBool		isindex = wyTrue;
	wyWChar		table[SIZE_512];
    
	wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);

	hselitem = TreeView_GetSelection(m_hwnd);
	//......................
	image = GetItemImage(m_hwnd, hselitem);
		
	switch(image)
	{
	case NPRIMARYKEY:
    case NCOLUMN:
	case NPRIMARYINDEX:
	case NINDEX:	
		//hselitem = TreeView_GetParent(m_hwnd, hselitem);
		return wyTrue;
		break;

    case NFOLDER:
		{
			if(GetNodeText(m_hwnd, hselitem, table, SIZE_512 - 1) == wyFalse)
				return wyFalse;
            			
			if(wcscmp(table, TXT_COLUMNS) || wcscmp(table, TXT_INDEXES))
			{
				isindex = wcscmp(table, TXT_INDEXES) == 0 ? wyTrue : wyFalse;
				break;
			}
		}
	default:
		return RefreshParentNode(hselitem);
	}
    
	//Refresh the 'Columns' and 'Indexes'
	RefreshIndexesOrColumnsFolder(m_hwnd, hselitem, image, wnd->m_tunnel, &wnd->m_mysql, m_seldatabase.GetString(), m_seltable.GetString(), isindex, wyTrue); 
	//TreeView_Expand(m_hwnd, hselitem, TVE_EXPAND); 

	return wyTrue;	
}

wyBool
CQueryObject::RefreshParentNode(HTREEITEM hselitem)
{
	wyInt32		ret;
	wyWChar     database[SIZE_512]={0};
	TVITEM		tvi, tvil;
	wyInt32		image, parentImage = 0;
	wyBool		index = wyFalse, isopentables = wyFalse;
	MDIWindow	*wnd;
	TREEVIEWPARAMS	treeviewparam = {0};
    wyWChar     filterText[70];
    HTREEITEM   hparent = NULL;
    OBDltElementParam *tempElem = NULL;

	
    memset(filterText, 0, sizeof(filterText));
    memset(&tvil, 0, sizeof(TVITEM));

    tvil.mask = TVIF_PARAM;

    if(!hselitem)
		return wyFalse;

	image = GetItemImage(m_hwnd, hselitem);
	index = IsSelectedIndex(hselitem); //selected node is "indexes" then index = wyTrue

	m_isnoderefresh = wyTrue;

	switch(image)
	{
	case NPRIMARYKEY:
    case NCOLUMN:
	case NPRIMARYINDEX:
	case NINDEX:
	    hselitem = TreeView_GetParent(m_hwnd, hselitem);
	    break;
    case NDATABASE:
        DeAllocateLParam(hselitem, image);
        break;
    case NSPITEM:
    case NEVENTITEM:
    case NFUNCITEM:
    case NTRIGGERITEM:
    case NVIEWSITEM:
    case NTABLE:
        hparent = TreeView_GetParent(m_hwnd, hselitem);
        tvil.hItem = hparent;
        TreeView_GetItem(m_hwnd, &tvil);
        tempElem = (OBDltElementParam *) tvil.lParam;
        if(tempElem)
            wcscpy(filterText, tempElem->m_filterText);
        parentImage = GetItemImage(m_hwnd, hparent);
        if(wcslen(filterText))
        {
            DeAllocateLParam(hparent, parentImage);
            m_prevString.SetAs(L"");
        }
        hselitem = hparent;
        break;        
    case NSP:
    case NEVENTS:
    case NFUNC:
    case NTRIGGER:
    case NVIEWS:
    case NTABLES:
        tvil.hItem = hselitem;
        TreeView_GetItem(m_hwnd, &tvil);
        tempElem = (OBDltElementParam *) tvil.lParam;
        if(tempElem)
            wcscpy(filterText, tempElem->m_filterText);
        if(wcslen(filterText))
        {
            DeAllocateLParam(hselitem, image);
            m_prevString.SetAs(L"");
        }
        break;
	}
	// now get the image type.
	tvi.mask	    = TVIF_IMAGE | TVIF_TEXT;
	tvi.hItem   	= hselitem;
	tvi.cchTextMax	= SIZE_512-1;
	tvi.pszText		= database;

	ret = TreeView_GetItem(m_hwnd, &tvi);

	if(!ret)
	{
		m_isnoderefresh = wyFalse;
        HandleOBFilter(filterText, wyFalse);
        m_isSelValid = wyFalse;
        SetWindowText(m_hwndFilter, filterText);
        return wyFalse;
	}

	isopentables = IsOpenTablesInOB();

	tvi.state = false;
	wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	
	treeviewparam.database = NULL;
	treeviewparam.hwnd = m_hwnd;
	if(tvi.iImage == NDATABASE)
		treeviewparam.isopentables = isopentables;
	else
		treeviewparam.isopentables = wyFalse;

	treeviewparam.isrefresh = m_isrefresh;
	treeviewparam.issingletable = wyFalse;
	treeviewparam.mysql = &wnd->m_mysql;
	treeviewparam.tunnel = wnd->m_tunnel;
	treeviewparam.tvi = &tvi;
	treeviewparam.checkboxstate = wyFalse;

	SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);
	m_isexpcollapse = OnItemExpanding(treeviewparam, m_isnoderefresh);

        	
	if(isopentables == wyFalse)
		SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
	
	if((image == NFOLDER && !index)|| (image == NCOLUMN) || (image == NPRIMARYKEY))
		hselitem = TreeView_GetChild(m_hwnd, hselitem);
	else if(image == NINDEX ||index || image == NPRIMARYINDEX) //if we are refreshing the indexes folder then index = true
	{
		hselitem = TreeView_GetChild(m_hwnd, hselitem);
		hselitem = TreeView_GetNextItem(m_hwnd, hselitem, TVGN_NEXT);
	}
		
	TreeView_SetItemState(m_hwnd, hselitem, TVIS_SELECTED, TVIS_STATEIMAGEMASK);

	if(isopentables == wyFalse)
		SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);
	
	ret = TreeView_Expand(m_hwnd, hselitem, TVE_EXPAND); 

	//If it is database and if we select open tables folder as default then we need to expand Tables folder.
	//issue reported here http://code.google.com/p/sqlyog/issues/detail?id=680
	if( image == NDATABASE && isopentables == wyTrue)
	{	
		ret = TreeView_Expand(m_hwnd, TreeView_GetChild(m_hwnd, hselitem), TVE_EXPAND); 
		TreeView_SelectItem(m_hwnd, hselitem);
	}

	//While refreshing 'Tables' folder re-organize the Triggeres also
	if(image == NTABLES)
		ReorderTriggers(TreeView_GetParent(m_hwnd, hselitem));

	SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);

	m_isnoderefresh = wyFalse;
	
    if(image == NDATABASE)
    {
        m_prevString.SetAs(L"");
        m_isSelValid = wyFalse;
        SetWindowText(m_hwndFilter, L"");
    }
    else if(wcslen(filterText))
    {
        HandleOBFilter(filterText, wyFalse);
        m_isSelValid = wyFalse;
        SetWindowText(m_hwndFilter, filterText);
    }
    
	if(ret)
        return wyTrue;

	return wyFalse;
}

// This function checks for various validation and puts the node into edit label mode.
// So that the user can change the name of the table.
wyBool
CQueryObject::RenameObject()
{
	wyInt32     item;
	HTREEITEM   hitem, temp;
    wyString    strtemp;
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
	
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	temp = hitem;
	item = GetSelectionImage();
	
	switch(item)
	{
	case NPRIMARYKEY:
	case NCOLUMN:
	case NPRIMARYINDEX:
	case NINDEX:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetTableDatabaseName(hitem);
		break;

	case NTABLE:
		GetTableDatabaseName(hitem);
		break;
	case NEVENTITEM:
	case NVIEWSITEM:
	case NSPITEM:
	case NFUNCITEM:
		{
			// Get the database name and View name.
			GetTableDatabaseName(hitem);
			strtemp.SetAs(m_seltable.GetString());
			m_seltable.SetAs(strtemp.GetString());
		}
		break;
	case NTRIGGERITEM:
		{
			GetTableDatabaseName(hitem);
			m_seltrigger.SetAs(m_seltable.GetString());			
		}
		break;

	default:
		return wyFalse;
	}

    m_AllowRename = wyTrue;
	VERIFY(TreeView_SelectItem(m_hwnd, temp)); 
	VERIFY(TreeView_EditLabel(m_hwnd, temp));
    m_AllowRename = wyFalse;
	return wyTrue;
}


// this funcion is called when the user has finished changing the name of the table.
// we execute a query to change the name and return as reuired.
wyBool
CQueryObject::EndRename(LPNMTVDISPINFO ptvdi)
{	
	wyInt32    item;
	wyInt32    ret=0;
    wyString   renametext;	
	HTREEITEM  hitem;
	wyWChar    buff[SIZE_512]; 
  

	if(ptvdi->item.pszText == NULL)
		return wyFalse;

    if(ptvdi->item.pszText)
        renametext.SetAs(ptvdi->item.pszText);
    
    if( renametext.GetLength() == 0)
        return wyFalse;
	hitem = TreeView_GetSelection(m_hwnd);

	if(GetNodeText(m_hwnd, hitem, buff, SIZE_512) == wyTrue)
		m_seltable.SetAs(buff);

    if(m_seltable.Compare(renametext.GetString()) == 0)
        return wyTrue;

	UpdateWindow(m_hwnd);
	item = GetSelectionImage();	
	
	switch(item)
	{
	case NTABLE:
		ret = EndRenameTable(ptvdi);
		break;
	
	case NVIEWSITEM:
		ret = EndRenameView(ptvdi);
		break;
	case NEVENTITEM:
		ret = EndRenameEvent(ptvdi);
		break;	
	case NTRIGGERITEM:
		ret = EndRenameTrigger(ptvdi);
		break;
	}
	
	

	if(ret)
	{
		m_seltable.SetAs(ptvdi->item.pszText);
	    return wyTrue;
	}
    return wyFalse;
}

// This function copies given table into a new table using MySQL inbuilt select command.
wyBool
CQueryObject::CopyTable(MDIWindow * wnd)
{

	wyInt32     item;
	CCopyTable  cpt;
	HTREEITEM   hitem;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

    HandlerToGetTableDatabaseName(item, hitem);
	
	cpt.Create(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, (wyChar*)m_seldatabase.GetString(), 
               (wyChar*)m_seltable.GetString(), hitem, TreeView_GetParent(m_hwnd, hitem));

	return wyTrue;
}

// Function initializes the Export Data dialog box with the parameter depending upon
// the selection.
wyBool
CQueryObject::ExportData(Tunnel * tunnel, PMYSQL mysql, TUNNELAUTH * auth, wyInt32 nid)
{
	wyInt32		item;
	HTREEITEM	hitem;
	ExportBatch cexp;
	
	VERIFY(hitem	= TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

	// now we have to see which of the selection has been made.
	switch(item)
	{
	case NDATABASE:
		GetDatabaseName(hitem);
		cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
		break;

	case NFOLDER:			//now we see which of the item was sent as id.
		switch(nid)
		{
		case ACCEL_EXPORTBATCH:
		case ID_IMPORTEXPORT_EXPORTTABLES:
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(hitem );			
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;

		case ID_IMPORTEXPORT_DBEXPORTTABLES:
		case ID_IMPORTEXPORT_DBEXPORTTABLES2:
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(hitem );
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
			break;

		case ID_IMPORTEXPORT_TABLESEXPORTTABLES:
			GetTableDatabaseName(TreeView_GetParent(m_hwnd, hitem));
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;
		}
		break;

	case NTABLE:			// now we see which of the item was sent as id.
		switch(nid)
		{
		case ACCEL_EXPORTBATCH:
		case ID_IMPORTEXPORT_EXPORTTABLES:
			GetTableDatabaseName(hitem );			
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;

		case ID_IMPORTEXPORT_TABLESEXPORTTABLES:
			GetTableDatabaseName(hitem);
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;

		case ID_IMPORTEXPORT_DBEXPORTTABLES:
		case ID_IMPORTEXPORT_DBEXPORTTABLES2:
			GetTableDatabaseName(hitem);
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
			break;
		}
		break;

	case NPRIMARYKEY:
	case NPRIMARYINDEX:
	case NINDEX:
	case NCOLUMN:
		switch(nid)
		{
		case ACCEL_EXPORTBATCH:
		case ID_IMPORTEXPORT_EXPORTTABLES:
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(hitem );			
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;

		case ID_IMPORTEXPORT_TABLESEXPORTTABLES:
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(hitem);
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), m_seltable.GetString());
			break;

		case ID_IMPORTEXPORT_DBEXPORTTABLES:
		case ID_IMPORTEXPORT_DBEXPORTTABLES2:
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetTableDatabaseName(hitem);
			cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
	
		}
		break;

	case NTABLES:
	case NVIEWS:
	case NFUNC:
	case NTRIGGER:
	case NEVENTS:
	case NSP:
		{
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetDatabaseName(hitem);
			switch(nid)
			{
			case ACCEL_EXPORTBATCH:
			case ID_IMPORTEXPORT_EXPORTTABLES:
			case ID_IMPORTEXPORT_DBEXPORTTABLES2:

                //if item is tables folder, check only tables
				if(item == NTABLES)
                    cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), NULL);
                else
				    cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");

				break;

			case ID_IMPORTEXPORT_TABLESEXPORTTABLES:

                //if item is tables folder, check only tables
                if(item == NTABLES)
                    cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), NULL);
                else
				    cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");

				break;

			case ID_IMPORTEXPORT_DBEXPORTTABLES:
				cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
				break;
			}		
		}
		break;

	case NVIEWSITEM:
	case NEVENTITEM:
	case NTRIGGERITEM:
	case NSPITEM:
    case NFUNCITEM:
		{
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
			GetDatabaseName(hitem);
			switch(nid)
			{
			case ACCEL_EXPORTBATCH:
			case ID_IMPORTEXPORT_EXPORTTABLES:
            case ID_IMPORTEXPORT_TABLESEXPORTTABLES:
			case ID_IMPORTEXPORT_DBEXPORTTABLES:
            case ID_IMPORTEXPORT_DBEXPORTTABLES2:
                cexp.Create(m_hwnd, tunnel, mysql, auth, m_seldatabase.GetString(), "");
				break;
			}		
		}
		break;
	default:
		cexp.Create(m_hwnd, tunnel, mysql, auth, "", "");
		break;
	}
	return wyTrue;
}

// This function initializes the Backup Database dialog box thru which the user can backup
// MyISAM tables.
wyBool
CQueryObject::BackUpDB(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32	    item;
	HTREEITEM   hitem;
	BackUp	    cback;

	if(tunnel->IsTunnel())
    {
		yog_message(m_hwnd, HTTPNOTWORKINGOPTIONS, pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		return wyTrue;
	}

	item = GetSelectionImage();
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	switch(item)
	{
	case NDATABASE:
		GetDatabaseName(hitem);
		cback.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;

	case NTABLE:			// now we see which of the item was sent as id.
		GetDatabaseName(TreeView_GetParent(m_hwnd, hitem));
		cback.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;

	case NINDEX:
	case NPRIMARYINDEX:
	case NCOLUMN:
	case NPRIMARYKEY:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(TreeView_GetParent(m_hwnd, hitem));
		cback.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;
	}	
	
	return wyTrue;
}

// This function initializes the Restore Database dialog box thru which the user can restore
// MyISAM tables.
wyBool
CQueryObject::RestoreDB(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32     item;
	HTREEITEM   hitem;
	CRestore    cres;

	if(tunnel->IsTunnel())
    {
		yog_message(m_hwnd, HTTPNOTWORKINGOPTIONS, pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		return wyTrue;
	}

	item = GetSelectionImage();
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	switch(item)
	{
	case NDATABASE:
		GetDatabaseName(hitem);
		cres.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;

	case NTABLE:			// now we see which of the item was sent as id.
		GetDatabaseName(TreeView_GetParent(m_hwnd, hitem));
		cres.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;

	case NINDEX:
	case NPRIMARYINDEX:
	case NCOLUMN:
	case NPRIMARYKEY:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(TreeView_GetParent(m_hwnd, hitem));
		cres.Create(m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());
		break;
	}	
	
	return wyTrue;
}

// This function initializes the import from CSV dialog box so that the user can import data from a CSV file 
wyBool
CQueryObject::ImportFromCSV(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32				item;
	HTREEITEM			hitem;
	ExportMultiFormat	cisv;

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

	/* stasrting from v4.2 we dont allow import csv in tunneling mode as its not possible
	   to do that */
	if(tunnel->IsTunnel())
    {
		yog_message(m_hwnd, CSVTUNNELIMPORTERROR, pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		return wyTrue;
	}
	
    if(HandlerToGetTableDatabaseName(item, hitem) == wyFalse)
        return wyFalse;

	cisv.Create(m_hwnd, (wyChar*)m_seldatabase.GetString(), (wyChar*)m_seltable.GetString(), tunnel, mysql);

	return wyTrue;
}
// This function initializes the import from XML dialog box so that the user can import data from a XML file 
wyBool
CQueryObject::ImportFromXML(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32				item;
	HTREEITEM			hitem;
	ExportMultiFormat	cisv;

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

	/*  we dont allow import XML in tunneling mode as its not possible
	   to do that */
	if(tunnel->IsTunnel())
    {
		yog_message(m_hwnd, CSVTUNNELIMPORTERROR, pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		return wyTrue;
	}
	
    if(HandlerToGetTableDatabaseName(item, hitem) == wyFalse)
        return wyFalse;

	cisv.CreateXML(m_hwnd, (wyChar*)m_seldatabase.GetString(), (wyChar*)m_seltable.GetString(), tunnel, mysql);

	return wyTrue;
}

// This function selects the table and database name and stores it in the member 
// variable of the object browser. It returns the Database node handle
HTREEITEM
CQueryObject::GetTableDatabaseName(HTREEITEM hitemtable)
{
	wyInt32		ret;
	TVITEM		tvi;
    wyWChar     database[SIZE_512] = {0}, table[SIZE_512] = {0};
	wyBool		retval;
	HTREEITEM   hitemdb = NULL;
	
	// get the original table name.
	tvi.mask        = TVIF_TEXT;
	tvi.hItem       = hitemtable;
	tvi.pszText     = table;
	tvi.cchTextMax  = SIZE_512 - 1;

	ret = TreeView_GetItem(m_hwnd, &tvi);
	_ASSERT(ret);
	if(ret == 0)
		return NULL;

	VERIFY(hitemdb = TreeView_GetParent(m_hwnd, TreeView_GetParent(m_hwnd, hitemtable)));
	
	if(!hitemdb)
		return NULL;

	retval = GetNodeText(m_hwnd, hitemdb, database, SIZE_512);
	if(retval == wyFalse)
		return NULL;

    m_seltable.SetAs(table);
    m_seldatabase.SetAs(database);

	return hitemdb;	
}

// This function selects the database name and stores it in the member variable of the object browser.
// The handle to the database item is passed as a parameter
wyBool
CQueryObject::GetDatabaseName(HTREEITEM hitemtable)
{
	wyInt32     ret;
	TVITEM      tvi;
    wyWChar      database[SIZE_512] = {0};
	
	// get the original table name.
	tvi.mask        = TVIF_TEXT;
	tvi.hItem       = hitemtable;
	tvi.pszText     = database;
	tvi.cchTextMax  = SIZE_512 - 1;

	VERIFY(ret = TreeView_GetItem(m_hwnd, &tvi));
	
	if(ret == 0)
		return wyFalse;

    m_seldatabase.SetAs(database);

	return wyTrue;	
}

// This function returns the index of the image of the selected node.
// This return value is used by a lot of function when we have to check what type of node
// the selection is.
wyInt32
CQueryObject::GetSelectionImage()
{
	HTREEITEM	hitem;

	hitem = TreeView_GetSelection(m_hwnd);
	return GetItemImage(m_hwnd, hitem);
}

HTREEITEM
CQueryObject::GetSelectionOnFilter(HWND hwnd)
{
        wyInt32 image = GetSelectionImage();
        HTREEITEM hti = TreeView_GetSelection(hwnd);
        switch(image)
        {
        case NTABLES:
        case NVIEWS:
        case NSP:
        case NTRIGGER:
        case NFUNC:
        case NEVENTS:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            TreeView_Expand(hwnd, hti, TVE_EXPAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        case NCOLUMN:
        case NINDEX:
        case NPRIMARYINDEX:
        case NPRIMARYKEY:
            hti = TreeView_GetParent(hwnd, hti);
            hti = TreeView_GetParent(hwnd, hti);
            hti = TreeView_GetParent(hwnd, hti);
            break;
        case NFOLDER:
            hti = TreeView_GetParent(hwnd, hti);
            hti = TreeView_GetParent(hwnd, hti);
            break;
        case NDATABASE:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            TreeView_Expand(hwnd, hti, TVE_EXPAND);
            hti = TreeView_GetChild(hwnd, hti);
            if(hti)
            {
                TreeView_Expand(hwnd, hti, TVE_EXPAND);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        case NTABLE:
        case NVIEWSITEM:
        case NTRIGGERITEM:
        case NFUNCITEM:
        case NEVENTITEM:
        case NSPITEM:
            hti = TreeView_GetParent(m_hwnd, hti);
            break;
        }
        return hti;
}


// The function is called when the user presses Insert in ObjectBrowser.
// We check on which type of node it is pressed and then process accordingly.
wyBool
CQueryObject::ProcessInsert()
{
	wyInt32	    image;
    MDIWindow*  wnd = GetActiveWin();

	image = GetSelectionImage();

	switch(image)
	{
	case NTABLE:
        if(wnd->GetActiveTabEditor())
		    CreateInsertStmt();
		break;

	case NDATABASE:
		CreateTableMaker();
		break;
	}
	
	return wyTrue;
}

// The function is called when a user presses Delete in ObjectBrowser.
// We check on whcih type of node it is pressed and also whether SHIFT is pressed 
// and then process accordingly.
wyBool
CQueryObject::ProcessDelete(wyBool isshiftpressed)
{
	wyInt32     image;
	MDIWindow*	wnd;

	VERIFY(wnd = GetActiveWin());

	image = GetSelectionImage();

	if(isshiftpressed)
	{
		if(image == NTABLE)
			EmptyTable(wnd->m_tunnel, &wnd->m_mysql);
		else if(image == NDATABASE)
			TruncateDatabase(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql);
	}
	else
	{
		switch(image)
		{
		case NDATABASE:
			DropDatabase(wnd->m_tunnel, &wnd->m_mysql);
			break;

		case NTABLE:
			DropTable(wnd->m_tunnel, &wnd->m_mysql);
			break;

		case NCOLUMN:
		case NPRIMARYKEY:
			DropField(wnd->m_tunnel, &wnd->m_mysql);
			break;

		case NINDEX:
		case NPRIMARYINDEX:
			DropIndex(wnd->m_tunnel, &wnd->m_mysql);
			break;

		case NVIEWSITEM:			
			DropDatabaseObject(wnd->m_tunnel, &wnd->m_mysql, "view");
			break;

		case NSPITEM:			
            DropDatabaseObject(wnd->m_tunnel, &wnd->m_mysql, "procedure");
			break;

		case NFUNCITEM:		
			DropDatabaseObject(wnd->m_tunnel, &wnd->m_mysql, "function");
			break;
		case NEVENTITEM:
			DropDatabaseObject(wnd->m_tunnel, &wnd->m_mysql, "event");
			break;
		case NTRIGGERITEM:
			DropTrigger(wnd->m_tunnel, &wnd->m_mysql);
			break;
		}
	}
	
	return wyTrue;
}

// The function is called when a user presses F2 in ObjectBrowser.
// We check on whcih type of node it is pressed and then process accordingly.
wyBool
CQueryObject::ProcessF2()
{
	wyInt32	   image;
	
	image = GetSelectionImage();
	
	switch(image)
	{
	case NTABLE:
	case NVIEWSITEM:
	case NEVENTITEM:
	case NTRIGGERITEM:
		this->RenameObject();
		break;
	}

	return wyTrue;
}
// The function is called when a user presses F6 in ObjectBrowser.
// We check on which type of node it is pressed and then process accordingly.
wyBool
CQueryObject::ProcessF6()
{	
	wyInt32		image;
	MDIWindow	*wnd;
	
	VERIFY(wnd = GetActiveWin());
	image = GetSelectionImage();
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
	switch(image)
	{
	case NDATABASE:
		pGlobals->m_pcmainwin->OnAlterDatabase(m_hwnd,wnd);
		break;

	case NCOLUMN:
	case NPRIMARYKEY:
        wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue, 0);
        break;

	case NTABLE:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue);
		break;
	
	case NFOLDER:
        {
            wyWChar         item[SIZE_512] = {0};
            wyString        itemstr("");

            GetNodeText(m_hwnd, TreeView_GetSelection(m_hwnd), item, SIZE_512 - 1);

            if(item)
                itemstr.SetAs(item);

            if(itemstr.CompareI(_("Columns")) == 0)
                wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue, 0);
        }
		break;

	case NVIEWSITEM:
		pGlobals->m_pcmainwin->OnAlterView(wnd);
		break;	
	case NSPITEM:
		pGlobals->m_pcmainwin->OnAlterProcedure(wnd);
		break;
	case NFUNCITEM:
		pGlobals->m_pcmainwin->OnAlterFunction(wnd);
		break;
	case NTRIGGERITEM:
		pGlobals->m_pcmainwin->OnAlterTrigger(wnd);
		break;
	case NEVENTITEM:
		pGlobals->m_pcmainwin->OnAlterEvent(wnd);
		break;
	case NINDEX:
	case NPRIMARYINDEX:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue, TABINDEXES);
		break;
    }
	return wyTrue;
}

// The function is called when a user presses F4 in ObjectBrowser.
// We check on which type of node it is pressed and then process accordingly.
wyBool
CQueryObject::ProcessF4()
{	
	wyInt32		image;
	MDIWindow	*wnd;
	
	VERIFY(wnd = GetActiveWin());
	image = GetSelectionImage();
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
	switch(image)
	{
	case NDATABASE:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd);
		break;

	case NCOLUMN:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd);
		break;
	case NPRIMARYKEY:
        wnd->m_pctabmodule->CreateTableTabInterface(wnd);
        break;
	case NTABLES:
	case NTABLE:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd);
		break;
	
	case NFOLDER:
        //{
        //    wyWChar         item[SIZE_512] = {0};
        //    wyString        itemstr("");

        //    GetNodeText(m_hwnd, TreeView_GetSelection(m_hwnd), item, SIZE_512 - 1);

        //    if(item)
        //        itemstr.SetAs(item);

        //    if(itemstr.CompareI(_("Columns")) == 0)
        //        wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue, 0);
        //}
		//break;
		wnd->m_pctabmodule->CreateTableTabInterface(wnd);
		break;
	case NVIEWS:
	case NVIEWSITEM:
		pGlobals->m_pcmainwin->OnCreateView(wnd->m_hwnd, wnd);
		break;	
	case NSP:
	case NSPITEM:
		pGlobals->m_pcmainwin->OnCreateProcedure(wnd->m_hwnd, wnd);
		break;
	case NFUNC:
	case NFUNCITEM:
		pGlobals->m_pcmainwin->OnCreateFunction(wnd->m_hwnd, wnd);
		break;
	case NTRIGGER:
	case NTRIGGERITEM:
		pGlobals->m_pcmainwin->OnCreateTrigger(wnd->m_hwnd, wnd);
		break;
	case NEVENTS:
	case NEVENTITEM:
		pGlobals->m_pcmainwin->OnCreateEvent(wnd->m_hwnd, wnd);
		break;
	case NINDEX:
	case NPRIMARYINDEX:
		wnd->m_pctabmodule->CreateTableTabInterface(wnd, wyTrue, TABINDEXES);
		break;
		//wnd->m_pctabmodule->CreateTableTabInterface(wnd);
    }
	return wyTrue;
}
// The function is called when a user Enter Delete in ObjectBrowser.
// We check on whcih type of node it is pressed and also whether SHIFT is pressed 
// and then process accordingly.
wyBool
CQueryObject::ProcessReturn(wyBool isshiftpressed)
{
	wyInt32     image, imageid;
	MDIWindow*	wnd;
    HTREEITEM   hti = NULL;
	
	wnd     = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	imageid = wnd->m_pctabmodule->GetActiveTabImage();

	_ASSERT(wnd);
    
    image = GetSelectionImage();
    hti = TreeView_GetSelection(m_hwnd);

    switch(image)
    {
    case NDATABASE:
    case NTABLES:
    case NSP:
    case NVIEWS:
    case NFUNC:
    case NTRIGGER:
    case NEVENTS:
        {
            if(hti)
            {
                TreeView_Expand(m_hwnd, hti, TVE_EXPAND);
                hti = TreeView_GetChild(m_hwnd, hti);
                if(hti)
                {
                    TreeView_SelectItem(m_hwnd, hti);
                    SetFocus(m_hwnd);
                }
            }
        }
    }


	if((imageid == IDI_QUERYBUILDER_16 || imageid == IDI_SCHEMADESIGNER_16) && isshiftpressed == wyFalse)
	{
		ProcessTable();  
		return wyTrue;		
	}		

	else
	{	
		switch(image)
		{
		case NTABLE:
			if(isshiftpressed)
				AdvProperties(wnd->m_tunnel, &wnd->m_mysql);
			else
				wnd->m_pctabmodule->CreateTabDataTab(wnd, wyFalse, wyTrue);
			break;
		}
		return wyTrue;
	}	
 }

// The function is for showing the update / insert data into a table dialog box.
// User can edit and add some data on the selected table.
wyBool
CQueryObject::ShowTable(wyBool isnewtab)
{
	wyInt32         item;
	HTREEITEM       hitem;
	MDIWindow*      wnd;
    	
	wnd = GetActiveWin();

	if(!wnd || !wnd->m_pctabmodule)
    {
		return wyFalse;
    }
    	
	hitem = TreeView_GetSelection(m_hwnd);
	item = GetSelectionImage();
	
	switch(item)
	{
	    case NTABLE:
	    case NVIEWSITEM:
            wnd->m_pctabmodule->CreateTabDataTab(wnd, isnewtab, wyTrue);
            break;

	    default:
		    return wyFalse;
	}	
	
	return wyTrue;
}

// this functions calls the advance properties dialog box to show the properties of a column.
wyBool
CQueryObject::AdvProperties(Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32     item;
	CAdvProp    cadvprop;
	HTREEITEM   hitem;
	MDIWindow*  wnd;
	
	VERIFY(wnd = GetActiveWin());

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

    if(item == NFOLDER || HandlerToGetTableDatabaseName(item, hitem) == wyFalse )
        return wyFalse;
	
	cadvprop.Create(wnd->m_hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString(), (wyChar*)m_seltable.GetString());
	SetFocus(m_hwnd);
	return wyTrue;
}

// Function to create the schema window.
wyBool
CQueryObject::CreateSchema()
{
	wyInt32     item;
	HTREEITEM   hitem;
	MDIWindow   *wnd;
	CSchema	    cs;

	VERIFY(wnd  = GetActiveWin());
	item    = GetSelectionImage();
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	switch(item)
	{
	case NDATABASE:
		GetDatabaseName(hitem);
		cs.Build(this, wnd->m_tunnel, &wnd->m_mysql, hitem);
		break;

	case NFUNCITEM:
	case NTRIGGERITEM:
	case NSPITEM:
	case NVIEWSITEM:
	case NEVENTITEM:
	case NTABLE:			// now we see which of the item was sent as id.
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		cs.Build(this, wnd->m_tunnel, &wnd->m_mysql, hitem);
		break;

	case NTABLES:
	case NVIEWS:
	case NTRIGGER:
	case NSP:
	case NFUNC:
	case NEVENTS:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		cs.Build(this, wnd->m_tunnel, &wnd->m_mysql, hitem);
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		cs.Build(this, wnd->m_tunnel, &wnd->m_mysql, hitem);
		break;
	
	case NINDEX:
	case NPRIMARYINDEX:
	case NCOLUMN:
	case NPRIMARYKEY:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		GetDatabaseName(hitem);
		cs.Build(this, wnd->m_tunnel, &wnd->m_mysql, hitem);
		break;
	
	}	

	return wyTrue;
}

LRESULT
CQueryObject::OnDBLClick(WPARAM wParam, LPARAM lParam)
{
	wyInt32         image;
	POINT           pnt;
	TVHITTESTINFO   tvh;
	MDIWindow*      wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	_ASSERT(wnd);
	TabObject		*pinfotab;

	pnt.x = GET_X_LPARAM(lParam);
	pnt.y = GET_Y_LPARAM(lParam);

	// set the hittest info structure.
	memset(&tvh, 0, sizeof(tvh));
	tvh.pt = pnt;
	TreeView_HitTest(m_hwnd, &tvh);

	if(tvh.hItem == NULL)
		return wyFalse;

	// make sure the item is selected.
	TreeView_SelectItem(m_hwnd, tvh.hItem);

	image = GetSelectionImage();
	
	switch(image)
	{
	case NDATABASE:
		GetDatabaseName(tvh.hItem);
		
		if(wnd->m_pctabmodule->GetActiveTabImage() == IDI_TABLEINDEX)
		{	
			pinfotab = (TabObject*) wnd->m_pctabmodule->GetActiveTabType();
			pinfotab->m_pobjinfo->Refresh();
		}
		break;
	}

	return wyTrue;
}

// Function to change the table type of a selected table to a different table type.
wyBool
CQueryObject::ChangeTableType(Tunnel * tunnel, PMYSQL mysql, const wyWChar *newtabletype)
{
	wyInt32			item = 0, isintransaction = 1;
	wyString		query;
	HTREEITEM		hitem;
    MYSQL_RES		*res = NULL;
	wyString		newtabtypestr;

	if(newtabletype)
		newtabtypestr.SetAs(newtabletype);

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	item = GetSelectionImage();

    if(item == NFOLDER || HandlerToGetTableDatabaseName(item, hitem) == wyFalse)
        return wyFalse;

	// change the cursor to hour glass.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	if(IsMySQL41(tunnel, mysql))
		query.Sprintf("alter table `%s`.`%s` engine = %s", m_seldatabase.GetString(), m_seltable.GetString(), newtabtypestr.GetString());
	else
		query.Sprintf("alter table `%s`.`%s` type = %s", m_seldatabase.GetString(), m_seltable.GetString(), newtabtypestr.GetString());
    		
    res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	tunnel->mysql_free_result(res);
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	return wyTrue;
}


//Function initializes the copy database dialog.
// Easy way to copy database between similar as well as different hosts.
wyBool
CQueryObject::CopyDB()
{
	wyInt32         image;
	wyBool          ret;
	CopyDatabase    *creord = NULL;
	MDIWindow       *wnd;
	HTREEITEM       hitem;

	VERIFY(wnd = GetActiveWin());
	
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	image = GetSelectionImage();

	switch(image)
	{
	case NTABLES:
	case NVIEWS:
	case NSP:
	case NFUNC:
	case NEVENTS:
	case NTRIGGER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem ));
		break;
	
	case NCOLUMN:
	case NINDEX:
	case NPRIMARYINDEX:
	case NPRIMARYKEY:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, TreeView_GetParent(m_hwnd, hitem)));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;
	
	case NTABLE:
	case NTRIGGERITEM:
	case NVIEWSITEM:
	case NFUNCITEM:
	case NSPITEM:
	case NEVENTITEM:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NSERVER:
		return wyTrue;

	default:
		break;
	}

	GetDatabaseName(hitem);

	creord = new CopyDatabase;
	if(!creord)
		return wyFalse;
	ret = creord->Create(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, (wyChar*)m_seldatabase.GetString());
	if(creord)
		delete creord;

	return wyTrue;
}

// Function initializes the copy database dialog.
// Easy way to copy database between similar as well as different hosts.
wyBool
CQueryObject::CopyTableToDiffertHostDB()
{
	wyInt32         image;
	wyBool          ret;
	CopyDatabase   creord;
	MDIWindow	    *wnd;
	HTREEITEM       hitem;

	VERIFY(wnd = GetActiveWin());
	
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	image = GetSelectionImage();

	if(HandlerToGetTableDatabaseName(image, hitem) == wyFalse)
        return wyFalse;

    //if the selected item is Tables folder, then create CopyDB dialog with the flag to check only tables
    if(image == NTABLES)
        ret = creord.Create(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, (wyChar*)m_seldatabase.GetString(), NULL, wyTrue);
    else
        ret = creord.Create(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, (wyChar*)m_seldatabase.GetString(), (wyChar*)m_seltable.GetString());

	return wyTrue;
}

// Function to empty the database i.e. drop all the tables in the database.
wyBool
CQueryObject::EmptyDatabase(HWND hwnd, Tunnel * tunnel, PMYSQL mysql)
{
	wyInt32     ret, image;
	HTREEITEM	hitem, hdbitem;
    wyString    query;
    EmptyDB     emptydb;
	wyInt32		state;
	   
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	image = GetSelectionImage();
	state = TreeView_GetItemState(m_hwnd, hitem, TVIS_USERMASK);

   	HandlerToGetDatabaseName(image, hitem);
	
    ret = emptydb.Create(hwnd, tunnel, mysql, (wyChar*)m_seldatabase.GetString());

	if(ret == 0)
		return wyFalse;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);
	m_seltable.Clear();
	m_seltype = NDATABASE;
    //GetTreeState();
	//RefreshObjectBrowser(tunnel, mysql);
	// RestoreTreeState();

	//GetDataBaseNode() will return NULL if the selected node is a Database,then we are assigning the selected node
	hdbitem = GetDatabaseNode();
	if(!hdbitem)
		hdbitem = hitem;

	state = TreeView_GetItemState(m_hwnd, hdbitem, TVIS_USERMASK);

	RefreshParentNode(hdbitem);
	if(!(state & TVIS_EXPANDED))
		SendMessage(m_hwnd, TVM_EXPAND, (WPARAM)TVE_COLLAPSE, (LPARAM)hdbitem);

	SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);   
	SetCursor(LoadCursor(NULL, IDC_ARROW));
    return wyTrue;
}

// Function to truncate the database i.e. empty all the tables in the database.
wyBool
CQueryObject::TruncateDatabase(HWND hwnd, Tunnel * tunnel, PMYSQL mysql)
{
	wyString    query, msg, myrowstr;
	wyInt32	    ret, image, count;
	HTREEITEM   hitem;
	MYSQL_RES	*myres; 
	MYSQL_ROW	myrow;
    MDIWindow   *wnd = GetActiveWin();
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyBool      flag = wyFalse, istunnel = wyFalse, retval = wyTrue;
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	image = GetSelectionImage();
	HandlerToGetDatabaseName(image, hitem);

    msg.Sprintf(_("Do you really want to truncate data for all table(s) in the database (%s)?\n\nWarning: You will lose all data!"), 
                 m_seldatabase.GetString());

	ret = yog_message(hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

	if(ret != IDYES)
		return wyFalse;
	

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	if(tunnel->IsTunnel() == true)
		istunnel = wyTrue;

	/*For http tunnel, its not requerd, because with each statement it must handle 
	so it handles in function "TunnelEnt::mysql_real_query()"
	*/
	if(istunnel == wyFalse)
	{
		//For direct connection setting this once is enough
		query.SetAs("set @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS");	

		myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
		if(!myres &&  tunnel->mysql_affected_rows(*mysql) == -1)
			return wyFalse;
		if(myres)
			tunnel->mysql_free_result(myres);

		query.SetAs("set FOREIGN_KEY_CHECKS = 0");

		myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
		if(!myres &&  tunnel->mysql_affected_rows(*mysql) == -1)
			return wyFalse;
		if(myres)
			tunnel->mysql_free_result(myres);
	}

	// get all the table names.	
	count = PrepareShowTable(tunnel, mysql, m_seldatabase, query);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		goto end;	
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("truncate table `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        //res = ExecuteAndGetResult(wnd, tunnel, mysql, query);
		
		//Executes the TRUNCATE TABLE query
		retval = HandleExecuteQuery(tunnel, mysql, query, istunnel);
		
		if(retval == wyFalse)
		{
			flag = wyFalse;
			tunnel->mysql_free_result(myres);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			goto end;
		}        
	}

	tunnel->mysql_free_result(myres);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	flag = wyTrue;

end:
	//Again revert back the SET varibles with direct connection
	if(istunnel == wyFalse)
	{
		query.SetAs("set FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS");
		myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
		if(!myres &&  tunnel->mysql_affected_rows(*mysql) == -1)
			return wyFalse;
		if(myres)
			tunnel->mysql_free_result(myres);
	}

    //issue reported here http://code.google.com/p/sqlyog/issues/detail?id=982
	//select a table then select  "Tabledata" tab, truncate the database, it is not refreshing in "Tabledata" tab.
    /*ptabeditor = wnd->GetActiveTabEditor();	
    if(ptabeditor)
	{
		TabMgmt *ptabmgmt = ptabeditor->m_pctabmgmt;
		if(ptabmgmt->GetSelectedItem() == IDI_TABLE && (ptabmgmt->m_insert->m_db.Compare(m_seldatabase.GetString()) == 0)) 
			ptabmgmt->m_insert->OnRefresh();
	}*/

	return flag;
}

// Function to bring the selected item in the object browser in sync.
wyBool
CQueryObject::SyncObjectDB(MDIWindow* wnd)
{
	wyString  query;
    MYSQL_RES *res;

	if(strcmp(wnd->m_database.GetString(), m_seldatabase.GetString()) != 0)
    {
		if(m_seldatabase.GetLength() == 0)
			return wyTrue;

		query.Sprintf("use `%s`", m_seldatabase.GetString());
        res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
        if(res)
            wnd->m_tunnel->mysql_free_result(res);

		wnd->m_database.SetAs(m_seldatabase.GetString());
	    pGlobals->m_pcmainwin->AddTextInCombo(m_seldatabase.GetAsWideChar());
	} 
	
	return wyTrue;
}

wyBool
CQueryObject::SyncObjectDBNocombo(MDIWindow* wnd)
{
	wyString  query;
    MYSQL_RES *res;

	if(strcmp(wnd->m_database.GetString(), m_seldatabase.GetString()) != 0)
    {
		if(m_seldatabase.GetLength() == 0)
			return wyTrue;

		query.Sprintf("use `%s`", m_seldatabase.GetString());
        res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
        if(res)
            wnd->m_tunnel->mysql_free_result(res);

		wnd->m_database.SetAs(m_seldatabase.GetString());
	    //pGlobals->m_pcmainwin->AddTextInCombo(m_seldatabase.GetAsWideChar());
	} 
	
	return wyTrue;
}

/* Function to drop all tables */
wyInt32
CQueryObject::DropTables(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	MYSQL_RES   *myres;
	MYSQL_ROW   myrow;
	wyString    query, dbnamestr;
    wyString	myrowstr;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41), istunnel = wyFalse, retval = wyTrue;
	wyInt32     count = 0;                                       // no of deleted objects
	
	dbnamestr.SetAs(db);
    PrepareShowTable(tunnel, mysql, dbnamestr, query);

	istunnel = (tunnel->IsTunnel() == true) ? wyTrue : wyFalse;

    myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
	
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return  count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop table `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
				
		//Execute the DROP table statement
		retval = HandleExecuteQuery(tunnel, mysql, query, istunnel);
		
		if(retval == wyFalse)
		{
			tunnel->mysql_free_result(myres);
			return count;
		}
		
		count++;		
	}

	tunnel->mysql_free_result(myres);
	
	//post 8.01
	//RepaintTabModule();
	return count;
}

/*
This wapper added when disabled FK-CHECK during http.
With http each query send as Batch, includes Set SQL-MODE, and SET FK-CHECK = 0
*/
wyBool		
CQueryObject::HandleExecuteQuery(Tunnel *tunnel, PMYSQL mysql, wyString &query, wyBool istunnel)
{
	MYSQL_RES   *res;
	wyBool force, batch, isfkchbatch;
	wyInt32 isintransaction = 1;

	force = batch = isfkchbatch = ((istunnel == wyTrue) ? wyTrue : wyFalse);
		
	//For http batch = true, force = true, isimporthtttp = true(thia makes SET SQL-MODE and SET FK-CHECK add to each script)
	res = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, batch, wyTrue, 
		                      false, force, wyFalse, 0, isfkchbatch, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		return  wyFalse;
	}
	

	if(res)
		tunnel->mysql_free_result(res);

	return  wyTrue;

}

/* Function to drop all views */
wyInt32
CQueryObject::DropViews(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	MYSQL_RES   *myres;
	MYSQL_ROW   myrow;
	wyString    query, myrowstr;
    MYSQL_RES   *res;
    MDIWindow   *wnd = GetActiveWin();
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyInt32     count = 0, isintransaction = 1;
	wyString	dbname(db);

    GetSelectViewStmt(db, query);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop view `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        res = ExecuteAndGetResult(wnd, tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return count;

		if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
		{
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
			return count;
		}
		count++;
		tunnel->mysql_free_result(res);
	}
	tunnel->mysql_free_result(myres);
	
	//Post 8.01
	//RepaintTabModule();
	return count;
}

/* Function to drop all procedures if the mysql version is greater than/equal to 5.0.10 */
wyInt32
CQueryObject::DropProcedures(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	MYSQL_RES	*myres, *res;
	MYSQL_ROW	myrow;
	wyString    query, myrowstr;
	wyInt32     count = 0, isintransaction = 1;
    MDIWindow   *wnd = GetActiveWin();
	wyString	dbname(db);
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyBool		iscollate = wyFalse;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	GetSelectProcedureStmt(db, query, iscollate);
	myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop procedure `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        res = ExecuteAndGetResult(wnd, tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return count;
		if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
		{
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
			return count;
		}
		count++;
		tunnel->mysql_free_result(res);
	}
	tunnel->mysql_free_result(myres);
	
	//Post 8.01
	//RepaintTabModule();
	return count;
}
/* Function to drop all events if the mysql version is greater than/equal to 5.1.6 */
wyInt32
CQueryObject::DropEvents(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	wyInt32     count = 0, isintransaction = 1;
	MYSQL_RES   *myres, *res;
	MYSQL_ROW   myrow;
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyString    query;
    MDIWindow   *wnd = GetActiveWin(); 
	wyString    dbnameasstring(db), myrowstr;
	wyBool		iscollate = wyFalse;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	GetSelectEventStmt(db, query, iscollate);
	
    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);

	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{	
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop event `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        res = ExecuteAndGetResult(wnd, tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return count;

		if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
		{
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
			return count;
		}

		tunnel->mysql_free_result(res);
	}
	tunnel->mysql_free_result(myres);
	
	//Post 8.01
	//RepaintTabModule();
	return count;
}

/* Function to drop all functions if the mysql version is greater than/equal to 5.0.10 */
wyInt32
CQueryObject::DropFunctions(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	wyInt32     count = 0, isintransaction = 1;
	MYSQL_RES   *myres, *res;
	MYSQL_ROW   myrow;
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyString    query;
    MDIWindow   *wnd = GetActiveWin();
	wyString    dbnameasstring(db), myrowstr;
	wyBool		iscollate = wyFalse;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	GetSelectFunctionStmt(db, query, iscollate);
	
    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);

	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{	
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop function `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        res = ExecuteAndGetResult(wnd, tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return count;

		if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
		{
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
			return count;
		}

		tunnel->mysql_free_result(res);
	}
	tunnel->mysql_free_result(myres);
	
	//Post 8.01
	//RepaintTabModule();
	return count;
}


wyBool 
CQueryObject::GetCreateProcedure(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strprocedure)
{
	HTREEITEM		hitem;
	wyString        strmsg, query;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	if(!GetCreateProcedureString(tunnel, mysql, m_seldatabase.GetString(), m_seltable.GetString(), strprocedure, strmsg, &query))
	{
		//mysql error
		if(tunnel->mysql_errno(*mysql) != 0)
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		else if(tunnel->mysql_errno(*mysql) == 0 && strmsg.GetLength() != 0) //sqlyog's error msg
			ShowErrorMessage(m_hwnd, query.GetString(), strmsg.GetString());

		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	return wyTrue;
}


wyBool   
CQueryObject::GetCreateFunction(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strfunction)
{
	HTREEITEM	hitem;
	wyString    strmsg, query;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	if(!GetCreateFunctionString(tunnel, mysql, m_seldatabase.GetString(), m_seltable.GetString(), strfunction, strmsg, &query))
	{
		//mysql error msg
		if(tunnel->mysql_errno(*mysql) != 0)
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		else if(tunnel->mysql_errno(*mysql) == 0 && strmsg.GetLength() != 0) //sqlyog's error message
			ShowErrorMessage(m_hwnd, query.GetString(), strmsg.GetString());
		
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	
    return wyTrue;
}

wyBool   
CQueryObject::GetDropFunction(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strfunction)
{
	HTREEITEM	hitem;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	strfunction.Sprintf("USE `%s`$$\r\n\r\n", m_seldatabase.GetString());
	strfunction.AddSprintf("DROP FUNCTION IF EXISTS `%s`$$\r\n\r\n", m_seltable.GetString());
	
	return wyTrue;
}

wyBool   
CQueryObject::GetDropProcedure(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strprocedure)
{
	HTREEITEM	hitem;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	strprocedure.Sprintf("USE `%s`$$\r\n\r\n", m_seldatabase.GetString());
	strprocedure.AddSprintf("DROP PROCEDURE IF EXISTS `%s`$$\r\n\r\n", m_seltable.GetString());
	
	return wyTrue;
}

wyBool    
CQueryObject::GetCreateView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strview)
{
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyString    query, msg;
	HTREEITEM	hitem;
    wyChar      buffer[SIZE_256];
    wyBool      isfromis = wyFalse;
    MDIWindow*  wnd = GetActiveWin();

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	query.Sprintf("show create table `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());
		
	myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());

        if(IsMySQL51(tunnel, mysql) == wyTrue)
        {
            isfromis = wyTrue;
            tunnel->mysql_real_escape_string(*mysql, buffer, m_seldatabase.GetString(), m_seldatabase.GetLength());
            query.Sprintf("select `IS_UPDATABLE`, `DEFINER`, `SECURITY_TYPE`, `VIEW_DEFINITION` from information_schema.`VIEWS` where `TABLE_SCHEMA` = '%s'", buffer);
            tunnel->mysql_real_escape_string(*mysql, buffer, m_seltable.GetString(), m_seltable.GetLength());
            query.AddSprintf(" AND `TABLE_NAME` = '%s'", buffer);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
        }

        if(!myres)
        {
            if(isfromis == wyTrue)
            {
                ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
		    return wyFalse;
        }
	}

	if(tunnel->mysql_num_rows(myres)== 0)
	{
		msg.Sprintf("View '%s' doesn't exists!", m_seltable.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		tunnel->mysql_free_result(myres);
		return wyFalse;
	}

	myrow = tunnel->mysql_fetch_row(myres);

    if(isfromis == wyFalse)
    {
        strview.SetAs(myrow[1], wnd->m_ismysql41);
    }
    else
    {
        strview.Sprintf("create algorithm=%s definer=%s sql security %s view `%s` as %s", myrow[0] ? (!strcmpi(myrow[0], "yes") ? "MERGE" : "TEMPTABLE") : "UNDEFINED", 
            myrow[1], myrow[2], m_seltable.GetString(), myrow[3]); 
    }

	tunnel->mysql_free_result(myres);
	return wyTrue;
}

wyBool    
CQueryObject::GetAlterView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strview)
{
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyString    query, msg;
	HTREEITEM	hitem;
    wyChar      buffer[SIZE_256];
    wyBool      isfromis = wyFalse;
    MDIWindow*  wnd = GetActiveWin();

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	query.Sprintf("show create table `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());
		
	myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());

        if(IsMySQL51(tunnel, mysql) == wyTrue)
        {
            isfromis = wyTrue;
            tunnel->mysql_real_escape_string(*mysql, buffer, m_seldatabase.GetString(), m_seldatabase.GetLength());
            query.Sprintf("select `IS_UPDATABLE`, `DEFINER`, `SECURITY_TYPE`, `VIEW_DEFINITION` from information_schema.`VIEWS` where `TABLE_SCHEMA` = '%s'", buffer);
            tunnel->mysql_real_escape_string(*mysql, buffer, m_seltable.GetString(), m_seltable.GetLength());
            query.AddSprintf(" AND `TABLE_NAME` = '%s'", buffer);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
        }

        if(!myres)
        {
            if(isfromis == wyTrue)
            {
                ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
		    return wyFalse;
        }
	}

	if(tunnel->mysql_num_rows(myres)== 0)
	{
		msg.Sprintf("View '%s' doesn't exists!", m_seltable.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		tunnel->mysql_free_result(myres);
		return wyFalse;
	}

	myrow = tunnel->mysql_fetch_row(myres);

    if(isfromis == wyFalse)
    {
        strview.SetAs(myrow[1], wnd->m_ismysql41);
		//Replacing CREATE with ALTER ,Hence 0 is the start position and 6 is the length of substring to be replaced
		if(strview.GetLength()!=0)
			strview.Replace(0, 6, "ALTER");
    }
    else
    {
        strview.Sprintf("alter algorithm=%s definer=%s sql security %s view `%s` as %s", myrow[0] ? (!strcmpi(myrow[0], "yes") ? "MERGE" : "TEMPTABLE") : "UNDEFINED", 
            myrow[1], myrow[2], m_seltable.GetString(), myrow[3]); 
    }

	tunnel->mysql_free_result(myres);
	return wyTrue;
}


wyBool  
CQueryObject::GetDropView(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strview)
{
	HTREEITEM	hitem;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	strview.Sprintf("USE `%s`$$\r\n\r\n", m_seldatabase.GetString());
	strview.AddSprintf("DROP VIEW IF EXISTS `%s`$$\r\n\r\n", m_seltable.GetString());

	return wyTrue;
}

wyBool 
CQueryObject::GetCreateTrigger(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strtrigger)
{
	wyString        trigger, definer;
	HTREEITEM       hitem, htemp;
	wyString        strmsg, query, table;
	wyWChar			trigname[SIZE_512] = {0};
	wyBool			retbool, ismysql41, ismysql5017;
	MDIWindow		*wnd = NULL;
	
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;

	ismysql41 = IsMySQL41(wnd->m_tunnel, &wnd->m_mysql);
	ismysql5017 = IsMySQL5017(wnd->m_tunnel, &wnd->m_mysql);

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));
	// Get the database name.
	//GetTableDatabaseName(hitem);
	VERIFY(htemp = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(htemp = TreeView_GetParent(m_hwnd, htemp));

	//Database
	GetDatabaseName(htemp);
    	
	//trigger name
	retbool = GetNodeText(m_hwnd, hitem, trigname, SIZE_512);
	if(retbool == wyFalse)
		return wyFalse;

	trigger.SetAs(trigname);

	//Gets the table name
	/*query.Sprintf("select `EVENT_OBJECT_TABLE`, `DEFINER` from `INFORMATION_SCHEMA`.`TRIGGERS` where `EVENT_OBJECT_SCHEMA` = '%s' and `TRIGGER_NAME` = '%s'", m_seldatabase.GetString(), trigger.GetString());
	
	myres = ExecuteAndGetResult(GetActiveWin(), wnd->m_tunnel, &wnd->m_mysql, query);	
	if(!myres && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	myrow = wnd->m_tunnel->mysql_fetch_row(myres);
	
	//If table drops and do alter trigger it gives no row(s)
	if(!myrow || !myrow[0])
	{
		if(myres)
			wnd->m_tunnel->mysql_free_result(myres);

		MessageBox(m_hwnd, L"Unable to retrive the table for this trigger", 
			pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}

	table.SetAs(myrow[0], ismysql41);
    	
	if(myres)
		wnd->m_tunnel->mysql_free_result(myres);

	//Gets the definer
	if(ismysql5017 == wyTrue) 
	{
		query.Sprintf("select `DEFINER` from `INFORMATION_SCHEMA`.`TRIGGERS` where `EVENT_OBJECT_SCHEMA` = '%s' and `TRIGGER_NAME` = '%s'", m_seldatabase.GetString(), trigger.GetString());
		myres = ExecuteAndGetResult(GetActiveWin(), wnd->m_tunnel, &wnd->m_mysql, query);	
		if(!myres && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
			return wyFalse;
		}

		myrow = wnd->m_tunnel->mysql_fetch_row(myres);
		definer.SetAs(myrow[0], ismysql41);	    	
		if(myres)
			wnd->m_tunnel->mysql_free_result(myres);
	}
	*/

	//Gets the Trigger statement
	if(!GetCreateTriggerString(m_hwnd, tunnel, mysql, m_seldatabase.GetString(), trigger.GetString(), strtrigger, strmsg))
	{
		//if it is mysql err
		if(tunnel->mysql_errno(*mysql) != 0)
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		else if(tunnel->mysql_errno(*mysql) == 0 && strmsg.GetLength() != 0)// if it is sqlyog error message
			ShowErrorMessage(m_hwnd, query.GetString(), strmsg.GetString());

		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	return wyTrue;
}

wyBool 
CQueryObject::GetDropTrigger(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyString &strtrigger)
{
	wyString    trigger;
	HTREEITEM	hitem;

	VERIFY(hitem	=	TreeView_GetSelection(m_hwnd));

	// Get the database name.
	GetTableDatabaseName(hitem);
	trigger.SetAs(m_seltable.GetString());

	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	//VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetTableDatabaseName(hitem);

	strtrigger.Sprintf("USE `%s`$$\r\n\r\n", m_seldatabase.GetString());

	strtrigger.AddSprintf("DROP TRIGGER /*!50032 IF EXISTS */ `%s`$$\r\n\r\n", trigger.GetString());
		
	return wyTrue;
}
wyBool
CQueryObject::DropDatabaseObject(Tunnel * tunnel, PMYSQL mysql, wyChar *objecttype)
{
	MYSQL_RES   *myres;
	wyString    query;
	wyInt32     ret, isintransaction = 1;
	HTREEITEM	hitem, hitemtemp;
	wyString    objectname;

#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif

	objectname.SetAs(objecttype);
	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	hitemtemp = hitem;
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	query.Sprintf(_("Do you really want to drop the %s (%s)?"), objectname.GetString(), m_seltable.GetString());

	ret = yog_message(m_hwnd, query.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL);
	
	if(ret != IDYES)
		return wyFalse;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	query.Sprintf("drop %s `%s`.`%s`",objectname.GetString(), m_seldatabase.GetString(), m_seltable.GetString());
	
    myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!myres && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
	}
	tunnel->mysql_free_result(myres);

	// Here recount the Function count "Functions(counter)" if the corresponding option is set in the Preference	   
	TreeView_DeleteItem(m_hwnd, hitemtemp);

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;

}

wyBool   
CQueryObject::DropTrigger(Tunnel * tunnel, PMYSQL mysql)
{
	MYSQL_RES	*myres;
    wyString    query;
	wyInt32     ret, isintransaction = 1;
	HTREEITEM	hitem, hitemtemp;
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));
	hitemtemp = hitem;
	// Get the database name.
	GetTableDatabaseName(hitem);
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	GetDatabaseName(hitem);

	query.Sprintf(_("Do you really want to drop the trigger (%s)?"), m_seltable.GetString());

	ret = yog_message(m_hwnd, query.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL);
	
	if(ret != IDYES)
		return wyFalse;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	query.Sprintf("drop trigger /*!50032 if exists */ `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());
	
    myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!myres && tunnel->mysql_affected_rows(*mysql)== -1)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
	}
	tunnel->mysql_free_result(myres);

	TreeView_DeleteItem(m_hwnd, hitemtemp);
	
	//Post 8.01
	//RepaintTabModule();

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

wyBool
CQueryObject::EndRenameTable(LPNMTVDISPINFO ptvdi)
{
    wyString    query;
    MYSQL_RES   *myres;
	wyString	itemtextstr;
    TabEditor   *ptabeditor = NULL;
	wyInt32		isintransaction = 1;

	MDIWindow*	wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	_ASSERT(wnd);
    ptabeditor = wnd->GetActiveTabEditor();	

	itemtextstr.SetAs(ptvdi->item.pszText);
	query.Sprintf("rename table `%s`.`%s` to `%s`.`%s`", 
					 m_seldatabase.GetString(), m_seltable.GetString(), 
					 m_seldatabase.GetString(), itemtextstr.GetString());
	
    myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!myres && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(myres);

	//When selected tab is table data then setting new name for the table.
	/*if(ptabeditor)
	{
		TabMgmt *ptabmgmt = ptabeditor->m_pctabmgmt;
		if(ptabmgmt->GetSelectedItem() == IDI_TABLE)
			ptabmgmt->m_insert->m_table.SetAs(itemtextstr.GetString());
	}*/
	
	return wyTrue;
}


wyBool
CQueryObject::EndRenameView(LPNMTVDISPINFO ptvdi)
{
	wyString    strview, query, msg, myrowstr, myrowsecstr;
	MYSQL_RES   *res;
	MYSQL_ROW   myrow;
		
	MDIWindow*	wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	_ASSERT(wnd);
	wyBool		ismysql41 = IsMySQL41(wnd->m_tunnel, &wnd->m_mysql);

    query.Sprintf("show tables from `%s` like '%s'", m_seldatabase.GetString(), m_seltable.GetString() );

    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	if(wnd->m_tunnel->mysql_num_rows(res)== 0)
	{
		msg.Sprintf("View '%s' doesn't exists!", m_seltable.GetString());

		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);

		wnd->m_tunnel->mysql_free_result(res);

		return wyFalse;
	}
	
	wnd->m_tunnel->mysql_free_result(res);
	myrowstr.SetAs(ptvdi->item.pszText);
	query.Sprintf("show tables from `%s` like '%s'", m_seldatabase.GetString(), myrowstr.GetString());

    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	if(wnd->m_tunnel->mysql_num_rows(res)> 0)
	{
		msg.Sprintf("View '%s' already exists!",  myrowstr.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	
	wnd->m_tunnel->mysql_free_result(res);

	/*	Create the new view with the informations from INFORMATION_SCHEMA and 
		then DROP the existing old VIEW */
	query.Sprintf("select VIEW_DEFINITION from INFORMATION_SCHEMA.VIEWS where TABLE_SCHEMA = '%s' and TABLE_NAME = '%s'", m_seldatabase.GetString(),  m_seltable.GetString());
	
    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query); 

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	myrow = wnd->m_tunnel->mysql_fetch_row(res);
	
	if(!myrow)
	{ 
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	myrowsecstr.SetAs(myrow[0], ismysql41);

    if(myrowsecstr.GetLength() == 0)
    {
        yog_message(m_hwnd, _(L"No View defination found.\n\n\
Probably this view was created like \n\"CREATE VIEW...SQL SECURITY DEFINER...\"\n\
and you are not the DEFINER"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR);
        return wyTrue;
    }

	strview.Sprintf("create view `%s`.`%s` as %s", m_seldatabase.GetString(), myrowstr.GetString(), myrowsecstr.GetString());

	wyInt32 isintransaction = 1;

    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, strview, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, strview.GetString());
		return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(res);

	query.Sprintf("drop view `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString());

    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}
wyBool
CQueryObject::EndRenameEvent(LPNMTVDISPINFO ptvdi)
{
	wyString    renameevent, query, msg;
	MYSQL_RES   *res;	
	wyBool		iscollate = wyFalse;	
	MDIWindow*	wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	wyInt32 isintransaction = 1;

	_ASSERT(wnd);
	
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	renameevent.SetAs(ptvdi->item.pszText);

	if(iscollate)
		query.Sprintf("show events where BINARY db= '%s' and name = '%s'", m_seldatabase.GetString(), 
															 renameevent.GetString());
	else
		query.Sprintf("show events where db= '%s' and name = '%s'", m_seldatabase.GetString(), 
															 renameevent.GetString());
   res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
   

   if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

  if(wnd->m_tunnel->mysql_num_rows(res) > 0)
   {
	   msg.Sprintf("Event '%s' allready exists!", renameevent.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
   }

   wnd->m_tunnel->mysql_free_result(res);

   query.Sprintf("alter event `%s`.`%s` rename to `%s`.`%s`", m_seldatabase.GetString(), m_seltable.GetString(), 
															m_seldatabase.GetString(), renameevent.GetString());
   res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;


   if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(res);
	return wyTrue;

}

wyBool
CQueryObject::EndRenameTrigger(LPNMTVDISPINFO ptvdi)
{
    wyString        query, dropquery, oldquery, msg, temptextstr;
	wyString		mytemprow1, mytemprow2, mytemprow3, mytemprow4, myrowstr;
    wyBool			restore = wyFalse;
	MYSQL_RES		*res;
	MYSQL_ROW		myrow;//,myrow1
	wyString		myrow0str, myrow1str, myrow2str, myrow3str, myrow4str, myrow5str;

	MDIWindow*		wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	wyBool			ismysql41 = IsMySQL41(wnd->m_tunnel, &wnd->m_mysql);

	_ASSERT(wnd);

	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s' and TRIGGER_NAME = '%s'",
      //               m_seldatabase.GetString(), m_seltrigger.GetString());
	
	/*query.Sprintf("SHOW TRIGGERS FROM `%s` WHERE `Trigger`='%s'", m_seldatabase.GetString(), m_seltrigger.GetString());
 
    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	if(wnd->m_tunnel->mysql_num_rows(res)== 0)
	{
		msg.Sprintf("Trigger '%s' doesn't exists!", m_seltrigger.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	
	wnd->m_tunnel->mysql_free_result(res);
	*/

	if(!ptvdi || !ptvdi->item.pszText)
	{
		return wyFalse;
	}

	temptextstr.SetAs(ptvdi->item.pszText);

	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s' and TRIGGER_NAME = '%s'",
		//m_seldatabase.GetString(), temptextstr.GetString());
	query.Sprintf("SHOW TRIGGERS FROM `%s` WHERE `Trigger`='%s'", m_seldatabase.GetString(), temptextstr.GetString());
	/*query1.Sprintf("show create trigger `%s`. `%s`", m_seldatabase.GetString(), temptextstr.GetString());*/
    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	////bug http://bugs.mysql.com/bug.php?id=75685. We have to fire show create trigger to get the body of trigger
	//res1=ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query1);
	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql);
		return wyFalse;
	}
	/*if(!res1 && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql);
		return wyFalse;
	}
*/
	if(wnd->m_tunnel->mysql_num_rows(res)> 0)
	{
		msg.Sprintf("Trigger '%s' already exists!", temptextstr.GetString());
		yog_message(m_hwnd, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	
	wnd->m_tunnel->mysql_free_result(res);
	    
	query.Sprintf("SHOW TRIGGERS FROM `%s`",m_seldatabase.GetString());

	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}
	
	if(wnd->m_tunnel->mysql_num_rows(res) == 0)
	{
		yog_message(m_hwnd, _(L"No information available!"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	
	do
	{
		myrow = wnd->m_tunnel->mysql_fetch_row(res);

		if(!myrow)
		{
			wnd->m_tunnel->mysql_free_result(res);
			return wyFalse;
		}
		myrow0str.SetAs(myrow[0], ismysql41);

		if((myrow0str.CompareI(m_seltrigger.GetString())) == 0)
			break;

	} while(1);
	//myrow1=wnd->m_tunnel->mysql_fetch_row(res1);
	//	//body of trigger
	//bodyoftrigger.SetAs(myrow1[2]);
	//GetBodyOfTrigger(&bodyoftrigger);

	/*	Create new view */
	myrow0str.SetAs(myrow[0], ismysql41);
	myrow1str.SetAs(myrow[1], ismysql41);
	myrow2str.SetAs(myrow[2], ismysql41);
	myrow3str.SetAs(myrow[3], ismysql41);
	myrow4str.SetAs(myrow[4], ismysql41);

	query.Sprintf("create trigger `%s` %s %s on `%s`.`%s` for each row %s",
		temptextstr.GetString(), /* Trigger */
		myrow4str.GetString(), /* Timing */
		myrow1str.GetString(), /* Event */
				  m_seldatabase.GetString(), /* Database*/
				  myrow2str.GetString(),      /* Table */
				  myrow3str.GetString() /* Statement */);

	oldquery.Sprintf("create trigger `%s` %s %s on `%s`.`%s` for each row %s",
		myrow0str.GetString(), /* Trigger */
		myrow4str.GetString(), /* Timing */
		myrow1str.GetString(), /* Event */
				  m_seldatabase.GetString(), /* Database*/
				  myrow2str.GetString(),
				  myrow3str.GetString() /* Statement */);

	/* Drop trigger otherwise you may not create another with same constraint */
	dropquery.Sprintf("drop trigger /*!50032 if exists */ `%s`.`%s`", m_seldatabase.GetString(), m_seltrigger.GetString());

	wyInt32 isintransaction = 1;
    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, dropquery, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

	if(isintransaction == 1)
		return wyFalse;

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, dropquery.GetString());
		return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(res);

    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());

        //on MySQL error create back the dropped trigger
        res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, oldquery);
        
        if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
			ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, oldquery.GetString());
		
        return wyFalse;
	}

	wnd->m_tunnel->mysql_free_result(res);
	
	if(!restore)
		return wyTrue;

	return wyFalse;
}


HTREEITEM 
CQueryObject::GetDatabaseNode()
{
	wyInt32     image;
	HTREEITEM	hselitem;

	hselitem = TreeView_GetSelection(m_hwnd);
    
	if(!hselitem)
		return NULL;
    
	image = GetItemImage(m_hwnd, hselitem);

	switch(image)
	{
	case NTABLE:
	case NVIEWSITEM:
	case NEVENTITEM:
	case NSPITEM:
	case NFUNCITEM:
	case NTRIGGERITEM:
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			break;

	case NCOLUMN:
	case NPRIMARYKEY:
	case NINDEX:	
	case NPRIMARYINDEX:
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			break;

	case NFOLDER:
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem  = TreeView_GetParent(m_hwnd, hselitem);
			hselitem  = TreeView_GetParent(m_hwnd, hselitem);
			break;
	case NEVENTS:
	case NVIEWS:
	case NSP:
	case NFUNC:
	case NTABLES:
    case NTRIGGER:
			hselitem  = TreeView_GetParent(m_hwnd, hselitem);
			break;
	case NDATABASE:
			break;
	}
	
	return hselitem; 
}

HTREEITEM 
CQueryObject::GetTableNode()
{
	wyInt32			image;
	HTREEITEM	hselitem;

	hselitem = TreeView_GetSelection(m_hwnd);

    if(!hselitem)
		return NULL;

	image = GetItemImage(m_hwnd, hselitem);

	switch(image)
	{
	case NCOLUMN:
	case NPRIMARYKEY:
	case NINDEX:
	case NPRIMARYINDEX:
	case NTRIGGERITEM:
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			hselitem = TreeView_GetParent(m_hwnd, hselitem);
			break;

	case NFOLDER:
	case NTRIGGER:
			hselitem  = TreeView_GetParent(m_hwnd, hselitem);
			break;

	case NTABLE:
			hselitem = NULL;

		break;
	}
	
	return hselitem; 
}

HTREEITEM 
CQueryObject::GetNode(HTREEITEM hdbitem, wyInt32 type)
{
	HTREEITEM	hselitem;
	TVITEM		tvi;
	wyWChar      database[SIZE_512] = {0};
	wyInt32		ret;

	hselitem = TreeView_GetChild(m_hwnd, hdbitem);

	while(1)
	{
		tvi.mask		= TVIF_IMAGE | TVIF_TEXT;
		tvi.hItem		= hselitem;
		tvi.cchTextMax	= SIZE_512-1;
		tvi.pszText		= database;

		ret = TreeView_GetItem(m_hwnd, &tvi);

		if(tvi.iImage == type)
			return hselitem;

		if(!ret)
			return NULL;

		hselitem = TreeView_GetNextItem(m_hwnd, hselitem, TVGN_NEXT);
	}
}

wyBool  
CQueryObject::InsertNodeText()
{
	wyWChar     buff[SIZE_512];
	MDIWindow*	wnd;
	wyString	buffstr;
    TabEditor   *ptabeditor = NULL;
    EditorBase  *peditorbase = NULL;
    wyString    temp;
	wyInt32		tabicon = 0;

	VERIFY(wnd = GetActiveWin());
    
    tabicon = wnd->m_pctabmodule->GetActiveTabImage();

    if(tabicon == IDI_SCHEMADESIGNER_16 || tabicon == IDI_QUERYBUILDER_16 || tabicon == IDI_DATASEARCH)
    {
        return wyFalse;
    }

	if(m_isexpcollapse == wyTrue)
		return wyFalse;
	
	if(!GetNodeText(m_hwnd, buff, SIZE_512))
		return wyFalse;
	
    ptabeditor = wnd->GetActiveTabEditor();

	if(ptabeditor)
	{
        peditorbase = ptabeditor->m_peditorbase;
        temp.SetAs(buff);
        buffstr.Sprintf(AppendBackQuotes() == wyTrue ? "`%s`" : "%s", temp.GetString());
        SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buffstr.GetString());
		peditorbase->m_edit = wyTrue;

		// Changes the focus to editor 
	    PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd ,0);
        return wyTrue;
    }

	return wyFalse;
}

wyBool 
CQueryObject::SelectFirstDB()
{
	if(m_filterdb.GetLength() == 0)
		TreeView_SelectItem(m_hwnd, TreeView_GetChild(m_hwnd, TreeView_GetRoot(m_hwnd)));

	return wyTrue;
}

wyBool 
CQueryObject::HandlerToGetTableDatabaseName(wyInt32 item, HTREEITEM hitem)
{
    switch(item)
	{
	case NINDEX:
	case NPRIMARYINDEX:
	case NCOLUMN:
	case NPRIMARYKEY:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NTABLE:
		break;

    case NTABLES:
        VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
        break;

    case NVIEWSITEM:
        VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
        break;

	default:
		return wyFalse;
    }
    // Get the database name.
    GetTableDatabaseName(hitem);
    return wyTrue;
}

void 
CQueryObject::HandlerToGetDatabaseName(wyInt32 item, HTREEITEM hitem)
{
    switch(item)
	{
	case NTABLES:
	case NVIEWS:
	case NTRIGGER:
	case NFUNC:
	case NEVENTS:
	case NSP:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem ));
		break;

	case NCOLUMN:
	case NPRIMARYKEY:
	case NINDEX:
	case NPRIMARYINDEX:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
        VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NFOLDER:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
        VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	case NVIEWSITEM:
	case NSPITEM:
	case NFUNCITEM:
	case NEVENTITEM:
	case NTRIGGERITEM:
	case NTABLE:
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
		break;

	default:
		break;
	}

	GetDatabaseName(hitem);
    return;
}

//draging
wyBool
CQueryObject::OnBeginDrag(LPARAM lParam)   
{
	wyInt32		image, id;
	MDIWindow	*wnd = GetActiveWin();
	CTCITEM		item;	
	HTREEITEM	hitem;
	POINT		pnt;

    item.m_mask  =  CTBIF_IMAGE | CTBIF_LPARAM;

	id = CustomTab_GetCurSel(wnd->m_pctabmodule->m_hwnd);
	CustomTab_GetItem(wnd->m_pctabmodule->m_hwnd, id, &item);

	if(item.m_iimage == IDI_QUERYBUILDER_16 || item.m_iimage == IDI_SCHEMADESIGNER_16)
	{
		pnt.x	= (LONG)LOWORD(lParam);
		pnt.y	= (LONG)HIWORD(lParam);
	
		hitem	= SelectOnContext(&pnt);
    
		image	= GetSelectionImage();
		
		if(image == NTABLE)
		{
			wnd->m_dragimaglist = SendMessage(m_hwnd, TVM_CREATEDRAGIMAGE, 0, 0);
			ImageList_BeginDrag((HIMAGELIST)(wnd->m_dragimaglist), 0, 0, 0);
			ImageList_DragEnter(m_hwnd, 0, 0);	
			return wyTrue;
		}

		//QB supports Tables as well as Views
		else if(item.m_iimage == IDI_QUERYBUILDER_16 && image == NVIEWSITEM)
		{
			wnd->m_dragimaglist = SendMessage(m_hwnd, TVM_CREATEDRAGIMAGE, 0, 0);
			ImageList_BeginDrag((HIMAGELIST)(wnd->m_dragimaglist), 0, 0, 0);
			ImageList_DragEnter(m_hwnd, 0, 0);	
			return wyTrue;
		}

		else
		{
			wnd->m_dragged = wyFalse;
			m_dragged = wyFalse;

			return wyFalse;
		}
	}
    return wyFalse;
}

// Drop table on Canvas ; for Query Builder and SchemaDesigner
wyBool
CQueryObject::ProcessTable()   
{
#ifndef COMMUNITY
    wyInt32				image, imageid;
    MDIWindow			*wnd;
    RECT				rccanvas;
	wyString			tablename;
	TabQueryBuilder		*ptabqb;
	TabSchemaDesigner	*ptabsd;
	wyBool				isview = wyFalse;
	
	if(!m_seldatabase.GetLength())
		return wyFalse;

    VERIFY(wnd = GetActiveWin());
	imageid = wnd->m_pctabmodule->GetActiveTabImage();
    image   = GetSelectionImage();
    
	if(image == NTABLE || image == NVIEWSITEM)
	{                
		//For QB supports Table as well as Views
		if(imageid == IDI_QUERYBUILDER_16)
        {
			ptabqb  = (TabQueryBuilder*)wnd->m_pctabmodule->GetActiveTabType();
			ptabqb->m_isdbclicktable = wyTrue;

			SetCursor(LoadCursor(NULL, IDC_WAIT));                       
	        
			GetWindowRect(ptabqb->m_hwnd, &rccanvas);
			MapWindowPoints(NULL, ptabqb->m_hwnd, (LPPOINT)&rccanvas, 2);
			
			//Paint the canvas after dropping the table
			InvalidateRect(ptabqb->m_hwnd, &rccanvas, FALSE);   

			//If selected image is view
			if(image == NVIEWSITEM)
				isview = wyTrue;

			ptabqb->DropTable(ptabqb->m_hwndtabview, 0, isview);
            return wyTrue;
        }

		//For SD only supports table
		else if(imageid == IDI_SCHEMADESIGNER_16 && image != NVIEWSITEM)
		{
			pGlobals->m_pcmainwin->GetSelectedTable(wnd, tablename);

			if(!tablename.GetLength())
				return wyFalse;
			
			ptabsd  = (TabSchemaDesigner*)wnd->m_pctabmodule->GetActiveTabType();

			SetCursor(LoadCursor(NULL, IDC_WAIT));                       
			GetWindowRect(ptabsd->m_hwnd, &rccanvas);
			MapWindowPoints(NULL, ptabsd->m_hwnd, (LPPOINT)&rccanvas, 2);
			
			//Paint the canvas after dropping the table
			InvalidateRect(ptabsd->m_hwnd, &rccanvas, FALSE);   
			ptabsd->CanvasDropTable(ptabsd->m_hwndcanvas, m_seldatabase.GetString(), tablename.GetString());
            return wyTrue;
        }
    }
#endif
    return wyFalse;
}


void
CQueryObject::OnDropTable()
{
	MDIWindow *wnd = GetActiveWin();
	
	if(wnd->m_dragged)
	{
		ImageList_DragLeave(m_hwnd);   
		ImageList_EndDrag();
		wnd->m_dragged = wyFalse;

		if(wnd->m_dragimaglist)
        {
            ImageList_Destroy((HIMAGELIST)wnd->m_dragimaglist);
		    wnd->m_dragimaglist = NULL;
        }
	}

	return;
}

wyChar*
CQueryObject::IsBackTick(wyBool isbacktick)
{    
    if(isbacktick == wyTrue)
       return "`";

    else
        return "";
}

// Function to select database in object browser when database is changed in combobox
wyBool     
CQueryObject:: OnComboChanged(wyWChar *dbname)
{
	HTREEITEM   hitem;
	wyWChar		database[SIZE_512] = {0};
	TVITEM		tvi={0};
	
	VERIFY(hitem = TreeView_GetRoot(m_hwnd));
	for(hitem = TreeView_GetChild(m_hwnd, hitem); hitem ; 
			hitem = TreeView_GetNextSibling(m_hwnd, hitem))
	{
		/* get the dbname from the objectbrowser */
		tvi.mask		=	TVIF_TEXT;
		tvi.hItem		=	hitem;
		tvi.cchTextMax	=	((sizeof(database)-1)*2);
		tvi.pszText		=	database;

		VERIFY(TreeView_GetItem(m_hwnd, &tvi));

		if(wcscmp(dbname, database) == 0)
		{	
			TreeView_SelectItem(m_hwnd, tvi.hItem);
			return wyTrue;
		}
	}

	return wyFalse;
}

void     
CQueryObject::OnItemExpanded(LPTVITEM  tvi)
{
	//when we r expanding a database,if we select open tables folder as default,then we need to expand Tables folder.
	//issue reported here http://code.google.com/p/sqlyog/issues/detail?id=680
	if(tvi->iImage == NDATABASE && IsOpenTablesInOB() == wyTrue )
	{
		TreeView_Expand(m_hwnd, TreeView_GetChild(m_hwnd, tvi->hItem), TVM_EXPAND );
	}

	TreeView_SelectItem(m_hwnd, tvi->hItem );
}

/*Trigger for a table should get drop when table is dropped.
Also this gets called when 'Tables' folder refreshed.
Triggers will check only if the trigger folder EXPANDED ONCE
*/
wyBool
CQueryObject::ReorderTriggers(HTREEITEM hitemdb)
{
	HTREEITEM		htreetrigitem = NULL, hitemtemp = NULL;
	wyWChar			trigname[SIZE_512];
	wyInt32			state = 0;
	wyString		strtrig, query, myrowstr;
	wyBool			isfound = wyFalse, ismysql41;
	MYSQL_RES		*myres=NULL;
	MYSQL_ROW		myrow;
	MDIWindow		*wnd = NULL;
	RelTableFldInfo *trigelem = NULL;
	List			triglist;	

	VERIFY(wnd = GetActiveWin());

	htreetrigitem = TreeView_GetChild(m_hwnd, hitemdb);

	//Find whether 'Trigger' folder present(whether mySQL version supports trigger)
	for(htreetrigitem; htreetrigitem; htreetrigitem = TreeView_GetNextSibling(m_hwnd, htreetrigitem))
	{
		if(GetNodeText(m_hwnd, htreetrigitem, trigname, SIZE_512) == wyFalse)
			return wyFalse;

		strtrig.SetAs(trigname);
        
		if(!strtrig.Compare("Triggers"))
		{
			isfound = wyTrue;
			break;
		}
	}

	//If no trigger return
	if(isfound == wyFalse)
		return wyFalse;

	//If trigger folder is not expanded-once return
	state = TreeView_GetItemState(m_hwnd, htreetrigitem, TVIS_EXPANDEDONCE);
	if(!(state & TVIS_EXPANDEDONCE))
		return wyTrue;

	//Gets all triggers. If in Triggers folder contains any other than the reurn result that will drop
	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s'",
		//m_seldatabase.GetString());

	query.Sprintf("SHOW TRIGGERS FROM `%s`", m_seldatabase.GetString());
	
	ismysql41 = IsMySQL41(wnd->m_tunnel, &wnd->m_mysql);
	myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!myres && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql) == -1)
	{
		ShowMySQLError(m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	//Keeps all triggers in linked list
	while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);

		trigelem = new RelTableFldInfo(myrowstr.GetString());

		triglist.Insert(trigelem);
	}

    if(myres)
	    wnd->m_tunnel->mysql_free_result(myres);
	
	htreetrigitem = TreeView_GetChild(m_hwnd, htreetrigitem);

	//Checking the triggers returnd with the triggers in treeview
	while(htreetrigitem)
	{
		if(GetNodeText(m_hwnd, htreetrigitem, trigname, SIZE_512) == wyFalse)
			return wyFalse;

		strtrig.SetAs(trigname);

		trigelem = (RelTableFldInfo*)triglist.GetFirst();
		isfound = wyFalse;

		while(trigelem)
		{
			if(!strtrig.CompareI(trigelem->m_tablefld))
			{
				isfound = wyTrue;
				break;
			}
			
			trigelem = (RelTableFldInfo*)trigelem->m_next;
		}

		hitemtemp = TreeView_GetNextSibling(m_hwnd, htreetrigitem);
		
		//If paricular trigger in tree not existing in linked list delete the node
		if(isfound == wyFalse)
		{			
            VERIFY(TreeView_DeleteItem(m_hwnd, htreetrigitem));			
		}

		htreetrigitem = hitemtemp;
	}
	
	return wyTrue;
}

//for handling Server node context menu
void
CQueryObject::OnWmCommand(WPARAM wparam)
{
	MDIWindow  *wnd;
	wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0;
	wyString	dirstr;
	ConnColorDlg	conncolor;
	ConnectionBase *conbase = NULL;
    
	conbase = pGlobals->m_pcmainwin->m_connection;

	VERIFY(wnd = GetActiveWin());

	switch(LOWORD(wparam))
	{
    	//Server node context menu - Refresh object browser
		case IDM_REFRESHOBJECT:
			pGlobals->m_pcmainwin->HandleOnRefresh(wnd);
			break;

		case IDM_COLLAPSEOBJECT:
			pGlobals->m_pcmainwin->HandleOnCollapse(wnd);
			break;

		case IDM_CREATEDATABASE:
			pGlobals->m_pcmainwin->OnCreateDatabase(wnd->m_hwnd, wnd);
			break;

		case ID_IMEX_TEXTFILE2:
			pGlobals->m_pcmainwin->OnImportFromSQL(wnd);
            break;

		case ID_IMPORT_EXTERNAL_DATA:
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		break;
	}
#endif
            conbase->OnSchdOdbcImport();
			break;
		
		case ID_POWERTOOLS_SCHEDULEEXPORT2:
			 conbase->OnScheduleExport();
			break;

		case ID_DATASEARCH:
			if(wnd && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
				&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
				wnd->m_pctabmodule->CreateDatabaseSearchTab(wnd);	
			break;

		case IDM_OBCOLOR:
			{
				//object browser color dialog
				conbase->m_rgbobbkcolor = TreeView_GetBkColor(m_hwnd);
				conbase->m_rgbobfgcolor = TreeView_GetTextColor(m_hwnd);
				conncolor.ShowConnColorDlg(wnd->m_hwnd);

				//if ob color is changed, save in .ini
				if(conncolor.m_changecolor == wyTrue)
				{
					TreeView_SetBkColor(m_hwnd, conbase->m_rgbobbkcolor);
                    /*SendMessage(m_hwndFilter, EM_SETBKGNDCOLOR, 0, conbase->m_rgbobbkcolor);*/

                    /*memset(&chf, 0, sizeof(CHARFORMAT2));
                    chf.cbSize = sizeof(CHARFORMAT2);
                    chf.dwMask = CFM_COLOR;
                    chf.crTextColor = conbase->m_rgbobfgcolor;
                    SendMessage(m_hwndFilter, EM_SETCHARFORMAT, NULL, (LPARAM)&chf);*/

					//Set the forground color
					//clrforground = conbase->m_rgbobbkcolor ^ 0xFFFFFF;
					TreeView_SetTextColor(m_hwnd, conbase->m_rgbobfgcolor);

					SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
					dirstr.SetAs(directory);
					conbase->SaveAdvancedTabDetails(NULL, wnd->m_currentconn.GetString(), dirstr.GetString());

					wnd->m_conninfo.m_rgbconn = conbase->m_rgbobbkcolor;
					wnd->m_conninfo.m_rgbfgconn = conbase->m_rgbobfgcolor;

					//change the tab color, if object browser color is changed
					pGlobals->m_pcmainwin->m_conntab->ChangeTabColor(wnd->m_conninfo.m_rgbconn,wnd->m_conninfo.m_rgbfgconn);
					//pGlobals->m_pcmainwin->m_conntab->ChangeTabColor(wnd->m_conninfo.m_rgbconn);
					


					conncolor.m_changecolor = wyFalse;
				}
				else
				{
					conbase->m_rgbobbkcolor = wnd->m_conninfo.m_rgbconn;
					conbase->m_rgbobfgcolor = wnd->m_conninfo.m_rgbfgconn;
				}

                InvalidateRect(m_hwndStParent, NULL, TRUE);
                InvalidateRect(m_hwndStMsg, NULL, TRUE);
				InvalidateRect(m_hwndRegexText, NULL, TRUE);
				InvalidateRect(m_hwndRegex, NULL, TRUE);
               /* InvalidateRect(m_hwndStClear, NULL, TRUE);*/
			}
			break;
	}
}
/* Function to drop all views */
wyInt32
CQueryObject::DropTriggers(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db)
{
	MYSQL_RES   *myres;
	MYSQL_ROW   myrow;
	wyString    query, myrowstr;
    MYSQL_RES   *res;
    MDIWindow   *wnd = GetActiveWin();
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	wyInt32     count = 0, isintransaction = 1;
	wyString	dbname(db);

	GetSelectTriggerStmt(db, query);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	
	if(!myres)
	{
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return count;
	}

	while(myrow = tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		query.Sprintf("drop trigger `%s`.`%s`", m_seldatabase.GetString(), myrowstr.GetString());
        res = ExecuteAndGetResult(wnd, tunnel, mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return count;

		if(!res && tunnel->mysql_affected_rows(*mysql)== -1)
		{
			ShowMySQLError(m_hwnd, tunnel, mysql, query.GetString());
			return count;
		}
		count++;
		tunnel->mysql_free_result(res);
	}
	tunnel->mysql_free_result(myres);
	
	//Post 8.01
	//RepaintTabModule();
	return count;
}

wyBool
CQueryObject::RefreshIndexesOrColumnsFolder(HWND hwnd, HTREEITEM hitem, wyInt32 imgsel, Tunnel *tunnel, 
											PMYSQL mysql, const wyChar *database, const wyChar *table, wyBool isindex, wyBool isrefresh)
{
	wyInt32		state = 0;
	wyString	tablename, dbname;
	HTREEITEM	hitemparent = NULL, hitemtemp = NULL;
	wyBool      istoexpand = wyFalse;
    wyWChar     filterText[70], filterTextTable[70];
    TVITEM      tvi;
    
    memset(filterText, 0, sizeof(filterText));
    memset(filterTextTable, 0, sizeof(filterTextTable));
    memset(&tvi, 0, sizeof(TVITEM));
    
	state = TreeView_GetItemState(hwnd, hitem, TVIS_USERMASK);
    //Not doing anything if its not expanded
	if(imgsel == NFOLDER && !(state & TVIS_EXPANDEDONCE) && TreeView_GetChild(hwnd, hitem))
    {
        TreeView_Expand(m_hwnd, hitem, TVE_EXPAND);
		return wyTrue;
    }
	switch(imgsel)
	{
	case NFOLDER:
		break;

	case NTABLE:
		state = TreeView_GetItemState(hwnd, hitem, TVIS_USERMASK);
	
		//Not doing anything if its not expanded
		if(!(state & TVIS_EXPANDED) && TreeView_GetChild(hwnd, hitem))
			return wyTrue;

		hitem = TreeView_GetChild(hwnd, hitem);
		if(isindex == wyTrue)
			hitem = TreeView_GetNextSibling(hwnd, hitem);
		
		break;

	case NPRIMARYKEY:
    case NPRIMARYINDEX:
	case NINDEX:
	case NCOLUMN:
		hitem = TreeView_GetParent(hwnd, hitem);
		break;
	}

	tablename.SetAs(table);
	dbname.SetAs(database);

	if(isrefresh == wyFalse)
	{
		hitemparent = TreeView_GetParent(hwnd, hitem);
		hitemtemp = TreeView_GetChild(hwnd, hitemparent);	
        
        if(!isindex)
        {
            //Columns
		    DeleteChildNodes(hwnd, hitemtemp, wyFalse);
		    InsertColumns(hwnd, tunnel, mysql, hitemtemp, tablename.GetAsWideChar(), dbname.GetAsWideChar());
        }

		//Indexes
		hitemtemp = TreeView_GetNextSibling(hwnd, hitemtemp);	
		if(! TreeView_GetChild(hwnd, hitemtemp))
			istoexpand = wyTrue;
		else
			DeleteChildNodes(hwnd, hitemtemp, wyFalse);

		InsertIndex(hwnd, tunnel, mysql, hitemtemp, tablename.GetAsWideChar(), dbname.GetAsWideChar());
		
		//Expand Index folder if it was empty
		if(istoexpand == wyTrue)
			TreeView_Expand(hwnd, hitemtemp, TVE_EXPAND);	
	}

	return wyTrue;
}

//the function selects refresh the table and selects the colums/indexes
HTREEITEM
CQueryObject::RefreshTableAndSelecItem(HTREEITEM hitem, wyInt32& image)
{
    HTREEITEM   htemp = hitem;
    wyInt32     colstate = 0, indexstate = 0, count, position = 0; 

    //block the paint
    //SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);

    switch(image)
	{
        case NPRIMARYKEY:
        case NPRIMARYINDEX:
	    case NINDEX:
	    case NCOLUMN:
            hitem = TreeView_GetParent(m_hwnd, hitem);
            htemp = hitem;
            image = NFOLDER;
		    break;

        case NTABLE:
            RefreshObjectBrowserOnCreateAlterTable();
            //unblock the paint
            SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
            UpdateWindow(m_hwnd);
            return TreeView_GetSelection(m_hwnd);
    }
    hitem = TreeView_GetParent(m_hwnd, hitem);
    for(count = 0, hitem = TreeView_GetChild(m_hwnd, hitem); hitem;  ++count, hitem = TreeView_GetNextSibling(m_hwnd, hitem))
    {
        if(count == 0)
            colstate = TreeView_GetItemState(m_hwnd, hitem, TVIS_USERMASK);
        else
            indexstate = TreeView_GetItemState(m_hwnd, hitem, TVIS_USERMASK);

        if(htemp == hitem)
            position = count;
    }

    RefreshObjectBrowserOnCreateAlterTable();
    hitem = TreeView_GetSelection(m_hwnd);
    TreeView_Expand(m_hwnd, hitem, TVE_EXPAND);

    for(count = 0, hitem = TreeView_GetChild(m_hwnd, hitem); hitem; ++count, hitem = TreeView_GetNextSibling(m_hwnd, hitem))
    {
        if(count == 0 && (colstate & TVIS_EXPANDED))
		{
            TreeView_Expand(m_hwnd, hitem, TVE_EXPAND);
		}
        else if(count == 1 && (indexstate & TVIS_EXPANDED))
		{
            TreeView_Expand(m_hwnd, hitem, TVE_EXPAND);
		}

        if(position == count)
		{
            htemp = hitem;
		}
    }
    //TreeView_SelectItem(m_hwnd, htemp);
    //unblock the paint
    //SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
    //UpdateWindow(m_hwnd);
    
    return htemp;
}

/*
Gets the table engine type
looks this into table-engine menu, if found mark the engine menu item
*/
VOID
CQueryObject::SelectTableEngineMenuItem(MDIWindow *wnd, HMENU hmenuhandle, wyInt32 indexmenu)
{
	wyInt32		item = -1, menucount = 0, index = 0;
	HTREEITEM	hitem;
	wyString	engine;
	wyWChar		name[SIZE_128];
	HMENU		hsubmenu;

	hsubmenu = GetSubMenu(hmenuhandle, indexmenu);

	if(!hsubmenu)
	{
		return;
	}

	VERIFY(hitem = TreeView_GetSelection(m_hwnd));

	item = GetSelectionImage();

	if(item != NTABLE)
	{
		return;
	}
	
	//Make sure query execution is only once when the Table menu is popped up.
	if(!pGlobals->m_menutableengine.GetLength())
	{
		//Gets selected db/table
		HandlerToGetTableDatabaseName(item, hitem);

		//Gets the selected table's engine
        if(wnd->m_executing == wyFalse)
        {
		    GetsTableEngineName(wnd, 
							m_seldatabase.GetString(),
							m_seltable.GetString(), &pGlobals->m_menutableengine);
        }

		if(!pGlobals->m_menutableengine.GetLength())
		{
			return;
		}
	}

	menucount = GetMenuItemCount(hsubmenu);

	//Mark the menu item if found
	for(index = 0; index < menucount; index++)
	{
        GetMenuString(hsubmenu, index, name, SIZE_128, MF_BYPOSITION);

		if(!wcsicmp(name, pGlobals->m_menutableengine.GetAsWideChar()))
		{
			CheckMenuItem(hsubmenu, index, MF_BYPOSITION | MF_CHECKED);
		}
        else
            CheckMenuItem(hsubmenu, index, MF_BYPOSITION | MF_UNCHECKED);
	}	
}

wyBool
CQueryObject::OpenTable()
{
    MDIWindow*      wnd = GetActiveWin();
    TabTableData*   ptabtabledata;
    TabEditor*      ptabeditor;
    TabTable*       ptable;

    ptabtabledata = wnd->GetActiveTabTableData();
    ptabeditor = wnd->GetActiveTabEditor();

    if(ptabtabledata)
    {
        if(ptabtabledata->m_istabsticky == wyTrue)
        {
            return wyTrue;
        }

        if(ptabtabledata->m_tabledata && !m_seldatabase.CompareI(ptabtabledata->m_tabledata->m_db) &&
            !m_seltable.CompareI(ptabtabledata->m_tabledata->m_table))
        {
            return wyTrue;
        }

        wnd->m_pctabmodule->CreateTabDataTab(wnd, wyFalse, wyTrue);
        return wyTrue;
    }
    if(ptabeditor && ptabeditor->m_pctabmgmt && ptabeditor->m_pctabmgmt->GetSelectedItem() == IDI_TABLE)
    {
        ptable = (TabTable*)ptabeditor->m_pctabmgmt->GetActiveTabType();
        
        if(ptable->m_data && !m_seldatabase.CompareI(ptable->m_data->m_db) &&
            !m_seltable.CompareI(ptable->m_data->m_table))
        {
            return wyTrue;
        }

        ptable->Refresh();
        return wyTrue;
    }

    return wyFalse;
}

wyBool 
CQueryObject::IsSelectionOnTable()
{
    HTREEITEM   hitem;
    wyInt32     image;

    image = GetSelectionImage();

    if((hitem = TreeView_GetSelection(m_hwnd)) &&
        (hitem = TreeView_GetParent(m_hwnd, hitem)) && 
        (hitem = TreeView_GetParent(m_hwnd, hitem)) &&
        (hitem = TreeView_GetParent(m_hwnd, hitem)) &&
        (hitem = TreeView_GetParent(m_hwnd, hitem)) == NULL &&
        (image == NTABLE || image == NVIEWSITEM))
    {
        return wyTrue;
    }

    return hitem ? wyTrue : wyFalse;
}

void
CQueryObject::HandleOBFilter(wyWChar  *text, wyBool isAutoCmplt)
{
    TVITEM      tvi;
    HTREEITEM   hti = NULL;
    wyWChar     str[70], str1[70];
    wyWChar     *temp = NULL;
    //isAutoCmplt=wyFalse;
    ::memset(str, 0, sizeof(str));
    ::memset(str1, 0, sizeof(str1));
#ifndef COMMUNITY
	if(m_isRegexChecked==wyTrue)
		isAutoCmplt=wyFalse;
#endif

    if(text)
    {
        wcscpy(str, text);
    }
    else
    {
        GetWindowText(m_hwndFilter, str, 65);
        RedrawWindow(m_hwndFilter, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
    }
    m_matchString.SetAs(L"");
    wcscpy(m_strOrig, str);

    SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);
    m_OBFilterWorkInProgress = wyTrue;

    if(wcsicmp(str, m_prevString.GetAsWideChar()) != 0)
    {
        hti = GetSelectionOnFilter(m_hwnd);

        ::memset(&tvi, 0, sizeof(TVITEM));
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hti;
        TreeView_GetItem(m_hwnd, &tvi);

        if(tvi.lParam == NULL)
        {
            tvi.lParam = (LPARAM)new OBDltElementParam(0);
            tvi.mask = TVIF_PARAM;
            tvi.hItem = hti;
            TreeView_SetItem(m_hwnd, &tvi);
            m_allocatedList.Insert((OBDltElementParam *)tvi.lParam);
        }
        else
        {
            if( wcslen(((OBDltElementParam *)tvi.lParam)->m_filterText))
            {
                m_prevString.SetAs(((OBDltElementParam *)tvi.lParam)->m_filterText);
                if(wcsicmp(str, m_prevString.GetAsWideChar()) == 0)
                {
                    m_OBFilterWorkInProgress = wyFalse;
                    return;
                }
            }
        }
        
        memset(&tvi, 0, sizeof(TVITEM));
        tvi.mask = TVIF_STATE;
        tvi.hItem = hti;
        tvi.stateMask = TVIS_OVERLAYMASK;
        tvi.state = INDEXTOOVERLAYMASK(wcscmp(str,L"")?1:0);
        TreeView_SetItem(m_hwnd, &tvi);

        wcscpy(str1, m_prevString.GetAsWideChar());
#ifndef COMMUNITY
		if( m_isRegexChecked != wyTrue )
		{
			_wcslwr_s(str1, 65);
			_wcslwr_s(str, 65);
		}
#else
			_wcslwr_s(str1, 65);
			_wcslwr_s(str, 65);
#endif
        temp = wcsstr(str1, str);
        if(temp == str1)
        {
            FilterDeleteList(str);
        }
        else
        {
            temp = wcsstr(str, m_prevString.GetAsWideChar());
            if(temp == str)
            {
                FilterTreeView(str);
            }
            else
            {
                FilterBothTreeViewAndDeleteList(str);
            }
        }
        m_prevString.SetAs(m_strOrig);
        m_isSelValid = wyFalse;

        if(isAutoCmplt && !m_isBkSpcOrDelOrClick && m_matchString.GetLength() && wcslen(str) > 0) 
        {
            SetWindowText(m_hwndFilter, m_matchString.GetAsWideChar());

            if(m_startCaret)
            {
                m_isSelValid = wyFalse;
                SendMessage(m_hwndFilter, EM_SETSEL, m_startCaret, m_startCaret);
            }
            else if (m_endCaret)
            {
                m_isSelValid = wyFalse;
                SendMessage(m_hwndFilter, EM_SETSEL, m_endCaret, m_endCaret);
            }
            else
            {
                m_isSelValid = wyTrue;
                SendMessage(m_hwndFilter, EM_SETSEL, wcslen(str), -1); 
            }
        }
    }

    SendMessage(m_hwnd, WM_SETREDRAW, TRUE, NULL);
    InvalidateRect(m_hwnd, NULL, TRUE);
    m_OBFilterWorkInProgress = wyFalse;
}

void
CQueryObject::FilterTreeView(wyWChar str[])
{
    wyInt32     image = -1;
    HTREEITEM   nexthti = NULL;
    HTREEITEM   hti;
    TVITEM      tvi;
#ifndef COMMUNITY
	pcre*		pregex;
	const wyChar *str11;
	wyInt32 offset,ovector[12];
#endif
    wyWChar     objName[200], *temp = NULL, stri[200];
    OBDltElementParam   *lparamValue = NULL;
    wyElem              *tempDlt = NULL;
    
    memset(stri, 0, sizeof(stri));

    hti = GetSelectionOnFilter(m_hwnd);
    ::memset(&tvi, 0, sizeof(TVITEM));
    image = GetSelectionImage();
    

    tvi.hItem = hti;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(m_hwnd, &tvi);
    lparamValue = (OBDltElementParam *)tvi.lParam;
    wcscpy(lparamValue->m_filterText, m_strOrig);
    hti = TreeView_GetChild(m_hwnd, hti);
#ifndef COMMUNITY
	wyString ttt;
	ttt.SetAs(str);
	//ttt.SetAs("(?c)T+"); /*PCRE_CASELESS*//*PCRE_NO_AUTO_CAPTURE*/PCRE_MULTILINE
if(m_isRegexChecked)	
pregex = pcre_compile(ttt.GetString(),PCRE_NO_AUTO_CAPTURE,&str11,&offset,NULL); 
#endif
    while(hti)
    {      
        tvi.hItem = hti;
        tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvi.pszText = objName;
        tvi.cchTextMax = 199;
        tvi.stateMask = TVIS_EXPANDED | TVIS_EXPANDEDONCE | TVIS_OVERLAYMASK;
        TreeView_GetItem(m_hwnd, &tvi);
                
        wcscpy(stri, tvi.pszText);
        _wcslwr_s(stri, 200);
		temp = wcsstr(stri, str);
#ifndef COMMUNITY		
		wyString tt;
		tt.SetAs(stri);			
		if(m_isRegexChecked)
		{
			int check=pcre_exec(pregex, NULL, tt.GetString(), tt.GetLength(), offset, PCRE_NO_UTF8_CHECK, ovector, sizeof(ovector) / sizeof(wyInt32));  
			if(check!=1)
			{
				nexthti = TreeView_GetNextSibling(m_hwnd, hti);
				tempDlt = new DltElement(tvi);
				lparamValue->m_deleteList.Insert(tempDlt);
				OnFilterDeleteItem(hti, tempDlt);
				TreeView_DeleteItem(m_hwnd, hti);
				hti = nexthti;
			}
			else
			{   
				m_matchString.SetAs(tvi.pszText);
				hti = TreeView_GetNextSibling(m_hwnd, hti);
			}
		}
		else
#endif
			{
				if(!temp)
				{
					nexthti = TreeView_GetNextSibling(m_hwnd, hti);
					tempDlt = new DltElement(tvi);
					lparamValue->m_deleteList.Insert(tempDlt);
					OnFilterDeleteItem(hti, tempDlt);
					TreeView_DeleteItem(m_hwnd, hti);
					hti = nexthti;
				}
				else
				{   
					if(temp == stri && (wcscmp(tvi.pszText, m_matchString.GetAsWideChar()) < 0 || !m_matchString.GetLength()))
					{
						m_matchString.SetAs(tvi.pszText);
					}
					hti = TreeView_GetNextSibling(m_hwnd, hti);
				}

			}
    }
}

void
CQueryObject::OnFilterDeleteItem(HTREEITEM hti, wyElem *dltElement)
{
    HTREEITEM       hchild = NULL, htemp = NULL;
    TVITEM          tvi;
    wyWChar         objName[200];
    wyElem          *tempDlt = NULL;

    for(hchild = TreeView_GetChild(m_hwnd, hti); hchild;)
    {
        tvi.hItem = hchild;
        tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvi.pszText = objName;
        tvi.cchTextMax = 199;
        tvi.stateMask = TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_OVERLAYMASK;
        TreeView_GetItem(m_hwnd, &tvi);
        tempDlt = new DltElement(tvi);
        ((DltElement *)dltElement)->m_child.Insert(tempDlt);
        OnFilterDeleteItem(hchild, tempDlt);
        htemp = TreeView_GetNextSibling(m_hwnd, hchild);
        TreeView_DeleteItem(m_hwnd, hchild);
        hchild = htemp;
    }
}

void
CQueryObject::OnFilterInsertItem(HTREEITEM hparent, wyElem *dltElement)
{
    HTREEITEM       hti = NULL;
    DltElement      *tmpDlt = NULL;
    TVINSERTSTRUCT  tvins;
    DltElement      *tmpelement = NULL;
    wyInt32         image = GetItemImage(m_hwnd, hparent);

    tvins.hInsertAfter = image == NDATABASE || image == NFOLDER? TVI_LAST: TVI_SORT;
    tvins.hParent = hparent;
            
    for(tmpDlt = (DltElement *)((DltElement *)dltElement)->m_child.GetFirst(); 
        tmpDlt;) 
        
    {
        tvins.item = tmpDlt->m_tvi;
        hti =  TreeView_InsertItem(m_hwnd, &tvins);
    
        if(tmpDlt->m_child.GetCount() > 0)
            OnFilterInsertItem(hti, tmpDlt);

        tmpelement = (DltElement *)tmpDlt->m_next;
        ((DltElement *)dltElement)->m_child.Remove(tmpDlt);
        delete(tmpDlt);
        tmpDlt = tmpelement;
    }
}


void
CQueryObject::FilterDeleteList(wyWChar str[])
{
    HTREEITEM   hparent = NULL, hti = NULL;
    DltElement  *pdltelement = NULL, *tmpelement = NULL;
    wyWChar     *temp = NULL, stri[200], strChild[70];
    TVINSERTSTRUCT		tvins;
    OBDltElementParam *lparamValue = NULL;
    TVITEM  tvi;
    
    memset(stri, 0, sizeof(stri));
    memset(strChild, 0, sizeof(strChild));
    memset(&tvi, 0, sizeof(TVITEM));

    hparent = GetSelectionOnFilter(m_hwnd);

    hti = TreeView_GetChild(m_hwnd, hparent);
    while(hti)
    {
        tvi.mask = TVIF_TEXT;
        tvi.hItem = hti;
        tvi.cchTextMax = 64;
        tvi.pszText = strChild;
        TreeView_GetItem(m_hwnd, &tvi);

        wcscpy(stri, strChild);
        _wcslwr_s(stri, 64);
        temp = wcsstr(stri, str);
        if(temp == stri && (wcscmp(stri, m_matchString.GetAsWideChar()) < 0 || !m_matchString.GetLength()))
        {
            m_matchString.SetAs(stri);
        }
        hti = TreeView_GetNextSibling(m_hwnd, hti);
    }

    memset(&tvi, 0, sizeof(TVITEM));
    memset(stri, 0, sizeof(stri));
    temp = NULL;

    tvi.hItem = hparent;
    tvi.mask = TVIF_PARAM | TVIF_IMAGE;
    TreeView_GetItem(m_hwnd, &tvi);
    lparamValue = (OBDltElementParam *)tvi.lParam;
    wcscpy(lparamValue->m_filterText, m_strOrig);

    tvins.hParent       = hparent;
    tvins.hInsertAfter	= TVI_SORT;
	        
    for(pdltelement = (DltElement*)lparamValue->m_deleteList.GetFirst(); pdltelement;)
    {
        wcscpy(stri, pdltelement->m_tvi.pszText);
        _wcslwr_s(stri, 200);
        temp = wcsstr(stri, str);
        
        if(temp)
        {
            tvins.item			= pdltelement->m_tvi;
            hti =  TreeView_InsertItem(m_hwnd, &tvins);

            if(temp == stri && (wcscmp(pdltelement->m_tvi.pszText, m_matchString.GetAsWideChar()) < 0 || !m_matchString.GetLength()))
            {
                m_matchString.SetAs(pdltelement->m_tvi.pszText);
            }

            OnFilterInsertItem(hti, pdltelement);

            tmpelement = (DltElement *)pdltelement->m_next;
            lparamValue->m_deleteList.Remove(pdltelement);
            delete(pdltelement);
            pdltelement = tmpelement;
        }
        else
        {
            pdltelement = (DltElement*)pdltelement->m_next;
        }
    }
}

void
CQueryObject::FilterBothTreeViewAndDeleteList(wyWChar str[])
{
    List        itemsToInsert;
    wyInt32     image = -1;
    HTREEITEM   hparent = NULL, hti = NULL;
    DltElement  *pdltelement = NULL, *tmpelement = NULL;
    wyWChar     *temp = NULL, stri[200];
    TVINSERTSTRUCT		tvins;
    TVITEM              tvi;
    OBDltElementParam   *lparamValue = NULL;
    
    memset(stri, 0, sizeof(stri));

    hti = GetSelectionOnFilter(m_hwnd);
    image = GetItemImage(m_hwnd, hti);

    hparent = hti;
    tvi.hItem = hti;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(m_hwnd, &tvi);
    lparamValue = (OBDltElementParam *)tvi.lParam;
    wcscpy(lparamValue->m_filterText, m_strOrig);
    
    for(pdltelement = (DltElement*)lparamValue->m_deleteList.GetFirst(); pdltelement;)
    {
        wcscpy(stri, pdltelement->m_tvi.pszText);
        _wcslwr_s(stri, 200);
        temp = wcsstr(stri, str);
        if(temp)
        {   
            tmpelement = (DltElement *)pdltelement->m_next;
            lparamValue->m_deleteList.Remove(pdltelement);
            itemsToInsert.Insert(pdltelement);
            if(temp == stri && (wcscmp(pdltelement->m_tvi.pszText, m_matchString.GetAsWideChar()) < 0 || !m_matchString.GetLength()))
            {
                m_matchString.SetAs(pdltelement->m_tvi.pszText);
            }
            pdltelement = tmpelement;
        }
        else
        {
            pdltelement = (DltElement*)pdltelement->m_next;
        }
    }

    FilterTreeView(str);

    for(pdltelement = (DltElement *)itemsToInsert.GetFirst(); pdltelement;)
    {
        tvins.hParent       = hparent;
	    tvins.hInsertAfter	= TVI_SORT;
	    tvins.item			= pdltelement->m_tvi;
        hti =  TreeView_InsertItem(m_hwnd, &tvins);

        OnFilterInsertItem(hti, pdltelement);
        tmpelement = (DltElement *)pdltelement->m_next;
        itemsToInsert.Remove(pdltelement);
        delete(pdltelement);
        pdltelement = tmpelement;
    }    
}

wyBool
CQueryObject::OnItemSelectionChanging(LPNMTREEVIEW lpnmtv)
{
    wyInt32         toImage = 0, fromImage = 0;
    HTREEITEM       toParent = NULL, fromParent = NULL, hti = NULL;
    TVITEM          tvi;
    OBDltElementParam   *lparamValueTo = NULL, *lp1 = NULL, *lp2 = NULL;
    wyWChar         msg[SIZE_192];

    if(m_OBFilterWorkInProgress || !lpnmtv->itemOld.hItem)
    {   
        return wyFalse;
    }
    
    toImage = GetItemImage(m_hwnd, lpnmtv->itemNew.hItem);
    fromImage = GetItemImage(m_hwnd, lpnmtv->itemOld.hItem);
    toParent = TreeView_GetParent(m_hwnd, lpnmtv->itemNew.hItem);
    fromParent = TreeView_GetParent(m_hwnd, lpnmtv->itemOld.hItem);

    switch(fromImage)
    {
    case NTABLE:
    case NSPITEM:
    case NFUNCITEM:
    case NEVENTITEM:
    case NTRIGGERITEM:
    case NVIEWSITEM:
        hti = TreeView_GetParent(m_hwnd, fromParent);
        lp1 = (OBDltElementParam *)TV_GetLParam(m_hwnd, fromParent);
        break;
    case NCOLUMN:
    case NPRIMARYINDEX:
    case NPRIMARYKEY:
    case NINDEX:
        hti = TreeView_GetParent(m_hwnd, fromParent);
        hti = TreeView_GetParent(m_hwnd, hti);
        lp1 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        break;
    case NFOLDER:
        hti = TreeView_GetParent(m_hwnd, fromParent);
        lp1 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        break;
    case NDATABASE:
        hti = TreeView_GetChild(m_hwnd, lpnmtv->itemOld.hItem);
        lp1 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        break;
    case NSERVER:
    case NTABLES:
    case NSP:
    case NVIEWS:
    case NEVENTS:
    case NTRIGGER:
    case NFUNC:
        lp1 = (OBDltElementParam *)TV_GetLParam(m_hwnd, lpnmtv->itemOld.hItem);
        break;
    }

    switch(toImage)
    {
    case NSERVER:
        wcscpy(msg, _(L"Filter Databases"));
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, lpnmtv->itemNew.hItem);
        break;
    case NTABLE:
    case NSPITEM:
    case NFUNCITEM:
    case NEVENTITEM:
    case NTRIGGERITEM:
    case NVIEWSITEM:
        hti = TreeView_GetParent(m_hwnd, toParent);
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, toParent);
        PrepareMessageText(toImage, hti, msg, lpnmtv->itemNew.hItem);
        break;
    case NCOLUMN:
    case NPRIMARYINDEX:
    case NPRIMARYKEY:
    case NINDEX:
        hti = TreeView_GetParent(m_hwnd, toParent);
        hti = TreeView_GetParent(m_hwnd, hti);
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        hti = TreeView_GetParent(m_hwnd, hti);
        PrepareMessageText(NTABLES, hti, msg, NULL);
        break;
    case NFOLDER:
        hti = TreeView_GetParent(m_hwnd, toParent);
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        hti = TreeView_GetParent(m_hwnd, hti);
        PrepareMessageText(NTABLES, hti, msg, NULL);
        break;
    case NDATABASE:
        hti = TreeView_GetChild(m_hwnd, lpnmtv->itemNew.hItem);
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, hti);
        PrepareMessageText(NTABLES, lpnmtv->itemNew.hItem, msg, hti); 
        break;
    case NTABLES:
    case NSP:
    case NVIEWS:
    case NEVENTS:
    case NTRIGGER:
    case NFUNC:
        lp2 = (OBDltElementParam *)TV_GetLParam(m_hwnd, lpnmtv->itemNew.hItem);
        PrepareMessageText(toImage, toParent, msg, lpnmtv->itemNew.hItem);
        break;
    }

    SetWindowText(m_hwndStMsg, msg);
    
    if(lp1 != lp2)
    {
        switch(toImage)
        {
        case NDATABASE:
            hti = TreeView_GetChild(m_hwnd, lpnmtv->itemNew.hItem);
            break;
        case NTABLE:
        case NVIEWSITEM:
        case NTRIGGERITEM:
        case NFUNCITEM:
        case NEVENTITEM:
        case NSPITEM:
            hti = toParent;
            break;
        case NCOLUMN:
        case NINDEX:
        case NPRIMARYINDEX:
        case NPRIMARYKEY:
            hti = TreeView_GetParent(m_hwnd, toParent);
            hti = TreeView_GetParent(m_hwnd, hti);
            break;
        case NFOLDER:
            hti = TreeView_GetParent(m_hwnd, toParent);
            break;
        default:
            hti = lpnmtv->itemNew.hItem;
            break;
        }

        memset(&tvi, 0, sizeof(TVITEM));
        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        TreeView_GetItem(m_hwnd, &tvi);
        
        lparamValueTo = (OBDltElementParam *)tvi.lParam;
        m_isSelValid = wyFalse;
        
        if(lparamValueTo)
        {
            m_prevString.SetAs(lparamValueTo->m_filterText);
            SetWindowText(m_hwndFilter, lparamValueTo->m_filterText);
        }
        else
        {
            m_prevString.SetAs(L"");
            SetWindowText(m_hwndFilter, L"");
        }
    }

    SetFocus(m_hwnd);
    return wyFalse;
}

void
CQueryObject::DeAllocateLParam(HTREEITEM hti, wyInt32 image)
{
    HTREEITEM   hchild = NULL;
    TVITEM      tvi;
    wyInt32     childImage;
    OBDltElementParam   *tempElem = NULL, *tempNext = NULL;


    memset(&tvi, 0, sizeof(TVITEM));
    switch(image)
    {
    case NSERVER:
        for(tempElem = (OBDltElementParam *)m_allocatedList.GetFirst(); tempElem; )
        {
            tempNext = (OBDltElementParam *)tempElem->m_next;
            m_allocatedList.Remove(tempElem);
            delete(tempElem);
            tempElem = tempNext;
        }
        break;
    case NDATABASE:
    //case NTABLE:
        for(hchild = TreeView_GetChild(m_hwnd, hti); hchild; hchild = TreeView_GetNextSibling(m_hwnd, hchild))
        {
            childImage = GetItemImage(m_hwnd, hchild);
            DeAllocateLParam(hchild, childImage);
        }
        break;
    case NTABLES:
        /*for(hchild = TreeView_GetChild(m_hwnd, hti); hchild; hchild = TreeView_GetNextSibling(m_hwnd, hchild))
        {
            childImage = GetItemImage(m_hwnd, hchild);
            DeAllocateLParam(hchild, childImage);
        }*/

        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        TreeView_GetItem(m_hwnd, &tvi);

        if(tvi.lParam)
        {
            m_allocatedList.Remove((OBDltElementParam *)tvi.lParam);
            delete((OBDltElementParam *)tvi.lParam);
            tvi.lParam = 0;
            TreeView_SetItem(m_hwnd, &tvi);
        }
        break;
    case NSP:
    case NFUNC:
    case NEVENTS:
    case NTRIGGER:
    case NVIEWS:
    //case NFOLDER:
        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        TreeView_GetItem(m_hwnd, &tvi);

        if(tvi.lParam)
        {
            m_allocatedList.Remove((OBDltElementParam *)tvi.lParam);
            delete((OBDltElementParam *)tvi.lParam);
            tvi.lParam = 0;
            TreeView_SetItem(m_hwnd, &tvi);
        }
        break;
    }
}

void
CQueryObject::PrepareMessageText(wyInt32 toImage, HTREEITEM hti, wyWChar msg[], HTREEITEM hItem)
{
    TVITEM  tvi;
    wyWChar name[70];
    
    tvi.hItem = hti;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = name;
    tvi.cchTextMax = 65;
    TreeView_GetItem(m_hwnd, &tvi);

    wcscpy(msg, L"");
    
    switch(toImage)
    {
    case NTABLE:
    case NTABLES:
        wcscat(msg, _(L"Filter tables in "));
        break;
    case NVIEWS:
    case NVIEWSITEM:
        wcscat(msg, _(L"Filter views in "));
        break;
    case NFUNC:
    case NFUNCITEM:
        wcscat(msg, _(L"Filter functions in "));
        break;
    case NEVENTS:
    case NEVENTITEM:
        wcscat(msg, _(L"Filter events in "));
        break;
    case NTRIGGER:
    case NTRIGGERITEM:
        wcscat(msg, _(L"Filter triggers in "));
        break;
    case NSP:
    case NSPITEM:
        wcscat(msg, _(L"Filter procedures in "));
        break;
    }
    wcscat(msg, tvi.pszText);
}