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


#include <assert.h>

#include "TabMgmt.h"
#include "MDIWindow.h"
#include "Global.h"
#include "ExportMultiFormat.h"
#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "CustGrid.h"
#include "CustTab.h"
#include "GUIHelper.h"
#include "EditorFont.h"
#include "TabModule.h"
#include "TabEditorSplitter.h"
#include "TabQueryTypes.h"
#include "QueryAnalyzerBase.h"
#include "ResultView.h"
#include "TabResult.h"

#ifndef COMMUNITY
#include "HelperEnt.h"
#include"QueryAnalyzerEnt.h"
#else
#include"QueryAnalyzerCommunity.h"
#endif


extern	PGLOBALS		pGlobals;

#define     EQUALS          "="
#define     NOTEQUALS       "<>"
#define     GREATERTHAN     ">"
#define     LESSTHAN        "<"
#define     FIELDLIKEBEGIN  "1"
#define     FIELDLIKEEND    "2"
#define     FIELDLIKEBOTH   "3"

#define CXOFFSET 8     // defined pitch of trapezoid slant
#define CXMARGIN 2     // left/right text margin
#define CYMARGIN 1     // top/bottom text margin
#define CYBORDER 1     // top border thickness

#define	RESULTAB_SCROLL    3345

// Class to implement the tab control in the query window.

TabMgmt::TabMgmt(HWND hwndparent, MDIWindow* pmdi)
{
	m_hwndparent = hwndparent;
    m_hwnd = NULL;
    m_presultview = NULL;
	m_pcquerymessageedit = NULL;
	m_tabeditorptr = NULL;
    m_himl = NULL;
	m_pqa = NULL;	
    m_pcimdiwindow = pmdi;
    m_ptabletab = NULL;
    m_phistory = NULL;
    m_pqueryobj = NULL;
}

TabMgmt::~TabMgmt()
{
    SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);
    DeleteAllItem(wyTrue);
    ImageList_Destroy(m_himl);
	m_himl = NULL;
		
	if(m_hfont)
	{
		DeleteObject(m_hfont);
		m_hfont = NULL;
	}
	
    if(m_hwnd)
	{
		VERIFY(DestroyWindow(m_hwnd));
		m_hwnd = NULL;
	}
}

wyBool
TabMgmt::Create()
{
	CreateQueryTab(m_hwndparent);
    AddMessageTab();

    if(pGlobals->m_istabledataunderquery == wyTrue)
    {
        AddTableDataTab();
    }

    if(pGlobals->m_isinfotabunderquery == wyTrue)
    {
        AddInfoTab();
    }

    if(pGlobals->m_ishistoryunderquery == wyTrue)
    {
        AddHistoryTab();
    }
    
    CustomTab_SetClosable(m_hwnd, wyFalse);
	SetFont();		
	return wyTrue;
}

// Creates the tab window. Its parent window is passed as the parameter in the constructor.
HWND
TabMgmt::CreateQueryTab(HWND hwnd)
{
	HWND	        hwndtab;
    TABCOLORINFO    ci;

	hwndtab = CreateCustomTab(hwnd, 0, 0, 0, 0, WndProc, IDC_QUERYTAB);
    SendMessage(hwndtab, TBM_SETLONGDATA, 0, (LPARAM)this);

    if(wyTheme::GetTabColors(COLORS_RESTAB, &ci))
    {
        CustomTab_SetColorInfo(hwndtab, &ci);
    }

	m_hwnd	= hwndtab;
    CustomTab_SetIconList(m_hwnd, pGlobals->m_hiconlist);
	UpdateWindow(m_hwnd);

	return hwndtab;
}

// Wndproc for the tab control.
LRESULT
TabMgmt::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool *pishandled)
{
    TabMgmt*        pcquerytab = (TabMgmt*)CustomTab_GetLongData(hwnd);
    MDIWindow*      wnd;    
    TABCOLORINFO    ci;
    SCNotification* pscn;
    
    *pishandled = wyFalse;

    if(!pcquerytab)
    {
        return 0;
    }
	
    wnd = pcquerytab->m_pcimdiwindow;

	switch(message)
	{	
        case UM_STARTEXECUTION:
            CustomTab_SetClosable(wnd->m_pctabmodule->m_hwnd, wyTrue, CustomTab_GetItemCount(wnd->m_pctabmodule->m_hwnd));
            CustomTab_StartIconAnimation(hwnd, CustomTab_GetCurSel(hwnd));
            SendMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_UPDATEMAINTOOLBAR, WPARAM(0), LPARAM(0)); 
            EnableWindow(wnd->m_pcqueryobject->m_hwnd, FALSE);
            *pishandled = wyTrue;
            break;

        case UM_ENDEXECUTION:
            CustomTab_SetClosable(wnd->m_pctabmodule->m_hwnd, wyTrue, 1);
            SendMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_UPDATEMAINTOOLBAR, WPARAM(1), LPARAM(0)); 
            EnableWindow(wnd->m_pcqueryobject->m_hwnd, TRUE);
            CustomTab_StopIconAnimation(hwnd, CustomTab_GetCurSel(hwnd));
            SendMessage(hwnd, UM_SETROWCOUNT, wparam, 0);

            if(lparam)
            {
                SetFocus((HWND)lparam);
            }

            *pishandled = wyTrue;
            break;
            	    
	    case UM_MESSAGETABFOCUS:
            *pishandled = wyTrue;
		    SetFocus(pcquerytab->m_pcquerymessageedit->GetHwnd());
		    break;

        case WM_NOTIFY:
            if(((LPNMHDR)lparam)->code == NM_GETCOLORS)
            {
                CustomTab_GetColorInfo(hwnd, &ci);
                ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color1 = ci.m_tabtext;
                ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color2 = ci.m_tabbg1;
                *pishandled = wyTrue;
            }
            else if(((LPNMHDR)lparam)->code == NM_GETHYPERLINKCOLOR)
            {
                CustomTab_GetColorInfo(hwnd, &ci);

                if(ci.m_mask & CTCF_LINK)
                {
                    ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color1 = ci.m_linkcolor;
                }

                *pishandled = wyTrue;
            }
            else if(((LPNMHDR)lparam)->code == SCN_MODIFIED)
            {
                pscn = (SCNotification*)lparam;

                if(pcquerytab->m_phistory && 
                    pcquerytab->m_phistory->m_hwndedit == pscn->nmhdr.hwndFrom && 
                    (pscn->modificationType & SC_MOD_INSERTTEXT))
                {
                    SendMessage(pcquerytab->m_phistory->m_hwndedit, SCI_GOTOLINE, 
                        SendMessage(pcquerytab->m_phistory->m_hwndedit, SCI_GETLINECOUNT, 0, 0), 0);
                    *pishandled = wyTrue;
                }
            }

            break;

        case UM_SETSTATUSLINECOL:
            wnd->m_pcquerystatus->AddLineColNum((HWND)wparam, (wyBool)lparam);
            *pishandled = wyTrue;
            return 1;

        case UM_SETROWCOUNT:
            wnd->m_pcquerystatus->AddNumRows(wparam, lparam ? wyTrue : wyFalse);
            *pishandled = wyTrue;
            return 1;

        case WM_COMMAND:
            if(HIWORD(wparam) == CBN_SELENDOK && pcquerytab->GetActiveTabIcon() == IDI_QUERYMESSAGE)
            {
                ((TabMessage*)pcquerytab->GetActiveTabType())->OnWMCommand(wparam);
            }

            break;
	}

	return 0;
}

void
TabMgmt::SetTabEditorPtr(TabTypes* ptr)
{
    m_tabeditorptr = ptr;
}

void
TabMgmt::HideResultWindow()
{
}


// Sets the font for text in the tab window.
void
TabMgmt::SetFont()
{
	EditorFont::SetFont(m_hwnd, "DataFont", wyTrue);
	//SendMessage(m_hwnd, WM_SETFONT,(WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	return;
}

// Creates a new List Window and maintain its pointer.
void
TabMgmt::CreateResultView()
{
    m_presultview = new ResultView(m_pcimdiwindow, m_hwnd);
    m_presultview->Create();
	return ;
}

// Function adds Record Tab at the specified index.
// The data passed is the TABRECORDPARAM structure which contains data about the resultset
// which has is used in various condition.
void
TabMgmt::AddRecordTab(wyInt32 tabindex, MySQLResultDataEx* pdata)
{
	wyString    result(_("Result"));
    TabResult*  pres;

    if(!m_presultview)
    {
        CreateResultView();
    }

    pres = new TabResult(m_pcimdiwindow, m_hwnd, m_presultview);
    pres->m_data = pdata;

    if(tabindex < 0)
    {
        tabindex = CustomTab_GetItemCount(m_hwnd);
    }
	
    InsertTab(m_hwnd, tabindex, IDI_QUERYRECORD, result, (LPARAM)pres);
}

// Function adds Message Tab at the specified index.
void
TabMgmt::AddMessageTab()
{
    wyString message(_("Messages"));
	
	m_pcquerymessageedit = new TabMessage(m_pcimdiwindow, m_hwnd);
    m_pcquerymessageedit->Create();
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_QUERYMESSAGE, message, (LPARAM)m_pcquerymessageedit);
}

void
TabMgmt::AddHistoryTab()
{
    wyString message(_("History"));
	
    m_phistory = new TabQueryHistory(m_pcimdiwindow, m_hwnd);
    m_phistory->m_referencedptr = &m_phistory;
    m_phistory->Create();
    InsertTab(m_hwnd, CustomTab_GetItemCount(m_hwnd), IDI_HISTORY, message, (LPARAM)m_phistory);
}

void
TabMgmt::AddInfoTab()
{
    wyString    message(_("Info"));
    wyInt32     i, count, cursel;
    CTCITEM     ctcitem = {0};
	
    m_pqueryobj = new TabQueryObject(m_pcimdiwindow, m_hwnd);
    m_pqueryobj->m_preferencedptr = &m_pqueryobj;
    m_pqueryobj->Create();
    ctcitem.m_mask = CTBIF_IMAGE;
    count = CustomTab_GetItemCount(m_hwnd);

    for(i = count - 1; i >= 0; --i)
    {
        CustomTab_GetItem(m_hwnd, i, &ctcitem);

        if(ctcitem.m_iimage == IDI_QUERYMESSAGE || ctcitem.m_iimage == IDI_TABLE)
        {
            count = i + 1;
            break;
        }
    }

    InsertTab(m_hwnd, count, IDI_TABLEINDEX, message, (LPARAM)m_pqueryobj);
    cursel = CustomTab_GetCurSel(m_hwnd);
    CustomTab_EnsureVisible(m_hwnd, cursel, wyFalse);    
}

void
TabMgmt::AddTableDataTab(MySQLTableDataEx* data)
{
    wyString    message(_("Table Data"));
    wyInt32     i, count, cursel;
    CTCITEM     ctcitem = {0};
	
    m_ptabletab = new TabTable(m_pcimdiwindow, m_hwnd, data);
    m_ptabletab->m_preferencedptr = &m_ptabletab;
    m_ptabletab->Create();
    ctcitem.m_mask = CTBIF_IMAGE;
    count = CustomTab_GetItemCount(m_hwnd);
    
    for(i = count - 1; i >= 0; --i)
    {
        CustomTab_GetItem(m_hwnd, i, &ctcitem);

        if(ctcitem.m_iimage == IDI_QUERYMESSAGE)
        {
            count = i + 1;
            break;
        }
    }

    InsertTab(m_hwnd, count, IDI_TABLE, message, (LPARAM)m_ptabletab);
    cursel = CustomTab_GetCurSel(m_hwnd);
    CustomTab_EnsureVisible(m_hwnd, cursel, wyFalse);
}

//Resizes the tab control. It then calls the resize member function of each of the control in querywindow.
void
TabMgmt::Resize()
{
	RECT    rcparent, rcsplitter;
	wyInt32	hpos, vpos, width, height;

    GetClientRect(m_hwndparent, &rcparent);
    GetWindowRect(m_tabeditorptr->m_pcetsplitter->m_hwnd, &rcsplitter);
    MapWindowPoints(NULL, m_hwndparent, (LPPOINT)&rcsplitter, 2);

    hpos = 0;
	vpos = rcsplitter.bottom;
    width = rcparent.right;
    height = rcparent.bottom - vpos;
    MoveWindow(m_hwnd, hpos, vpos, width, height, TRUE);
	
	//Resizes the Analyzer tab
	if(m_pqa)
    {
		m_pqa->Resize(); 
    }

	if(m_pcquerymessageedit)
    {
	    m_pcquerymessageedit->Resize();
    }

    if(m_presultview)
    {
        m_presultview->Resize();
    }

    if(m_ptabletab)
    {
        m_ptabletab->Resize();
    }

    if(m_phistory)
    {
        m_phistory->Resize();
    }

    if(m_pqueryobj)
    {
        m_pqueryobj->Resize();
    }
}

// Function deletes all the tab item in the tab control.
// If the tab contains data about the result we free them and also and deallocate all the
// structures.
void
TabMgmt::DeleteAllItem(wyBool isdeletefixedtabs)
{
	wyInt32         tabcount, itemcount, index = 0;
	CTCITEM         ctci;
    TabQueryTypes*  ptr;
    	
    ctci.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
    itemcount = CustomTab_GetItemCount(m_hwnd);
	
	for(tabcount = 0; tabcount < itemcount; tabcount++)
	{
        CustomTab_GetItem(m_hwnd, index, &ctci);

        if((ctci.m_iimage == IDI_QUERYMESSAGE || ctci.m_iimage == IDI_TABLE || ctci.m_iimage == IDI_HISTORY || ctci.m_iimage == IDI_TABLEINDEX) && 
            isdeletefixedtabs == wyFalse)
        {
            index++;
            continue;
        }
        
        ptr = (TabQueryTypes*)ctci.m_lparam;
        CustomTab_DeleteItem(m_hwnd, 0,wyTrue);
        delete ptr;
	}

    delete m_presultview;
    m_presultview = NULL;
    m_pqa = NULL;

	return;
}

void 
TabMgmt::DeleteTab(wyInt32 index, wyBool ispostion)
{
    CTCITEM         ctci;
    TabQueryTypes*  ptr;
    wyInt32         i;

    i = CustomTab_GetCurSel(m_hwnd);
    ctci.m_mask = CTBIF_LPARAM;
    CustomTab_GetItem(m_hwnd, index, &ctci);
    ptr = (TabQueryTypes*)ctci.m_lparam;
    CustomTab_DeleteItem(m_hwnd, index, ispostion);
    delete ptr;

    if(index <= i && CustomTab_GetItemCount(m_hwnd))
    {
        ctci.m_mask = CTBIF_IMAGE;
        CustomTab_GetItem(m_hwnd, i, &ctci);
        
        if(ctci.m_iimage != IDI_QUERYMESSAGE && ctci.m_iimage != IDI_HISTORY)
        {
            i = SelectFixedTab(IDI_QUERYMESSAGE, wyTrue);
        }
        
        SelectTab(i);
    }
}

HWND
TabMgmt::GetHwnd()
{
	return m_hwnd;
}

// Create imagelist of images needed in the tab control.
void
TabMgmt::CreateTabImageList()
{
	wyInt32     imagecount;
	wyInt32     imgres[] = { IDI_QUERYRECORD, IDI_QUERYMESSAGE, IDI_QUERYMESSAGE, IDI_HISTORY, IDI_VIEWDATA};
	HICON       hicon;
	VERIFY(m_himl = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR32, 1, 0));

	for(imagecount = 0; imagecount <(sizeof(imgres)/sizeof(imgres[0])); imagecount++)
	{
		hicon =(HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(imgres[imagecount]), IMAGE_ICON, 16,16,LR_DEFAULTCOLOR);
		VERIFY(ImageList_AddIcon(m_himl, hicon)!= -1);
		DestroyIcon(hicon);
	}

	return;
}

// Function to do some processing when the tab is changed in the window.
// Basically it is when we show or hide messageedit box or listview window and stuff like that.
void
TabMgmt::OnTabSelChange(LPNMCTC lpnmctc)
{
	wyInt32			index, tabcount, i;
	CTCITEM			ctci;
	TabQueryTypes   *ptr, *prevptr;
	
    index = CustomTab_GetCurSel(m_hwnd);

    if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

    if(!lpnmctc)
    {
        ctci.m_mask =  CTBIF_LPARAM;
        CustomTab_GetItem(m_hwnd, index, &ctci);
        ptr = (TabQueryTypes*)ctci.m_lparam;
        ptr->UpdateStatusBar(m_pcimdiwindow->m_pcquerystatus);
    }
    else
    {
        tabcount = CustomTab_GetItemCount(m_hwnd);
            
        for(i = 0; i < tabcount; ++i)
        {
            if(i != index)
            {
                ctci.m_mask =  CTBIF_LPARAM;
                CustomTab_GetItem(lpnmctc->hdr.hwndFrom, i, &ctci);
                prevptr = (TabQueryTypes*)ctci.m_lparam;
                prevptr->OnTabSelChange(wyFalse);
            }
        }

        ctci.m_mask = CTBIF_LPARAM;
        CustomTab_GetItem(m_hwnd, index, &ctci);
        ptr = (TabQueryTypes*)ctci.m_lparam;
        ptr->OnTabSelChange(wyTrue);
    }
}


void
TabMgmt::SelectTab(wyInt32 index)
{
    CustomTab_SetCurSel(m_hwnd, index);
	CustomTab_EnsureVisible(m_hwnd, index);
	return;
}

DataView*
TabMgmt::GetDataView(wyInt32 index)
{
    CTCITEM     ctci;
    DataView*   pview = NULL;

    if(index >= CustomTab_GetItemCount(m_hwnd))
    {
        return NULL;
    }

	if(index < 0)
    {
        index = CustomTab_GetCurSel(m_hwnd);
    }

    ctci.m_mask = CTBIF_LPARAM | CTBIF_IMAGE;
	CustomTab_GetItem(m_hwnd, index, &ctci);
    pview = ctci.m_iimage == IDI_QUERYRECORD ? m_presultview : 
        (DataView*)((ctci.m_iimage == IDI_TABLE && m_ptabletab) ? m_ptabletab->m_ptableview : NULL);

    return pview;
}

// Function returns the record data about a particular tab whose index is passed as parameter.
MySQLDataEx*
TabMgmt::GetResultData(wyInt32 index)
{
	DataView* pview;

    pview = GetDataView(index);
    return pview ? pview->GetData() : NULL;
}

// Function returns the current selected image index
// Function returns the record data about a particular tab whose index is passed as parameter.
wyInt32
TabMgmt::GetSelectedItem()
{
	CTCITEM ctci = {0};

	ctci.m_mask	= CTBIF_IMAGE;
    CustomTab_GetItem(m_hwnd, CustomTab_GetCurSel(m_hwnd), &ctci);
	return ctci.m_iimage;
}


// Function to change the number in all the tab items when a tab is deleted or inserted.
void
TabMgmt::ChangeTitles()
{
	wyUInt32        tabcount, itemcount;
    wyString        newtitle;
    CTCITEM         ctci = {0};
    const wyChar*   ptr;

    itemcount = CustomTab_GetItemCount(m_hwnd);

	for(tabcount = 0; tabcount < itemcount; tabcount++)
	{
        ptr = "";
        ctci.m_mask = CTBIF_IMAGE;
        CustomTab_GetItem(m_hwnd, tabcount, &ctci);

        switch(ctci.m_iimage)
		{
		    case IDI_QUERYRECORD:
                ptr = _("Result");
			    break;

		    case IDI_QUERYMESSAGE:
                ptr = _("Messages");
			    break;

		    case IDI_QAALERT:
                ptr = _("Profiler");
			    break;

            case IDI_TABLE:
                ptr = _("Table Data");
                break;

            case IDI_HISTORY:
                ptr = _("History");
                break;

            case IDI_TABLEINDEX:
                ptr = _("Info");
                break;
		}

        newtitle.Sprintf("%s%u %s", tabcount < 8 ? "&" : "", tabcount + 1, ptr);
		
        //set it to the tab.
		ctci.m_mask = CTBIF_TEXT | CTBIF_IMAGE;
		ctci.m_psztext = (wyChar*)newtitle.GetString();
        ctci.m_cchtextmax = newtitle.GetLength();
        CustomTab_SetItem(m_hwnd, tabcount, &ctci);
	}
}

void
TabMgmt::AddAnalyzerTab(wyInt32 index , MDIWindow *wnd)
{
	wyBool istunnel = wyFalse;

	if(m_pqa)
	{
		delete(m_pqa);
		m_pqa = NULL;
	}

#ifndef COMMUNITY
	//Check whether the PQA option enabled in preference
	if(pGlobals->m_ispqaenabled == wyFalse || pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO ||
		pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
    {
		return;
    }

	if(wnd->m_tunnel->IsTunnel() == true)
		istunnel = wyTrue;
	m_pqa = new QueryAnalyzer(m_hwnd, wnd);
#else
	if(!wnd)
		wnd = GetActiveWin();

	if(!wnd)
		return;
	
	m_pqa = new QueryAnalyzerCommunity(m_hwnd, wnd);
#endif
	
	m_pqa->CreateControls(" ", 1, istunnel, wyTrue, wyFalse);
}


wyInt32
TabMgmt::AddExplainQueryAnalyzer(MySQLResultDataEx* pdata, wyBool isselectquery, wyBool istunnel, wyInt32 index, wyBool isExtended)
{

    #ifndef COMMUNITY
	wyString    textquery, resultexplain;
		
	//For ssh QA should work
	if(istunnel == wyTrue)
	{
		if(m_pcimdiwindow && m_pcimdiwindow->m_conninfo.m_isssh == wyTrue)
        {
			istunnel = wyFalse;
        }
	}
	
    textquery.SetAs(pdata->GetQuery());
	
    m_pqa = new QueryAnalyzer(m_hwnd, m_pcimdiwindow);

    if(!textquery.GetString())
    {
		delete m_pqa;
		m_pqa = NULL;
		return -1;
    }
    
    m_pqa->m_querytext.SetAs(textquery);
    m_pqa->CreateMainWindow();

    ((QueryAnalyzer *)m_pqa)->CreateOtherControls((wyChar *)m_pqa->m_querytext.GetString());
    
    m_pqa->m_htmlformatstr.SetAs("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;\
						  charset=UTF-8\"/></head><body>");

	m_pqa->m_htmlformatstr.Add("<style type=\"text/css\">");
	
	m_pqa->m_htmlformatstr.Add(CSS_CLASSES);
	m_pqa->m_htmlformatstr.Add(CSS_CLASS2);
	

	m_pqa->m_htmlformatstr.Add(CSS_WARNING);

	m_pqa->m_htmlformatstr.Add("</style>");	
	

    m_pqa->m_htmlformatstr.Add(((QueryAnalyzer *)m_pqa)->m_statushtml.GetString());
    
    ((QueryAnalyzer *)m_pqa)->ConvertMySQLResultToHtml(pdata->m_datares, &resultexplain, isExtended ? 5 : 1, wyFalse); 
	m_pqa->m_htmlformatstr.Add(resultexplain.GetString());
	
    if(m_pqa->CreateHtmlEditor(&m_pqa->m_htmlformatstr) == wyFalse)
    {
		delete m_pqa;
		m_pqa = NULL;
		return -1;
    }
		
	m_pqa->CreateAnalyzerTab(index);
    
#endif
	return 1;
}

//Creates the Query Analyzer tab
wyInt32
TabMgmt::AddQueryAnalyzer(MySQLResultDataEx* pdata, wyBool isselectquery, wyBool istunnel, wyInt32 index)
{	
#ifndef COMMUNITY
	wyBool		islimit = wyFalse;
    wyString    textquery;
    

	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO ||
		pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
    {
	    return 1;
    }

	//Procedd only if query is SELECT or if from version 5.6 Explain is avaliable for INSERT, DELETE, UPDATE and REPLACE
    if(isselectquery == wyFalse && m_pcimdiwindow->m_tunnel->IsTunnel() == wyFalse)
    {
        if(!pdata->m_isExplain)
		return 0;
    }
			
	if(pGlobals->m_ispqaenabled == wyFalse)
    {
		return 1;
    }
			
	if(m_pqa)
	{
		delete m_pqa;
		m_pqa = NULL;
	}

	//For ssh QA should work
	if(istunnel == wyTrue)
	{
		if(m_pcimdiwindow && m_pcimdiwindow->m_conninfo.m_isssh == wyTrue)
        {
			istunnel = wyFalse;
        }
	}
	
    textquery.SetAs(pdata->GetQuery());

    if(pdata->m_islimit == wyTrue)
	{
        textquery.AddSprintf(" LIMIT %d, %d", pdata->m_startrow, pdata->m_limit);
		islimit = wyTrue;
	}
		
    m_pqa = new QueryAnalyzer(m_hwnd, m_pcimdiwindow);

	if(m_pqa->CreateControls((wyChar*)textquery.GetString(), index, istunnel, wyFalse, islimit) == wyFalse)
	{
		delete m_pqa;
		m_pqa = NULL;
		return -1;
	}
	
#endif
	return 1;
}

TabQueryTypes*   
TabMgmt::GetActiveTabType()
{
    CTCITEM item = {0};

    item.m_mask = CTBIF_LPARAM;
    CustomTab_GetItem(m_hwnd, CustomTab_GetCurSel(m_hwnd), &item);

    return (TabQueryTypes*)item.m_lparam;

}

wyInt32   
TabMgmt::GetActiveTabIcon()
{
    CTCITEM item = {0};

    item.m_mask = CTBIF_IMAGE;
    CustomTab_GetItem(m_hwnd, CustomTab_GetCurSel(m_hwnd), &item);

    return item.m_iimage;

}

wyInt32
TabMgmt::SelectFixedTab(wyInt32 image, wyBool isonlyprob)
{
    wyInt32 i;
    CTCITEM item = {0};

    i = CustomTab_GetItemCount(m_hwnd) - 1;

    for(i = CustomTab_GetItemCount(m_hwnd) - 1; i >= 0; --i)
    {
        item.m_mask = CTBIF_IMAGE;
        CustomTab_GetItem(m_hwnd, i, &item);

        if(item.m_iimage == image)
        {
            if(isonlyprob == wyFalse)
            {
                SelectTab(i);
            }

            return i;
        }
    }

    return -1;
}

void 
TabMgmt::AddFixedTab(wyInt32 image)
{
    if(GetActiveTabIcon() != IDI_QUERYMESSAGE)
    {
        SelectFixedTab(IDI_QUERYMESSAGE);
    }

    switch(image)
    {
        case IDI_TABLE:
            AddTableDataTab();
            break;

        case IDI_TABLEINDEX:
            AddInfoTab();
            break;

        case IDI_HISTORY:
            AddHistoryTab();
            break;

        case IDI_QUERYMESSAGE:
            AddMessageTab();
            break;
    }

    ChangeTitles();
}