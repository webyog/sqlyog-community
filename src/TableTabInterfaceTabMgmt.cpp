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

#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabFields.h"
#include "TabAdvancedProperties.h"
#include "TabIndexes.h"
#include "TabForeignKeys.h"
#include "TabPreview.h"
#include "DoubleBuffer.h"

# define MAX_HEIGHT		335

TableTabInterfaceTabMgmt::TableTabInterfaceTabMgmt(HWND hwndparent, TableTabInterface* ptabint)
{
    m_isbuffereddraw = wyFalse;
    m_hwndparent    = hwndparent;
    m_tabinterfaceptr = ptabint;

    m_hwnd          = NULL;
    m_hcommonwnd    = NULL;
    m_hwndtool      = NULL;
    m_tabindexes    = NULL;
    m_tabfk         = NULL;
    m_tabpreview    = NULL;
    m_tabadvprop    = NULL;
    m_tabfields     = NULL;
    m_allsubtabsloaded = wyFalse;
	m_wnd = m_tabinterfaceptr->m_mdiwnd;
}

TableTabInterfaceTabMgmt::~TableTabInterfaceTabMgmt()
{
    if(m_tabadvprop)
    {
        delete m_tabadvprop;
        m_tabadvprop = NULL;
    }
    if(m_tabfields)
    {
        delete m_tabfields;
        m_tabfields = NULL;
    }
    if(m_tabfk)
    {
        delete m_tabfk;
        m_tabfk = NULL;
    }
    if(m_tabindexes)
    {
        delete m_tabindexes;
        m_tabindexes = NULL;
    }
    if(m_tabpreview)
    {
        delete m_tabpreview;
        m_tabpreview = NULL;
    }
    
}

wyInt32
TableTabInterfaceTabMgmt::OnTabSelChanging()
{
    wyInt32 index = CustomTab_GetCurSel(m_hwnd);
    CTCITEM					ctci={0};
    
    ctci.m_mask =  CTBIF_IMAGE | CTBIF_LPARAM;
	CustomTab_GetItem(m_hwnd, index, &ctci);

    switch(ctci.m_iimage)
	{
	    case IDI_TABLEOPTIONS:    //..Advanced Properties Tab
			m_tabadvprop->OnTabSelChanging();
			break;

	    case IDI_COLUMN:     //..Fields Tab
		    if(!m_tabfields->OnTabSelChanging())
			    return 0;
			break;

		case IDI_MANINDEX_16:       //..Indexes Tab
			m_tabindexes->OnTabSelChanging();
			break;

	    case IDI_MANREL_16:         //..Foreign Keys Tab
		    m_tabfk->OnTabSelChanging();
			break;

	    case IDI_TABPREVIEW:             //..Preview Tab
		    break;
    }	

    return 1;
}

void
TableTabInterfaceTabMgmt::OnTabSelChange(wyBool visible)
{
    wyInt32     index;
    CTCITEM     ctci;
    
    if(pGlobals->m_pcmainwin->m_finddlg)
    {
        DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
        pGlobals->m_pcmainwin->m_finddlg = NULL;
    }

    index = CustomTab_GetCurSel(m_hwnd);

    ctci.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
    CustomTab_GetItem(m_hwnd, index, &ctci);
    
    switch(ctci.m_iimage)
    {
	    case IDI_TABLEOPTIONS:
		    m_tabadvprop->OnTabSelChange();
			break;

	    case IDI_COLUMN:
		    m_tabfields->OnTabSelChange();
			break;

	    case IDI_TABPREVIEW:     ////..PreviewTab
		    m_tabpreview->OnTabSelChange();
			break;

	    case IDI_MANINDEX_16:
		    m_tabindexes->OnTabSelChange();
			break;

	    case IDI_MANREL_16:         //ForeignKeys Tab
		    m_tabfk->OnTabSelChange();
			break;
    }

	//SendMessage(m_hwnd, WM_SETREDRAW, TRUE, NULL);
}

wyBool
TableTabInterfaceTabMgmt::Create()
{
    // Creating tab-management
	if(!CreateTableTab())
        return wyFalse;

    /// Creating common window
    if(!CreateCommonWindow())
        return wyFalse;

    /// Creating all sub-tab

    if(!CreateFieldsTab())
        return wyFalse;

    if(!CreateIndexesTab())
        return wyFalse;

    if(!CreateForeignKeysTab())
        return wyFalse;

    if(!CreateAdvancedPropertiesTab())
        return wyFalse;

    if(!CreatePreviewTab())
        return wyFalse;/**/

    /// Setting flag
    m_allsubtabsloaded = wyTrue;

    return wyTrue;
}

wyBool
TableTabInterfaceTabMgmt::CreateCommonWindow()
{
    wyUInt32    style   = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
    HINSTANCE   hinst   = pGlobals->m_entinst?pGlobals->m_entinst:pGlobals->m_hinstance;

    /// Creating Common Window
    m_hcommonwnd        =   CreateWindowEx(WS_EX_CONTROLPARENT, TTI_SUBTABS_COMMONWNDCLS, L"", style,
                                        0, 0, 0, 0,
                                        m_hwnd, (HMENU)0,
                                        hinst, this);
    if(!m_hcommonwnd)
        return wyFalse;

    UpdateWindow(m_hcommonwnd);
    ShowWindow(m_hcommonwnd, SW_SHOW);

    return wyTrue;
}

// Wndproc for the tab control.
LRESULT
TableTabInterfaceTabMgmt::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool *pishandled)
{
    TableTabInterface           *ptabint = NULL;
    TableTabInterfaceTabMgmt    *pctabintmgmt = NULL;
	MDIWindow		            *wnd = GetActiveWin();
    
	*pishandled = wyFalse;
    
    if(wnd && (wnd->m_pctabmodule->GetActiveTabImage() == IDI_CREATETABLE || wnd->m_pctabmodule->GetActiveTabImage() == IDI_ALTERTABLE))
    {
        ptabint = (TableTabInterface*) wnd->m_pctabmodule->GetActiveTabType();
        pctabintmgmt = (TableTabInterfaceTabMgmt*) ptabint->m_ptabintmgmt;
    }
	else
		return 0;
    
    switch(message)
    {
    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr =(LPNMHDR)lparam;
            switch(lpnmhdr->code)
			{
				case CTCN_WMDESTROY:
                    pctabintmgmt->m_hwnd = NULL;
                    break;
			}
        }
        break;
    }
    return 0;    
}

VOID
TableTabInterfaceTabMgmt::Resize(HWND  hwnd)
{
    RECT			rcmain, rcstatus;
	wyInt32			hpos, vpos, width, height;

    VERIFY(GetWindowRect(GetParent(hwnd), &rcmain));
    VERIFY(MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rcmain, 2));

    VERIFY(GetWindowRect(pGlobals->m_pcmainwin->m_hwndstatus, &rcstatus));
	VERIFY(MapWindowPoints(NULL, GetParent(m_hwndparent), (LPPOINT)&rcstatus, 2));

    hpos    =   rcmain.left;
    vpos    =   CustomTab_GetTabHeight(GetParent(hwnd));
    height  =   rcmain.bottom - vpos; // 25 points : 25 points is the height of the Custom-Tab which shows sub-tabs
    width   =   rcmain.right;

    MoveWindow(hwnd, hpos, vpos, width, height, TRUE);
}

LRESULT	CALLBACK
TableTabInterfaceTabMgmt::ToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TableTabInterfaceTabMgmt *ptm = (TableTabInterfaceTabMgmt*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HBRUSH hbr;
    DUALCOLOR dc = {0};
    wyInt32 ret;

    if(message == WM_CTLCOLORSTATIC && !ptm->m_tabinterfaceptr->m_open_in_dialog && wyTheme::GetBrush(BRUSH_TOOLBAR, &hbr))
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        
        if(wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc))
        {
            if(IsWindowEnabled(hwnd))
            {
                if(dc.m_flag & 1)
                {
                    SetTextColor((HDC)wParam, dc.m_color1);
                }
            }
            else
            {
                if(dc.m_flag & 2)
                {
                    SetTextColor((HDC)wParam, dc.m_color2);
                }
            }
        }

        return (LRESULT)hbr;
    }
    else if(message == WM_NOTIFY)
    {
        if(!ptm->m_tabinterfaceptr->m_open_in_dialog
            && wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc) && 
            (ret = wyTheme::PaintButtonCaption(lParam, &dc)) != -1)
        {
            return ret;
        }
    }

    return CallWindowProc(ptm->m_origtoolwndproc, hwnd, message, wParam, lParam);
}

LRESULT	CALLBACK
TableTabInterfaceTabMgmt::SubTabsCommonWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TableTabInterfaceTabMgmt    *tabmgmt    = (TableTabInterfaceTabMgmt *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    TabAdvancedProperties       *tabadvprop =   NULL;
    wyInt32 ret;

    if(tabmgmt)
    {
        tabadvprop = tabmgmt->m_tabadvprop;
    }

    switch(message)
    {
    case WM_NCCREATE:
        tabmgmt = (TableTabInterfaceTabMgmt*)((CREATESTRUCT *) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) tabmgmt);
		if(tabadvprop)
			SetScrollRange(tabadvprop->m_hwndscroll, SB_CTL,0, MAX_HEIGHT, TRUE);
        break;

    case WM_CREATE:
        tabmgmt->CreateOtherWindows(hwnd);
        break;

    case UM_SETFOCUS:
        SetFocus((HWND)wParam);
        break;
    
    case WM_PAINT:
        {
			PAINTSTRUCT ps = {0};
            HDC hdc;
			
			hdc = BeginPaint(hwnd, &ps);
			
            if(tabmgmt->m_isbuffereddraw == wyTrue)
            {
                DoubleBuffer db(hwnd, hdc);	
                db.EraseBackground(RGB(255, 255, 255));
                db.PaintWindow();
            }
            else
            {
                DoubleBuffer::EraseBackground(hwnd, hdc, NULL, RGB(255, 255, 255));
            }

            EndPaint(hwnd, &ps);
        }
        return 1;

    case UM_ISIGNOREHWNDFROMPAINT:
        *((wyInt32*)lParam) = 1;
        return 1;
        
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_COMMAND:
        tabmgmt->OnWMCommand(hwnd, message, wParam, lParam);
        break;

    case WM_DESTROY:
        {
            if(tabmgmt->m_tabfields->m_p)
                delete tabmgmt->m_tabfields->m_p;
            tabmgmt->m_tabfields->m_p = NULL;
            tabmgmt->m_hcommonwnd = NULL;
        }
        break;

    case WM_HELP:
        {
            switch(tabmgmt->GetActiveTabImage())
            {
            case IDI_COLUMN:
                {
                if(tabmgmt->m_tabinterfaceptr->m_isaltertable)
                    ShowHelp("http://sqlyogkb.webyog.com/article/88-alter-table-in-database");
                else
                   ShowHelp("http://sqlyogkb.webyog.com/article/85-create-table");
                }
                break;

            case IDI_MANINDEX_16:
                {
                    if(tabmgmt->m_tabinterfaceptr->m_isaltertable)
                        ShowHelp("http://sqlyogkb.webyog.com/article/92-alter-index");
                    else
                        ShowHelp("http://sqlyogkb.webyog.com/article/91-create-index");
                }
                break;

            case IDI_MANREL_16:
                ShowHelp("http://sqlyogkb.webyog.com/article/90-fk-in-mysql-and-sqlyog");
                break;

            case IDI_TABPREVIEW:
                ShowHelp("http://sqlyogkb.webyog.com/article/97-preview");
                break;

            case IDI_TABLEOPTIONS:
                ShowHelp("http://sqlyogkb.webyog.com/article/96-advanced-tab");
                break;
            }
            return 1;
        }
        break;

	case WM_MOUSEWHEEL:
		if(tabadvprop)
			tabadvprop->OnMouseWheel(wParam);
		break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_NOTIFY:
        if(!tabmgmt->m_tabinterfaceptr->m_open_in_dialog 
            && (ret = wyTheme::DrawToolBar(lParam)) != -1)
        {
            return ret;
        }
        else if(((LPNMHDR)lParam)->code == TTN_GETDISPINFO)
        {
            tabmgmt->OnToolTipInfo((LPNMTTDISPINFO)lParam);
            return 1;
        }

        break;

	case WM_VSCROLL:
		if(tabadvprop)
        {
            tabmgmt->m_isbuffereddraw = wyTrue;
			tabadvprop->OnVScroll(wParam);
            tabmgmt->m_isbuffereddraw = wyFalse;
        }
		break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void 
TableTabInterfaceTabMgmt::OnToolTipInfo(LPNMTTDISPINFO lpnmtt)
{
    wyInt32 image;

    m_tooltipstr.Clear();
    image = GetActiveTabImage();

    switch(lpnmtt->hdr.idFrom)
    {
        case IDI_ADDROW:
            if(image == IDI_COLUMN)
            {
                m_tooltipstr.SetAs(_(L"Insert new column (Alt+Ins)"));
            }
            else if(image == IDI_MANINDEX_16)
            {
                m_tooltipstr.SetAs(_(L"Insert new index (Alt+Ins)"));
            }
            else if(image == IDI_MANREL_16)
            {
                m_tooltipstr.SetAs(_(L"Insert new FK (Alt+Ins)"));
            }

            break;

        case IDI_DELETEROW:
            if(image == IDI_COLUMN)
            {
                m_tooltipstr.SetAs(_(L"Delete selected columns(s) (Alt+Del)"));
            }
            else if(image == IDI_MANINDEX_16)
            {
                m_tooltipstr.SetAs(_(L"Delete selected indexes(s) (Alt+Del)"));
            }
            else if(image == IDI_MANREL_16)
            {
                m_tooltipstr.SetAs(_(L"Delete selected FK(s) (Alt+Del)"));
            }

            break;

        case IDI_MOVEUP:
            m_tooltipstr.SetAs(_(L"Move up (Alt+Up)"));
            break;

        case IDI_MOVEDOWN:
            m_tooltipstr.SetAs(_(L"Move down (Alt+Down)"));
            break;
    }

    lpnmtt->lpszText = m_tooltipstr.GetAsWideChar();
}

wyBool 
TableTabInterfaceTabMgmt::OnSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyInt32 cmd = 0;

    if(lparam >> 29)
    {
        switch(wparam)
        {
            case VK_INSERT:
                cmd = IDI_ADDROW;
                break;

            case VK_DELETE:
                cmd = IDI_DELETEROW;
                break;

            case VK_UP:
                cmd = IDI_MOVEUP;
                break;

            case VK_DOWN:
                cmd = IDI_MOVEDOWN;
                break;
        }

        if(cmd)
        {
            SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(cmd, 0), (LPARAM)m_hwndtool);
            return wyTrue;
        }
    }

    return wyFalse;
}

wyBool
TableTabInterfaceTabMgmt::OnWMCommand(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    wyString    newval;

    switch(LOWORD(wParam))
    {
    case IDI_ADDROW:
        OnClickAddRow();
        break;

    case IDI_DELETEROW:
        OnClickDeleteRows();
        break;

    case IDI_MOVEUP:
        OnClickMoveUp();
        break;

    case IDI_MOVEDOWN:
        OnClickMoveDown();
        break;

    case IDC_HIDECOLUMNS:
        /// Showing/Hiding language options
        m_tabfields->ShowHideCharsetAndCollation();
        break;

    case IDC_ROWFORMAT:
    case IDC_DELAYKEY:
    case IDC_CHECKSUM:
        {
            switch(HIWORD(wParam))
            {
            case CBN_DROPDOWN:
                if(GetActiveTabImage() == IDI_TABLEOPTIONS)
                {
                    /// Storing previous value of the combo-box
                    m_tabinterfaceptr->GetComboboxValue((HWND)lParam, m_tabadvprop->m_prevval);
                }
                break;

                case CBN_CLOSEUP:
                case CBN_SELENDOK:
                    if(!m_tabadvprop->m_disableenchange && !m_tabinterfaceptr->m_dirtytab)
                    {
                        /// Marking tab as dirty only when the combo-box value is changed.
                        m_tabinterfaceptr->GetComboboxValue((HWND)lParam, newval);
                        if(m_tabadvprop->m_prevval.Compare(newval) != 0)
                            m_tabinterfaceptr->MarkAsDirty(wyTrue);
                    }
                    break;
            }
        }
        break;

    case IDCANCEL:
        /// Send UM_CLOSEDLG to tabbed-interface wndproc if the tabbed-int is opened in sd-dialog
        if(m_tabinterfaceptr && m_tabinterfaceptr->m_open_in_dialog)
            PostMessage(m_tabinterfaceptr->m_hwnd, UM_CLOSEDLG, wParam, lParam);
        return wyFalse;
    }
    
    switch(HIWORD(wParam))
    {
    case EN_CHANGE:
        if(m_tabpreview && !m_tabpreview->m_settingpreviewcontent && !m_tabadvprop->m_disableenchange  && !m_tabinterfaceptr->m_dirtytab)
        {
			m_tabinterfaceptr->MarkAsDirty(wyTrue);
        }
        break;

    case EN_SETFOCUS:
    case CBN_SETFOCUS:
        if(GetActiveTabImage() == IDI_TABLEOPTIONS)
            m_tabadvprop->m_hlastfocusedwnd = (HWND)lParam;
        break;
    }
    
    return wyTrue;
}

void
TableTabInterfaceTabMgmt::OnClickAddRow()
{
    /// Check the current active sub-tab and call insert-function accordingly
    switch(GetActiveTabImage())
    {
    case IDI_COLUMN:
        m_tabfields->ProcessInsert();
        break;
    case IDI_MANINDEX_16:
        m_tabindexes->ProcessInsert();
        break;
    case IDI_MANREL_16:
        m_tabfk->ProcessInsert();
        break;
    default:
        return;
    }
}

void
TableTabInterfaceTabMgmt::OnClickDeleteRows()
{
    /// Check the current active sub-tab and call insert-function accordingly
    switch(GetActiveTabImage())
    {
    case IDI_COLUMN:
        m_tabfields->ProcessDelete();
        break;
    case IDI_MANINDEX_16:
        m_tabindexes->ProcessDelete();
        break;
    case IDI_MANREL_16:
        m_tabfk->ProcessDelete();
        break;
    default:
        return;
    }
}
//enumeration procedure that hides other child windows

void
TableTabInterfaceTabMgmt::OnClickMoveUp()
{
    switch(GetActiveTabImage())
    {
    case IDI_COLUMN:
        m_tabfields->OnClickMoveUp();
        break;
    }
}

void
TableTabInterfaceTabMgmt::OnClickMoveDown()
{
    switch(GetActiveTabImage())
    {
    case IDI_COLUMN:
        m_tabfields->OnClickMoveDown();
        break;
    }
}

BOOL CALLBACK 
TableTabInterfaceTabMgmt::ShowWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND*       hwndarr;
    wyUInt32    ind = 0;
    
    hwndarr = NULL;
    hwndarr = (HWND*)lParam;
    if(!hwndarr)
        return TRUE;

    while(hwndarr[ind])
    {
        if(hwnd == hwndarr[ind])
        {
            ShowWindow(hwnd, SW_SHOW);
            return TRUE;
        }
        ind++;
    }
    ShowWindow(hwnd, SW_HIDE);
    return TRUE;
}

void
TableTabInterfaceTabMgmt::ShowHideToolButtons(wyBool hide)
{
    /// Showing/Hiding button toolbars
    SendMessage(m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDM_SEPARATOR, (LPARAM)MAKELONG(hide ? TRUE : FALSE,0));
    SendMessage(m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(hide ? TRUE : FALSE,0));
    SendMessage(m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(hide ? TRUE : FALSE,0));
}

wyBool
TableTabInterfaceTabMgmt::CreateOtherWindows(HWND hparent)
{
    CreateToolBar(hparent);
    return wyTrue;
}

void
TableTabInterfaceTabMgmt::Resize()
{
    RECT			rcmain, rcstatus;
	wyInt32			hpos, vpos, width, height;

    GetWindowRect(m_hwndparent, &rcmain);
    VERIFY(MapWindowPoints(NULL, m_hwndparent, (LPPOINT)&rcmain, 2));

    VERIFY(GetWindowRect(pGlobals->m_pcmainwin->m_hwndstatus, &rcstatus));
	VERIFY(MapWindowPoints(NULL, GetParent(m_hwndparent), (LPPOINT)&rcstatus, 2));
    
    hpos    = rcmain.left + 6; // increase padding
    vpos    = rcmain.top + 111;
	width   = (rcmain.right-hpos);

	/*if(!m_tabinterfaceptr->m_open_in_dialog)*/
		height  = rcmain.bottom - vpos - 50;    //..35 points in the bottom are reserved for Bottom Frame
	/*else
		height  = rcmain.bottom - vpos;*/
    
    VERIFY(MoveWindow(m_hwnd, hpos + 5, vpos, width - 16, height, TRUE));
    
    VERIFY(GetWindowRect(m_hwnd, &rcmain));
    VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcmain, 2));

    Resize(m_hcommonwnd);
    
    if(m_tabinterfaceptr->m_open_in_dialog)
        MoveWindow(m_hwndtool,  2 , 0, rcmain.right - 3, 25, TRUE);        //...Toolbar Position
    else
        MoveWindow(m_hwndtool,  0, 0, rcmain.right, 25, TRUE);        //...Toolbar Position
    
    if(m_tabfields)
        m_tabfields->Resize();
    
    if(m_tabindexes)
        m_tabindexes->Resize();
    if(m_tabfk)
        m_tabfk->Resize();
    if(m_tabadvprop)
        m_tabadvprop->Resize();
    if(m_tabpreview)
        m_tabpreview->Resize();/**/
}

wyBool
TableTabInterfaceTabMgmt::CreateTableTab()
{
    HWND            hwndafter;
    TABCOLORINFO    ci = {0};

    /// Creating subtabs main window
    VERIFY(m_hwnd = CreateCustomTab(m_hwndparent, 0, 0, 0, 0, (CTBWNDPROC)WndProc, IDC_TABLETAB));
    ci.m_border = wyTrue;
    ci.m_mask = CTCF_BORDER;
    CustomTab_SetColorInfo(m_hwnd, &ci);

    if(!m_hwnd)
    {
		return wyFalse;
	}
    else
    {
        if(!m_tabinterfaceptr->m_open_in_dialog)
        {
            if(wyTheme::GetTabColors(COLORS_TABLETAB, &ci))
            {
            //ci.m_activesep = ci.m_tabbg1;
                CustomTab_SetColorInfo(m_hwnd, &ci);
            }
	    	UpdateWindow(m_hwnd);
        }
     }

    if(IsMySQL41(m_wnd->m_tunnel, &(m_wnd->m_mysql)))
        hwndafter = m_tabinterfaceptr->m_hcmbcollation;
    else
        hwndafter = m_tabinterfaceptr->m_hcmbtabletype;

    /// Setting m_hwnd next to hwndafter (For Tab-Key, etc)
    SetWindowPos(m_hwnd, hwndafter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    return wyTrue;
}

wyBool
TableTabInterfaceTabMgmt::CreateFieldsTab()
{
    wyBool      retval = wyTrue;
    wyString    fieldstab;

    fieldstab.SetAs(_("&1 Columns"));

    m_tabfields = new TabFields(m_hcommonwnd, this);

    /// Creating Fields tab
    if(!m_tabfields->Create())
        return wyFalse;

    /// Inserting tab into tab-management
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_COLUMN, fieldstab, (LPARAM)m_tabfields);
    
    return retval;
}

wyBool
TableTabInterfaceTabMgmt::CreateIndexesTab()
{
    wyBool      retval = wyTrue;
    wyString    tabtitle;

    tabtitle.SetAs(_("&2 Indexes"));

    m_tabindexes = new TabIndexes(m_hcommonwnd, this);
    
    /// Creating Index-tab
    m_tabindexes->Create();

    /// Inserting Index-tab into tab-management
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_MANINDEX_16, tabtitle, (LPARAM)m_tabindexes);

    return retval;
}

wyBool
TableTabInterfaceTabMgmt::CreateForeignKeysTab()
{
    wyBool      retval = wyTrue;
    wyString    tabtitle;

    tabtitle.SetAs(_("&3 Foreign Keys"));
    
    m_tabfk = new TabForeignKeys(m_hcommonwnd, this);
    
    /// Creating Foreign-Keys tab
    if(!m_tabfk->Create())
        return wyFalse;

    /// Inserting tab into CustomTab
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_MANREL_16, tabtitle, (LPARAM)m_tabfk);
    return retval;
}

wyBool
TableTabInterfaceTabMgmt::CreatePreviewTab()
{
    wyBool      retval = wyTrue;
    wyString    tabtitle;

    tabtitle.SetAs(_("&5 SQL Preview"));

    m_tabpreview = new TabPreview(m_hcommonwnd, this);

    /// Creating Preview instance
    m_tabpreview->Create();

    /// Inserting tab into CustomTab
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_TABPREVIEW, tabtitle, (LPARAM)m_tabpreview);

    return retval;
}

wyBool
TableTabInterfaceTabMgmt::CreateAdvancedPropertiesTab()
{
    wyBool      retval = wyTrue;
    wyString    tabtitle;
   
	tabtitle.SetAs(_("&4 Advanced"));
    
    m_tabadvprop = new TabAdvancedProperties(m_hcommonwnd, this);


    /// Creating Advanced Properties instance
    if(!m_tabadvprop->Create())
        return wyFalse;
    
    /// Inserting tab into CustomTab
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_TABLEOPTIONS, tabtitle, (LPARAM)m_tabadvprop);
    return retval;
}

void
TableTabInterfaceTabMgmt::SelectTab(wyInt32 index)
{
    /*if(GetActiveTabImage() == IDI_COLUMN && index == 0)
        return;*/

    CustomTab_SetCurSel(m_hwnd, index);
	CustomTab_EnsureVisible(m_hwnd, index);
	return;
}

wyBool
TableTabInterfaceTabMgmt::CreateToolBar(HWND hwnd)
{
    wyUInt32 style = WS_CHILD | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_TOOLTIPS;
	
    if(m_tabinterfaceptr->m_open_in_dialog == wyFalse)
    {
        style |= TBSTYLE_FLAT;
    }

	// create the tool bar to show options for the result set
	VERIFY(m_hwndtool = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, style, 0,0,0,0, hwnd,
											(HMENU)IDC_TOOLBAR, (HINSTANCE)pGlobals->m_hinstance, NULL));

	m_origtoolwndproc = (WNDPROC)SetWindowLongPtr(m_hwndtool, GWLP_WNDPROC, (LONG_PTR)TableTabInterfaceTabMgmt::ToolbarProc);
    SetWindowLongPtr(m_hwndtool, GWLP_USERDATA, (LONG_PTR)this);

	// buttons to the toolbar
	AddToolButtons();

	return wyTrue;
}

// adds buttons to the toolbar
void 
TableTabInterfaceTabMgmt::AddToolButtons()
{
	INT			i=0,j, size;
	LONG		ret;
	HICON		hicon;
	TBBUTTON	tbb[30];

    wyInt32 command[] = {   IDI_ADDROW,
						    IDI_DELETEROW,
						    IDM_SEPARATOR,
						    IDI_MOVEUP,
						    IDI_MOVEDOWN 
						};

	wyUInt32 states[][2] = {
							{TBSTATE_ENABLED, TBSTYLE_BUTTON},
							{TBSTATE_ENABLED, TBSTYLE_BUTTON},
							{TBSTATE_ENABLED, TBSTYLE_SEP},
							{TBSTATE_ENABLED, TBSTYLE_BUTTON},
                            {TBSTATE_ENABLED, TBSTYLE_BUTTON}
						   };

	wyInt32 imgres[] = {
						IDI_ADDROW, 
						IDI_DELETEROW,
						IDI_USERS,
						IDI_MOVEUP,
						IDI_MOVEDOWN
					  };

	VERIFY(m_himglist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32  | ILC_MASK, 1, 0));

 	SendMessage(m_hwndtool, TB_SETIMAGELIST, 0, (LPARAM)m_himglist);
	SendMessage(m_hwndtool, TB_SETEXTENDEDSTYLE, 0 , (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
    SendMessage(m_hwndtool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	size = sizeof(command)/sizeof(command[0]);

	// set some required values
	SendMessage(m_hwndtool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	// now create everything for the toolbar.
	for(j=0; j < size; j++)	
	{
		hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE ( imgres[j] ), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		VERIFY((i = ImageList_AddIcon(m_himglist, hicon))!= -1);
		VERIFY(DestroyIcon(hicon));
		
		memset(&tbb[j], 0, sizeof(TBBUTTON));

		tbb[j].iBitmap = MAKELONG(i, 0);
		tbb[j].idCommand = command[j];
		tbb[j].fsState = (UCHAR)states[j][0];
		tbb[j].fsStyle = (UCHAR)states[j][1];
	}  

	VERIFY((ret = SendMessage(m_hwndtool, TB_ADDBUTTONS, (WPARAM)size,(LPARAM) &tbb)!= FALSE));

    /* Now set and show the toolbar */
	SendMessage(m_hwndtool, TB_AUTOSIZE, 0, 0);
	ShowWindow(m_hwndtool, SW_SHOW);
}

wyInt32
TableTabInterfaceTabMgmt::GetActiveTabImage()
{
    CTCITEM ctci = {0};
    wyInt32		ncurselindex;
    
    if(!m_hwnd)
        return -1;

    ctci.m_mask = CTBIF_IMAGE;
    ncurselindex	=	CustomTab_GetCurSel(m_hwnd);
    VERIFY(CustomTab_GetItem(m_hwnd, ncurselindex, &ctci));
    return ctci.m_iimage;
}