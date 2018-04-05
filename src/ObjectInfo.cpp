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

#include "ObjectInfo.h"
#include "EditorFont.h"
#include "Htmlrender.h"

#define TABLEINFO_X             2
#define TABLEINFO_Y             30
#define TABLEINFO_HEIGHT_ADJ    33
#define TABLEINFO_WIDTH_ADJ     7
#define SPACE_30                30
#define IDM_OBJECT_COPY		13
#define ONE_KB				1024

#define CSS_INFOTAB_CLASSES ".resultcaptionstyle	{font: 14px  \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left;}\
.colcaptionstyleleft {font: bold 12px \"Courier New\", Courier, mono; background:#4caaf9; color: #FFFFFF;text-align:left; padding-left:2px; padding-right:1px;border-spacing:0px;}\
.cellstyleleft{text-align:left;padding-left:1px; padding-right:1px;}\
.cellstyleright{text-align:right;padding-right:1px; padding-left:1px;}\
.captionfontstyle{font: bold 12px \"Courier New\", Courier, mono; text-align:right;}\
.datafontstylerowodd{font: 12px \"Courier New\", Courier, mono; text-align:right; background:#e5e5e5;height:23px;}\
.datafontstyleroweven{font: 12px \"Courier New\", Courier, mono; text-align:right; background:#FFFFFF;height:18px;}\
.statustablestyle{border: none; solid #EEE1FF;height:auto;word-wrap:break-word;table-layout:fixed;}\
.pkcolcaptionstyle{background:#4caaf9; color: #FFFFFF;}\
.imgcaptionstyle{vertical-align: middle;}\
.text1 {font: bold 16px \"Trebuchet MS\", Verdana, Arial, Helvetica;}\
.optimizecolstyle{font: 12px \"Courier New\", Courier, mono; text-align:left; background:#D6E7FF;height:18px; text-align:left; padding-left:10px; padding-right:10x;}\
.optimizedtablestyle{border:none solid #EEE1FF; word-wrap:break-word;table-layout:fixed;}\
.optimizedtable1style{word-wrap:break-all;table-layout:fixed;}\
.tablestyle{border: none; solid #EEE1FF;}\
.tabcaptionstyle{font: 14px \"Open Sans\", Verdana, Arial, Helvetica; text-align:left; color:black;}\
.buttonstyle{color:#333333; border-color:#a9b4bc;font: bold 12px \"Trebuchet MS\", Verdana, Arial, Helvetica,sans-serif;  background-color:  #efefee;}\
.warningstyle{font: 13px \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left; color:grey;}\
.redindexrowstyle{font: 12px \"Couthrier New\", Courier, mono; text-align:right; background:#FAC8A5; height:18px;}\
.redindexcolstyle{font: 12px \"Courier New\", Courier, mono; text-align:left; background:#FAC8A5; height:18px; text-align:left; padding-left:10px; padding-right:10x;}\
.captionstyle{font: 14px  \"Trebuchet MS\", Verdana, Arial, Helvetica; text-align:left;background-color:#FFFFFF; background-repeat:repeat-x;}\
.blueline { background-color: #4caaf9;height: 1px; }\
a:link { color: #3b7dbb; text-decoration:none;} a:visited { color: #3b7dbb; text-decoration:none;} a:hover {color: #4caaf9; text-decoration:none;} a:active { color: #4caaf9; text-decoration:none;}\
.whitespace{background-color: #FFFFFF;height: 5px; }"   

#define SCHEMA_OPTBUTTON "<input id=\"schemaoptbutton\" type=\"button\" value=\"%s\" class=\"buttonstyle\"></input>"
#define LINK_SCHEMA_OPTIMIZE _("Calculate Optimal Datatypes")
#define LINK_STOP_SCHEMA_OPTIMIZE _("Stop Calculation")
#define LINK_HIDE_SCHEMA_OPTIMIZE _("Hide Optimal Datatypes")
#define OPTIMZER_WARNING "The schema optimization is as per the present data in the table. \
						  Neither the server nor SQLyog can know what data may be inserted to the table in the future. \
						  The server admin or the application developer will have to take the decision before using the optimized schema."

#define OPTIMIZER_HELP _("Find the optimal datatypes for this table by reading existing data. <a href=\"optimizer_help \" target=\"_blank\"> Read more</a>")

#define REDUNDANT_INDEX_FINDBUTTON "<input type=\"button\" value=\"%s\" class=\"buttonstyle\"></input>"
#define LINK_REDUNDANT_INDEX_FIND _("Find Redundant Indexes")
#define LINK_REDUNDANT_INDEX_HIDE _("Hide Redundant Indexes Info")
#define REDUNDANT_INDEX_TABLE_HELP _("Find the redundant indexes of this table. <a href=\"redundantindexes_help \" target=\"_blank\"> Read more</a>")
#define REDUNDANT_INDEX_DB_HELP _("Find the redundant indexes of each table in the database. <a href=\"redundantindexes_help \" target=\"_blank\"> Read more</a>")

ObjectInfo::ObjectInfo(MDIWindow* pmdi, HWND hwndparent)
{
    m_hwndparent = hwndparent;
    m_wnd = pmdi;

    m_origtoolproc              = NULL;
	m_lastselnode				= (OBJECT)-1;
	m_hwndhtmleditor			= NULL;
	m_hwndtoolbar				= NULL;
	m_himglist					= NULL;
	m_hwndhtmleditor			= NULL;	
	m_hwndrefreshinfo			= NULL;
	m_hwndoptstatic				= NULL;
	m_hwndopthtml				= NULL;
	m_hwndopttext				= NULL;
	m_isinfotabalredyselected	= wyFalse;
	m_istableanalyse			= wyFalse;
	m_myrestableanalyse			= NULL;
	m_istostopcaption			= wyFalse;
	m_isoptimizationaborted		= wyFalse;
	m_pagewidth					= 0;
	m_isschemaoptimized			= wyFalse;
	m_ishiddenoptimization		= wyFalse;
	m_istohideoptimizecolumn    = wyFalse;

	m_myrescolumninfo		    = NULL;
	m_myrestableindexinfo	    = NULL;
	m_myrestableddlinfo		    = NULL;
    m_showredundantindex        = wyFalse;
    m_calledfromredundantindex  = wyFalse;
    m_tableinfosres             = NULL;
    m_viewinfores               = NULL;
    m_procinfores               = NULL;
    m_funcinfores               = NULL;
    m_triginfores               = NULL;
    m_eventinfores              = NULL;
    m_dbddlinfores              = NULL;
	m_isMDIclose = wyFalse;
	m_findreplace = NULL;
    m_origframeproc = NULL;

#ifndef COMMUNITY
    m_schemaoptimize = NULL;
    m_redindexfinder = NULL;

	if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
    {
		m_schemaoptimize = new SchemaOptimizer(pmdi , this);
        m_redindexfinder = new RedundantIndexFinder(pmdi, this);
    }
#endif

}

ObjectInfo::~ObjectInfo()
{
    if(m_hwndframe)
    {
        DestroyWindow(m_hwndframe);
    }

	FreeTableLevelResultsets();
    FreeDBLevelResultsets();	

#ifndef COMMUNITY	
	delete m_schemaoptimize;
    delete m_redindexfinder;
#endif
}

void
ObjectInfo::Create()
{
    m_hwndframe = CreateWindowEx(WS_EX_CONTROLPARENT, 
								 INSERT_UPDATE_WINDOW_CLASS_NAME_STR, 
                                 L"", WS_CHILD, 0, 0, 0, 0, 	
								 m_hwndparent, 
								 (HMENU)20, 
								 pGlobals->m_entinst, NULL);

	SetWindowLongPtr(m_hwndframe, GWLP_USERDATA, (LONG_PTR)this);
    m_origframeproc = (WNDPROC)SetWindowLongPtr(m_hwndframe, GWLP_WNDPROC, (LONG_PTR)ObjectInfo::FrameProc);

    ShowWindow(m_hwndframe, SW_HIDE);
    
	CreateEditWindow();
    CreateHTMLControl();
	CreateToolBar();
    InitView();
}

void 
ObjectInfo::CreateEditWindow()
{
	m_hwnd = CreateWindowEx(0, 
                            L"Scintilla", 
                            L"Source", 
                            WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 
                            0, 0, 0, 0,
                            m_hwndframe,
                            (HMENU)IDC_QUERYEDIT, 
                            pGlobals->m_pcmainwin->GetHinstance(), 
                            this);

    m_wporigproc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,(LONG_PTR)ObjectInfo::EditProc);	
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA,(LONG_PTR)this);

    SendMessage(m_hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
	SendMessage(m_hwnd, SCI_SETREADONLY, true, 0);
	SendMessage(m_hwnd, SCI_SETMARGINWIDTHN, 1, 0);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTHTRACKING, TRUE, 0);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTH, 10, 0);
	SendMessage(m_hwnd, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
	SetColor();
    SetFont();
}

void
ObjectInfo::SetFont()
{
    EditorFont::SetFont(m_hwnd, "HistoryFont", wyTrue);
}

void
ObjectInfo::SetColor()
{
    wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};
    COLORREF	color, backcolor;
	
	//Get the complete path.
	if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        wyString	dirstr(directory);

        backcolor   =   wyIni::IniGetInt(GENERALPREFA, "MTISelectionColor",   DEF_TEXTSELECTION, dirstr.GetString());
        SendMessage(m_hwnd,SCI_SETSELBACK,1,backcolor);
        
        backcolor=wyIni::IniGetInt(GENERALPREFA, "MTIBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString()); 
        SendMessage( m_hwnd, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)backcolor);
        
        SendMessage( m_hwnd, SCI_SETCARETFORE,backcolor ^ 0xFFFFFF,0); //Change Caret color in editor window

        color = wyIni::IniGetInt(GENERALPREFA, "MTIFgColor", DEF_NORMALCOLOR, dirstr.GetString()); 
        
        SendMessage(m_hwnd, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, color);
        SendMessage(m_hwnd, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, backcolor);
        SendMessage(m_hwnd, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);
    }
    else
    {
        EditorFont::SetColor(m_hwnd, wyTrue, wyTrue);
    }
    
    EditorFont::SetCase(m_hwnd);
}

void 
ObjectInfo::CreateHTMLControl()
{
	HINSTANCE	hinst = NULL;	

#ifdef COMMUNITY
	hinst = pGlobals->m_hinstance;
#else
	hinst = pGlobals->m_entinst;
#endif

    m_hwndhtmleditor = CreateWindowEx(0, 
                                      INFO_HTML_WINDOW, 
                                      L"Source", 
                                      WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
                                      0, 0, 0, 0,
                                      m_hwndframe, 
                                      (HMENU)NULL, 
                                      hinst, 
                                      this);

	SetWindowLongPtr(m_hwndhtmleditor, GWLP_USERDATA,(LONG_PTR)this);
    htmlayout::attach_event_handler(m_hwndhtmleditor, &DOMEventsHandler);
    HTMLayoutSetCallback(m_hwndhtmleditor, &HTMLayoutNotifyHandler, 0);    
}

LRESULT	CALLBACK
ObjectInfo::HtmlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ObjectInfo* ptabobject = (ObjectInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LRESULT     lResult;
	BOOL        bHandled;
    POINT		pt;

    lResult = HTMLayoutProcND(hwnd,message,wParam,lParam, &bHandled);
    
    if(bHandled)
    {
        return lResult;
    }

	switch(message)
	{		
	    case WM_HELP:
		    ShowHelp("http://sqlyogkb.webyog.com/article/84-query-profiler");
            break;	

        case UM_FOCUS:
            SetFocus(hwnd);
            break;

	    case WM_MOUSEMOVE:									
			if(ptabobject && ptabobject->m_istostopcaption == wyTrue)	
			{
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);

                //check whether the point is inside
                if(IsPointInsideHTMLElement(hwnd, L"schemaoptbutton", pt) == wyTrue)
				{					
					SetCursor(LoadCursor(NULL, IDC_ARROW));					
					ReleaseCapture();
				}
				else
				{					
					SetCursor(LoadCursor(NULL, IDC_WAIT));	
					SetCapture(hwnd);
				}			
            }			

		    break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void 
ObjectInfo::CreateToolBar()
{
	wyInt32 style = WS_CHILD | CCS_NORESIZE | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | WS_VISIBLE | CCS_NODIVIDER;
	
	m_hwndtoolbar = CreateWindowEx(0, 
        TOOLBARCLASSNAME, NULL, style, 
        0, 0, 0, 0, 
        m_hwndframe, 
        (HMENU)IDC_TOOLBAR,(HINSTANCE)pGlobals->m_hinstance, NULL);

    SetWindowLongPtr(m_hwndtoolbar, GWLP_USERDATA, (LONG_PTR)this);
    m_origtoolproc = (WNDPROC)SetWindowLongPtr(m_hwndtoolbar, GWLP_WNDPROC, (LONG_PTR)ObjectInfo::ToolbarWndProc);

	style = WS_CHILD | WS_VISIBLE;

	//For static text 'Format:'
	m_hwndoptstatic = CreateWindowEx(0, WC_STATIC, 
        _(L"Format: "), style  , 0,0, 0,0,	m_hwndtoolbar, 
        (HMENU)IDC_INFOFORMATTEXT, (HINSTANCE)pGlobals->m_hinstance, NULL);

	SendMessage(m_hwndoptstatic, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

	//HTML radio button
	m_hwndopthtml = CreateWindowEx(0, WC_BUTTON, TEXT("HTML"), 
        style | BS_AUTORADIOBUTTON, 0, 0, 0, 0, 
		m_hwndtoolbar, (HMENU)IDC_INFOHTMLMODE, 
		(HINSTANCE)pGlobals->m_hinstance, NULL);
    SendMessage(m_hwndopthtml, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

	//Text radio button
	m_hwndopttext = CreateWindowEx(0, WC_BUTTON, _(L"Text/Detailed"), 
			style | BS_AUTORADIOBUTTON, 0, 0, 0, 0,
			m_hwndtoolbar, (HMENU)IDC_INFOTEXTMODE, 
			(HINSTANCE)pGlobals->m_hinstance, NULL);
    SendMessage(m_hwndopttext, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
	
	//By default 'HTML option' is selected, 'Text option' is unchecked
	SendMessage(m_hwndopthtml, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	SendMessage(m_hwndopttext, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);	

	//Refresh Button
	m_hwndrefreshinfo = CreateWindowEx(0, WC_BUTTON, _(L"Refresh"), 
        style | WS_TABSTOP, 0, 0, 0, 0, 
        m_hwndtoolbar, (HMENU)IDC_REFRESHINFOTAB, 
        (HINSTANCE)pGlobals->m_hinstance, NULL);
	SendMessage(m_hwndrefreshinfo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
}

LRESULT	CALLBACK 
ObjectInfo::ToolbarWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ObjectInfo* pctableinfo = (ObjectInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HBRUSH      hbrush;
    DUALCOLOR   dc = {0};
    wyInt32     ret;

    switch(message)
    {
        case WM_CTLCOLORSTATIC:
            if(wyTheme::GetBrush(BRUSH_TOOLBAR, &hbrush))
            {
                SetBkMode((HDC)wparam, TRANSPARENT);

                if(wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc))
                {
                    if(IsWindowEnabled(hwnd))
                    {
                        if(dc.m_flag & 1)
                        {
                            SetTextColor((HDC)wparam, dc.m_color1);
                        }
                    }
                    else
                    {
                        if(dc.m_flag & 2)
                        {
                            SetTextColor((HDC)wparam, dc.m_color2);
                        }
                    }
                }

                return (LRESULT)hbrush;
            }

            break;

        case WM_NOTIFY:
            if(wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc) && 
                (ret = wyTheme::PaintButtonCaption(lparam, &dc)) != -1)
            {
                return ret;
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wparam))
			{
				case IDC_INFOTEXTMODE:
				case IDC_INFOHTMLMODE:
					if(pGlobals->m_pcmainwin->m_finddlg)
					{
						DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
						pGlobals->m_pcmainwin->m_finddlg = NULL;
					}
					
					pctableinfo->WriteSelectedFormatOption();
					pctableinfo->ShowOnTop();
					return 1;

				case IDC_REFRESHINFOTAB:
					pctableinfo->m_lastselnode = (OBJECT)-1;
					pctableinfo->Refresh(wyTrue);
					return 1;
			}
            break;
    }

    return CallWindowProc(pctableinfo->m_origtoolproc, hwnd, message, wparam, lparam);
}

void 
ObjectInfo::WriteSelectedFormatOption()
{
    wyInt32     ret; 
    wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0;
    wyString    dirstr;

    ret = SendMessage(m_hwndopthtml, BM_GETCHECK, (WPARAM)0, 0);
    SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    dirstr.SetAs(directory);
    wyIni::IniWriteInt(GENERALPREFA, "InfoTabFormatOption", ret, dirstr.GetString());
}

void
ObjectInfo::InitView()
{
    wyInt32     ret; 
    wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0;
    wyString    dirstr;

    ret = SendMessage(m_hwndopthtml, BM_GETCHECK, (WPARAM)0, 0);
    SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    dirstr.SetAs(directory);
    ret = wyIni::IniGetInt(GENERALPREFA, "InfoTabFormatOption", BST_CHECKED, dirstr.GetString());

    SendMessage(m_hwndopthtml, BM_SETCHECK, ret, 0);
    SendMessage(m_hwndopttext, BM_SETCHECK, !ret, 0);
    ShowOnTop();
}

void
ObjectInfo::Refresh(wyBool isforce)
{
    MDIWindow*      wnd = m_wnd;
    CQueryObject*   pqueryobject = wnd->m_pcqueryobject;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    switch(wnd->m_pcqueryobject->GetSelectionImage())
    {
	    case NSERVER:
		    ShowValues(wnd,"","",OBJECT_SERVER, isforce);
		    break;
		
	    case NTABLE:
		    pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));
		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(), pqueryobject->m_seltable.GetString(), OBJECT_TABLE, isforce);
            break;

        case NVIEWSITEM:
            pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, TreeView_GetParent(pqueryobject->m_hwnd, 
				                            TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd))));

		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(), pqueryobject->m_seltable.GetString(), OBJECT_VIEW, isforce);
            break;

        case NDATABASE:
		    pqueryobject->GetDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));
		    ShowValues(wnd, 
		    pqueryobject->m_seldatabase.GetString(), "", OBJECT_DATABASE, isforce);
            break;

        case NSPITEM:
		    pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
						    TreeView_GetParent(pqueryobject->m_hwnd, 
				            TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd))));
		    ShowValues(wnd, 
				    pqueryobject->m_seldatabase.GetString(), 
				    pqueryobject->m_seltable.GetString(), OBJECT_PROCEDURE, isforce);
            break;
 
        case NFUNCITEM:
		    pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));

		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
				            TreeView_GetParent(pqueryobject->m_hwnd, 
						    TreeView_GetSelection(pqueryobject->m_hwnd))));
		 
		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(),	
				    pqueryobject->m_seltable.GetString(), OBJECT_FUNCTION, isforce);
            break;
  
        case NTRIGGERITEM:
		    pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));
            pqueryobject->m_seltrigger.SetAs(pqueryobject->m_seltable.GetString());

		    pqueryobject->GetTableDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
				            TreeView_GetParent(pqueryobject->m_hwnd, 
						    TreeView_GetSelection(pqueryobject->m_hwnd))));
			
		    ShowValues(wnd,
				    pqueryobject->m_seldatabase.GetString(), 
				    pqueryobject->m_seltable.GetString(), OBJECT_TRIGGER, isforce, 
				    pqueryobject->m_seltrigger.GetString());
            break;

        case NEVENTITEM:
		    pqueryobject->GetTableDatabaseName(TreeView_GetSelection(pqueryobject->m_hwnd));

		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
				            TreeView_GetParent(pqueryobject->m_hwnd, 
						    TreeView_GetSelection(pqueryobject->m_hwnd))));
		 
		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(),	
				    pqueryobject->m_seltable.GetString(), OBJECT_EVENT, isforce);
            break;
	    
        case NTABLES :
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_TABLES, isforce);
		    break;
	    
        case NVIEWS :
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_VIEWS, isforce);
		    break;
	    
        case NSP :
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_PROCEDURES, isforce);
		    break;
	    
        case NFUNC:
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_FUNCTIONS, isforce);
		    break;
	    
        case NTRIGGER:
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_TRIGGERS, isforce);
		    break;
	    
        case NEVENTS:
		    pqueryobject->GetDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd,TreeView_GetSelection(pqueryobject->m_hwnd)));
		    ShowValues(wnd,pqueryobject->m_seldatabase.GetString(),"",OBJECT_EVENTS, isforce);
		    break;
		
	    case NFOLDER:
			    pqueryobject->GetTableDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
					    TreeView_GetSelection(pqueryobject->m_hwnd)));
			
		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(), pqueryobject->m_seltable.GetString(), OBJECT_TABLE, isforce);
            break;

	    case NPRIMARYKEY:
	    case NCOLUMN:
	    case NINDEX:
	    case NPRIMARYINDEX:
			    pqueryobject->GetTableDatabaseName(TreeView_GetParent(pqueryobject->m_hwnd, 
				        TreeView_GetParent(pqueryobject->m_hwnd, 
					    TreeView_GetSelection(pqueryobject->m_hwnd))));

		    ShowValues(wnd, pqueryobject->m_seldatabase.GetString(), pqueryobject->m_seltable.GetString(), OBJECT_TABLE, isforce);
            break;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

wyBool
ObjectInfo::ShowValues(MDIWindow* pcquerywnd, const wyChar* db, const wyChar* table, OBJECT obj, wyBool isforce, const wyChar* trigger)
{	
    wyBool      ret = wyFalse;
	wyString    st, columns;

	if(obj != OBJECT_TABLE || m_istohideoptimizecolumn == wyFalse)
	{
        if(isforce == wyFalse && !m_lastseldb.Compare(db) && 
            !m_lastselobj.Compare(table) && m_lastselnode == obj)
        {
			return wyFalse;
        }
		else
		{
			FreeTableLevelResultsets();
	        FreeDBLevelResultsets();
		    m_showredundantindex = wyFalse;
		}
	}

	SendMessage(m_hwnd, SCI_SETREADONLY, false, 0);
	
	m_lastseldb.SetAs(db); 
	m_lastselnode =(OBJECT)obj; 
	m_lastselobj.SetAs(table);

	if(obj == OBJECT_SERVER)
	{
		ret = ShowServerInfo(pcquerywnd);
	}
	else if(obj == OBJECT_DATABASE)
	{
		ret = ShowDBInfo(pcquerywnd, db);
	}
	else if(obj == OBJECT_TABLES)
	{
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)"");
		ret = ShowAllTables(pcquerywnd, db, obj);		
	}
	else if(obj == OBJECT_TABLE || obj == OBJECT_VIEW)
	{
		ret = ShowTableInfo(pcquerywnd, db, table, obj);
	}
	else if(obj == OBJECT_PROCEDURE)
	{
		ret = ShowProcedureInfo(pcquerywnd, db, table);
	}
	else if(obj == OBJECT_FUNCTION)
	{
		ret = ShowFunctionInfo(pcquerywnd, db, table);
	}
	else if(obj == OBJECT_TRIGGER)
	{
		ret = ShowTriggerInfo(pcquerywnd, db, table, trigger);
	}
	else if(obj == OBJECT_EVENT)
	{
		ret = ShowEventInfo(pcquerywnd, db, table);
	}
	else if(obj == OBJECT_VIEWS)
	{	
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""); 
		ret = ShowAllViews(pcquerywnd, db, OBJECT_VIEWS);
	}
	else if(obj == OBJECT_PROCEDURES)
	{
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)"");
		ret = ShowAllProcedures(pcquerywnd, db, OBJECT_PROCEDURES);
	}	
	else if(obj == OBJECT_FUNCTIONS)
	{
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)"");
		ret = ShowAllFunctions(pcquerywnd, db, OBJECT_FUNCTIONS);
	}
	else if(obj == OBJECT_TRIGGERS)
	{
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)"");
		ret = ShowAllTriggers(pcquerywnd, db, OBJECT_TRIGGERS);
	}
	else if(obj == OBJECT_EVENTS)
	{
		SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)"");
		ret = ShowAllEvents(pcquerywnd, db, OBJECT_EVENTS);
	}
    
	SendMessage(m_hwnd, SCI_SETSEL, -1, 0);
	SendMessage(m_hwnd, SCI_SETREADONLY, true, 0);

	return ret;
}

void
ObjectInfo::ShowOnTop()
{
	ShowWindow(m_hwndtoolbar, SW_SHOW);
	ShowWindow(m_hwndopthtml, SW_SHOW);
	ShowWindow(m_hwndoptstatic, SW_SHOW);
	ShowWindow(m_hwndopttext, SW_SHOW);
	ShowWindow(m_hwndrefreshinfo, SW_SHOW);

	if(SendMessage(m_hwndopthtml, BM_GETCHECK, 0, 0))
	{
		ShowWindow(m_hwnd, SW_HIDE);
		ShowWindow(m_hwndhtmleditor, SW_SHOW);
		SetFocus(m_hwndhtmleditor);
	}
	else
	{
        ShowWindow(m_hwndhtmleditor, SW_HIDE);
        ShowWindow(m_hwnd, SW_SHOW);
		SetFocus(m_hwnd);
	}
}




//----------------------------------------------------------



// Function to show various information about the table.
wyBool
ObjectInfo::ShowTableInfo(MDIWindow * pcquerywnd, const wyChar*db, const wyChar *table, OBJECT obj, wyBool istoanalyse)
{
	wyString    query, temptitle, columns;
	wyBool		ret = wyFalse;

	if(m_strtableinfo.GetLength())
	{
		m_strtableinfo.Clear();
	}

	if(m_htmlformatstr.GetLength())
	{
		m_htmlformatstr.Clear();
	}
	
	m_istableanalyse = istoanalyse;

	if(m_istableanalyse == wyTrue)		
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
	}

    if(m_istableanalyse == wyFalse && m_istostopcaption == wyFalse && 
       m_istohideoptimizecolumn == wyFalse && !pcquerywnd->m_stopquery && 
       m_calledfromredundantindex == wyFalse)
	{
		FreeTableLevelResultsets();
	}

	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));

	if(obj == OBJECT_TABLE)
	{
		temptitle.Sprintf(_("/*Table: %s*/\r\n"), table);
	}
	else
	{	
		temptitle.Sprintf(_("/*View: %s*/\r\n"), table);
	}
	
	AppendLine(temptitle);

	SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(), (LPARAM)temptitle.GetString());	


	if(m_istostopcaption == wyFalse)
	{
		// get information about the fields.
		query.Sprintf("show full fields from `%s`.`%s`", db, table);
			
		// create the first top title.
		temptitle.Sprintf(_("/*Column Information*/\r\n"));
			
		// now we put a line -- below only uptill necessary.
		AppendLine(temptitle);

		m_myrescolumninfo = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, 
                                    query, temptitle, wyFalse, m_myrescolumninfo);

		if(!m_myrescolumninfo)
			return wyFalse;				
	}
	
	//Column info in Html form
	ret = ShowTableColumnInfoHtmlFormat(pcquerywnd, m_myrescolumninfo, table, obj);
	if(m_istostopcaption == wyTrue)
		return wyTrue;

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	

	if(m_istableanalyse == wyTrue && m_optimizedschema.GetLength())
		AddTableOptimizeTableSchema();		
	
	m_istableanalyse = wyFalse;
	m_istostopcaption = wyFalse;
	m_isoptimizationaborted = wyFalse;

	//View doesn't have any index 
	if(obj == OBJECT_TABLE)
	{		
		temptitle.Sprintf(_("/*Index Information*/\r\n"));
		AppendLine(temptitle);
		
		query.Sprintf("show keys from `%s`.`%s`", db, table);
			
		m_myrestableindexinfo = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, 
                                        query, temptitle, wyFalse, m_myrestableindexinfo);
		if(!m_myrestableindexinfo)
			return wyFalse;
		
		//Index info in Html form
		ret = ShowTableIndexInfoHtmlFormat(pcquerywnd, m_myrestableindexinfo, table, OBJECT_INDEX);		
	}	
    
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	// no ret
  	// create the third top title.
	temptitle.Sprintf(_("/*DDL Information*/\r\n"));
	AppendLine(temptitle);
	
	query.Sprintf("show create table `%s`.`%s`", db, table);
	    
	//Add the MySQL columns in normal form
	m_myrestableddlinfo = AddMySQLColInfo(pcquerywnd, table, query, temptitle, obj, m_myrestableddlinfo);
		
	if(!m_myrestableddlinfo)
			return wyFalse;
	
	//Show Table/View DDL info in HTML format
	ShowTableDDLInfoHtmlFormat(pcquerywnd, m_myrestableddlinfo, table, obj);
	    
	return wyTrue;
}   

/*
Disply HTML formatted resulted o/p for Table's Column Information
'columns' defines the Fields to be shown.
*/
wyBool
ObjectInfo::ShowTableColumnInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj)
{
	wyString columns;
	wyBool ret = wyTrue;

	m_isschemaoptimized = wyFalse;
	
	if(myres)
	{
		pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

		if(pcquerywnd->m_ismysql41 == wyTrue)
		{
			columns.SetAs("Field,Type,Null,Comment,");
		}
		else
		{
			//Selected columns to disply
			columns.SetAs("Field,Type,Null");
	}
	}

	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, table, obj, wyFalse, wyFalse);

	return ret;
}

/*
Disply HTML formatted resulted o/p for Table's Index Information
Showing the All coulmns of MySQL result in Info tab
*/
wyBool
ObjectInfo::ShowTableIndexInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj)
{
	wyBool ret = wyTrue;

	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
  
	//Showing all coulmns to info tab, so we passed 'NULL'
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, NULL, table, obj);

	return ret;
}

wyBool
ObjectInfo::ShowTableDDLInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj)
{
	OBJECT		objtype;
	wyBool		ret = wyTrue;
	wyString	columns;

	//The DDL for 'Table' or 'View'	
	objtype = (obj == OBJECT_TABLE) ? OBJECT_TABLEDDL : OBJECT_VIEWDDL;

	//Set the column to display in info tab
	if(objtype == OBJECT_TABLEDDL)
	{
		columns.SetAs("Create Table");
	}
	else
	{
		columns.SetAs("Create View");
	}
		
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	//HTML formatted o/p	
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, table, (OBJECT)objtype);

	return ret;
}

void 
ObjectInfo::AppendLine(wyString &title)
{
    wyInt32 len = 0, count;

    len = title.GetLength() - 2;
    
    for(count = 0; count < len; count++)
    {
        title.Add("-");
	}

	title.Add("\r\n\r\n");
    return;
}

// Function to show various information about the procedure.

wyBool
ObjectInfo::ShowProcedureInfo(MDIWindow * pcquerywnd, const wyChar *db, const wyChar *procedure)
{
    wyString    query, temptitle, columns;
	MYSQL_RES	*myres = NULL;

	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));

	// get information about the fields.
	query.Sprintf("show create procedure `%s`.`%s`", db, procedure); 

	// create the first top title.
	temptitle.Sprintf(_("/*Stored Proc: %s*/\r\n"),procedure);
    AppendLine(temptitle);

	//Add the result in normal form
	myres = AddMySQLColInfo(pcquerywnd, procedure, query, temptitle, OBJECT_PROCEDURE);
		
	if(!myres)
	{
		return wyFalse;
	}
		
	//'Create procedure' statement in HTML format
	ShowProcedureInfoHtmlFormat(pcquerywnd, myres, procedure);

	if(myres)
	{
		pcquerywnd->m_tunnel->mysql_free_result(myres);
	}

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	// no ret

	return wyTrue;
}

/*
'Create Procedure' statement in HTML format
*/
wyBool
ObjectInfo::ShowProcedureInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *procedure)
{
	wyString columns;
	
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	
	//selected column
	columns.SetAs("Create Procedure");
	
	AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, procedure, OBJECT_PROCEDURE);

	return wyTrue;
}

// Function to show various information about the Function.
wyBool
ObjectInfo::ShowFunctionInfo(MDIWindow * pcquerywnd, const wyChar *db, const wyChar *function)
{
    wyString    query, temptitle, columns;
	MYSQL_RES	*myres = NULL;

	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));

	// get information about the fields.
	query.Sprintf("show create function `%s`.`%s`", db, function); 

	// create the first top title.
	temptitle.Sprintf(_("/*Function: %s*/\r\n"), function);
	AppendLine(temptitle);
	
	//Add the result in normal form
	myres = AddMySQLColInfo(pcquerywnd, function, query, temptitle, OBJECT_FUNCTION);
	
	if(!myres)
	{
		return wyFalse;
	}

	//Display Function info in HTML format
	ShowFunctionInfoHtmlFormat(pcquerywnd, myres, function);

	if(myres)
	{
		pcquerywnd->m_tunnel->mysql_free_result(myres);
	}

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	// no ret

	return wyTrue;
}

/*
'Create Function' statement in HTML format
*/
wyBool
ObjectInfo::ShowFunctionInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *function)
{
	wyString columns;

	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	columns.SetAs("Create Function");

	AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, function, OBJECT_FUNCTION);

	return wyTrue;
}

// Function to show various information about the Trigger.
wyBool
ObjectInfo::ShowTriggerInfo(MDIWindow * pcquerywnd, const wyChar *db, 
                            const wyChar * table, const wyChar *trigger)
{
	wyString    query, temptitle;
	MYSQL_RES	*myres = NULL;
	wyString	columns;

	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));

	// get information about the fields.
    //query.Sprintf("select `Trigger_name`, `Event_object_schema`, `Event_object_table`, `Event_manipulation`, `Action_orientation` ,`Action_timing`, `Action_reference_old_table`, `Action_reference_new_table`, `Action_reference_old_row`, `Action_reference_new_row`, `Created`, `Action_statement` from `INFORMATION_SCHEMA`.`TRIGGERS` where `EVENT_OBJECT_SCHEMA` = '%s' and `TRIGGER_NAME` = '%s'", 
		//					db, trigger);

	query.Sprintf("SHOW TRIGGERS FROM `%s` WHERE `Trigger`='%s'", db, trigger);
	
	// create the first top title.
	temptitle.Sprintf(_("/*Trigger: %s*/\r\n"), trigger);
	AppendLine(temptitle);
	
	//Add the MySQL coulms in normal form
	myres = AddMySQLColInfo(pcquerywnd, trigger, query, temptitle, OBJECT_TRIGGER);

	if(!myres)
	{
		return wyFalse;
	}
		
	//Trigger info in HTML format
	ShowTriggerInfoHtmlFormat(pcquerywnd, myres, trigger);

	if(myres)
	{
		pcquerywnd->m_tunnel->mysql_free_result(myres);
	}

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	// no ret

	return wyTrue;
}

/*
Trigger info in HTML format
*/
wyBool
ObjectInfo::ShowTriggerInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *trigger)
{
	wyString columns;

	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	//Selected columns
	columns.SetAs("Table,Event,Timing,Statement");

	AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, trigger, OBJECT_TRIGGER);

	return wyTrue;
}

// Function to show various information about the database.
wyBool
ObjectInfo::ShowDBInfo(MDIWindow * pcquerywnd, const wyChar *db)
{
    wyString    query, temptitle;
	
	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));	

	temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
	AppendLine(temptitle);

	VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	

    if(m_calledfromredundantindex == wyFalse)
    {
        FreeDBLevelResultsets();
	}

	// get information about the tables.	
	if(ShowAllTables(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
	{
		return wyFalse;
	}

	if(IsMySQL5(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql) == wyTrue)
	{	
		SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
		if(ShowAllViews(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
		{
			return wyFalse;
		}
        	
		SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
		
		if(ShowAllProcedures(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
		{
			return wyFalse;
		}
	
		SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
		
		if(ShowAllFunctions(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
		{
			return wyFalse;
		}
            
		SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
		
		if(ShowAllTriggers(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
		{
			return wyFalse;
		}

		if(IsMySQL516(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql) == wyTrue)
		{
			SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
			if(ShowAllEvents(pcquerywnd, db, OBJECT_DATABASE) == wyFalse)
			{
				return wyFalse;
		}
	}
	}
	
	if(pcquerywnd->m_ismysql41 == wyTrue)
	{
		SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	// no ret

		temptitle.Sprintf(_("/*DDL Information For - %s*/\r\n"), db);

		AppendLine(temptitle);

		// get information about the charset & collation.
		query.Sprintf("show create database `%s`", db);

		m_dbddlinfores = AddMySQLColInfo(pcquerywnd, db, query, temptitle, OBJECT_DATABASE, m_dbddlinfores);
		
		if(!m_dbddlinfores)
		{
			return wyFalse;
	}
	}

	return wyTrue;	
}

// Generic function to add the information whose query is passed as parameter.

MYSQL_RES*
ObjectInfo::AddInfo(Tunnel * tunnel, PMYSQL umysql, wyString &query , 
                    wyString &temptitle, wyBool createtable, MYSQL_RES *res)
{
	wyChar		*tempresult;
	MYSQL_RES	*myres = NULL;
	MYSQL_FIELD	*myfields;
	wyBool		ismysql41 = IsMySQL41(tunnel, umysql);
	wyString	result;
	DWORD		len;
    
    SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()); // no ret

    if(!res)
	{
		myres = ExecuteAndGetResult(m_wnd/*GetActiveWin()*/, tunnel, umysql, query);
		if(!myres)
		{
			ShowMySQLError(m_hwnd, tunnel, umysql, query.GetString());
			return NULL;
		}
	}

	else
	{
		myres = res;
		tunnel->mysql_data_seek(myres, 0);
	}

	VERIFY(myfields = tunnel->mysql_fetch_fields(myres));
	
	// Get the formatted output for the field things and add it to the edit box.
	if(createtable == wyTrue)
		VERIFY(tempresult = FormatCreateTableResultSet(tunnel, myres, myfields));
	else
		VERIFY(tempresult = FormatResultSet(tunnel, myres, myfields));

	if(ismysql41 == wyFalse)                     // conversion of accented characters to utf8
	{
		result.SetAs(tempresult, wyFalse);
	}
	else
	{
		result.SetAs(tempresult);
	}
		
	len = result.GetLength();
	SendMessage(m_hwnd, SCI_APPENDTEXT, len, (LPARAM)result.GetString());

	// free the buffer.
	HeapFree(GetProcessHeap(), NULL, tempresult);

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");// no ret

	return myres;
}

/*
Add the MySQL result columns to Info tab. 
Here we avoided the Table format vor avoiding the padding spaces in 'Create statements', etc while copying to other editor
*/
MYSQL_RES*
ObjectInfo::AddMySQLColInfo(MDIWindow *pcquerywnd, const wyChar *objname, wyString &query, wyString &temptitle, OBJECT obj, MYSQL_RES *myres)
{
	wyString	columns;
	//MYSQL_RES	*myres = NULL;
    
	if(!myres)
	{
		myres = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
		if(!myres)
		{
			ShowMySQLError(m_hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
			return NULL;
		}
	}

	else
	{
		pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	}


	//Selected columns/order of columns to add to Info tab
	switch(obj)
	{
	case OBJECT_DATABASE:
		columns.SetAs("Create Database");
		break;

	case OBJECT_TABLE:
			columns.SetAs("create table");
		break;

	case OBJECT_VIEW:
			columns.SetAs("create view");
		obj = OBJECT_VIEWDDL;
		break;

	case OBJECT_PROCEDURE:
		columns.SetAs("Create Procedure,sql_mode,");
		break;

	case OBJECT_FUNCTION:
		columns.SetAs("Create Function,sql_mode,");
		break;

	case OBJECT_TRIGGER:
		//columns.SetAs("Action_statement,Event_object_table,Event_manipulation,Action_orientation,Action_timing,Action_reference_old_table,Action_reference_new_table,Action_reference_old_row,Action_reference_new_row,Created,");
		columns.SetAs("Statement,Table,Event,Timing,Created,");
		break;
        
	case OBJECT_EVENT:
		columns.SetAs("Create Event,sql_mode,time_zone,character_set_client,collation_connection,Database Collation,");
		break;

	default:
		return myres;
	}
	
	//Adding the Coulmns to Info Tab
	FormatTextModeSQLStatement(pcquerywnd, objname, &myres, &temptitle, (wyChar*)columns.GetString(), obj);

    return myres;
}

wyBool
ObjectInfo::ShowServerInfo(MDIWindow * wnd)
{
	wyBool		ret = wyTrue;
	wyString	temptitle, result, query, imgcaption, htmlinbuff;
	MYSQL_RES	*myres = NULL;

	if(m_htmlformatstr.GetLength())
	{
		m_htmlformatstr.Clear();
	}
				
	//For Normal text output
	temptitle.Sprintf(_("/*Server Information For - "));
	temptitle.AddSprintf("%s@%s", wnd->m_conninfo.m_user.GetString(), wnd->m_conninfo.m_host.GetString());
	temptitle.AddSprintf("*/ \r\n");
	AppendLine(temptitle);
	
	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0, (LPARAM)""));
	VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));

	result.Sprintf("%-*s: %s\r\n", SPACE_30, "MySQL Version", wnd->m_mysql->server_version );
	result.AddSprintf("%-*s: %s\r\n", SPACE_30, "Host", wnd->m_conninfo.m_host.GetString());
	result.AddSprintf("%-*s: %s\r\n", SPACE_30, "User", wnd->m_conninfo.m_user.GetString());
	result.AddSprintf("%-*s: %d\r\n", SPACE_30, "Port", wnd->m_conninfo.m_port); 
	if(wnd->m_ismysql41 == wyTrue)
	{	
		GetServerCharset(wnd->m_conninfo.m_charset);
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "Server Default Charset", wnd->m_conninfo.m_charset.GetString());
	}

	if(wnd->m_conninfo.m_ishttp)
	{	
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "Url", wnd->m_conninfo.m_url.GetString());
		result.AddSprintf("%-*s: %d(ms)\r\n", SPACE_30, "Timeout", wnd->m_conninfo.m_timeout);
	}

	if(wnd->m_conninfo.m_isssh )
	{
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "SSH Host", wnd->m_conninfo.m_sshhost.GetString());
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "Username", wnd->m_conninfo.m_sshuser.GetString());
		result.AddSprintf("%-*s: %d\r\n", SPACE_30, "SSH Port", wnd->m_conninfo.m_sshport);
		result.AddSprintf("%-*s: %d\r\n", SPACE_30, "Local Port", wnd->m_conninfo.m_localport);
	}

	if(wnd->m_conninfo.m_issslchecked)
	{	
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "CACertificate", wnd->m_conninfo.m_cacert.GetString());
		result.AddSprintf("%-*s: %s\r\n", SPACE_30, "Cipher", wnd->m_conninfo.m_cipher.GetString());
		if(wnd->m_conninfo.m_issslauthchecked)
		{	
			result.AddSprintf("%-*s: %s\r\n", SPACE_30, "ClientCertificate", wnd->m_conninfo.m_clicert.GetString());
		    result.AddSprintf("%-*s: %s\r\n", SPACE_30, "ClientKey", wnd->m_conninfo.m_clikey.GetString());
		}
	}
	
	SendMessage(m_hwnd, SCI_APPENDTEXT, result.GetLength(),(LPARAM)result.GetString());

	//SHOW VARIABLES O/P
	query.SetAs("SHOW VARIABLES");
	temptitle.SetAs("\r\n/*SHOW VARIABLES Output*/\r\n");
	AppendLine(temptitle);
	myres = AddInfo(wnd->m_tunnel, &wnd->m_mysql, query, temptitle, wyFalse);

	if(!myres)
		return wyFalse;

	/*Initialize the HTML for Server details*/	
	//Setting the CSS styles
	m_htmlformatstr.SetAs("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;\
						charset=UTF-8\"/></head><body>");

	m_htmlformatstr.Add("<style type=\"text/css\">");
	m_htmlformatstr.Add(CSS_INFOTAB_CLASSES);
	m_htmlformatstr.Add("</style>");

	imgcaption.Sprintf("&nbsp;MySQL Server: %s", wnd->m_mysql->server_version);
	
	//For HTML Formatted output
	wnd->m_tunnel->mysql_data_seek(myres, 0);
	ret = ShowHtmlFormatResultOnServerObject(wnd, myres, wyTrue, &htmlinbuff);	

	if(myres)
	{
		wnd->m_tunnel->mysql_free_result(myres);
	}
	
	//SHOW STATUS O/P
	query.SetAs("SHOW STATUS");
	temptitle.SetAs("\r\n/*SHOW STATUS Output*/\r\n");
	AppendLine(temptitle);
	myres = AddInfo(wnd->m_tunnel, &wnd->m_mysql, query, temptitle, wyFalse);

	if(!myres)
	{
		return wyFalse;
	}

	//For HTML Formatted output
	wnd->m_tunnel->mysql_data_seek(myres, 0);
	ret = ShowHtmlFormatResultOnServerObject(wnd, myres, wyFalse, &htmlinbuff);

	if(myres)
	{
		wnd->m_tunnel->mysql_free_result(myres);
	}
	
	if(!m_serveruptime.GetLength())
	{
		m_serveruptime.SetAs("0");
	}

	m_htmlformatstr.AddSprintf("<div><img class=\"imgcaptionstyle\"src=\"res:SERVER.PNG\"><span class=\"text1\">%s, Running for: %s</span></div><br>", 
		imgcaption.GetString(), m_serveruptime.GetString());
	  m_htmlformatstr.Add("<div class=\"blueline\"></div>");
	m_htmlformatstr.Add(htmlinbuff.GetString());
	m_htmlformatstr.Add("<br/>&nbsp;<br/>&nbsp;</html>");
	
	//Passing HTML buffer to HTML control
	HTMLayoutLoadHtml(m_hwndhtmleditor, (PBYTE)m_htmlformatstr.GetString(), m_htmlformatstr.GetLength());
	
	//html page Selection mode
	HTMLayoutSetMode(m_hwndhtmleditor, HLM_SHOW_SELECTION);
			
	return wyTrue;
}

// Function to show various information about the Event.

wyBool
ObjectInfo::ShowEventInfo(MDIWindow * pcquerywnd, const wyChar *db, const wyChar *event)
{
    wyString    query, temptitle, columns;

	VERIFY(SendMessage(m_hwnd, SCI_SETTEXT, 0,(LPARAM)""));

	// get information about the fields.
	query.Sprintf("show create event `%s`.`%s`", db, event); 

	// create the first top title.
	temptitle.Sprintf(_("/*Event Information*/\r\n"));
	AppendLine(temptitle);

	//Add the MySQL coulms in normal form
    m_eventinfores = AddMySQLColInfo(pcquerywnd, event, query, temptitle, OBJECT_EVENT, m_eventinfores);

	if(!m_eventinfores)
	{
		return wyFalse;
	}

	//Event info in HTML format
	ShowEventInfoHtmlFormat(pcquerywnd, m_eventinfores, event);

	// Add a temporary line.
	SendMessage(m_hwnd, SCI_APPENDTEXT, 2,(LPARAM)"\r\n");	

	return wyTrue;
}

/*
Event info in HTML format
*/
wyBool
ObjectInfo::ShowEventInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *event)
{
	wyString columns;
    
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	//Selected columns
	columns.SetAs("Create Event");

	AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, event, OBJECT_EVENT);

	return wyTrue;
}

wyBool
ObjectInfo::ShowAllTables(MDIWindow * pcquerywnd, const wyChar *db, OBJECT obj)
{
	wyString    query, temptitle;
	wyString	columns;
	wyBool      ismysql5 = IsMySQL5(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql), ret = wyTrue;

    if(m_calledfromredundantindex == wyFalse)
    {
        FreeDBLevelResultsets();
	}

	if(obj == OBJECT_TABLES)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	
	}
	
	// get information about the tables.	
	query.Sprintf("show table status from `%s` ", db);

	if(ismysql5 == wyTrue)
	{
		query.AddSprintf("where engine is not null");		
	}

	temptitle.SetAs(_("/*Table Information*/\r\n"));
	AppendLine(temptitle);
	
    m_tableinfosres = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_tableinfosres); 
	
	if(!m_tableinfosres)
	{
		return wyFalse;
	}

	//HTML formatted info
	ret = ShowAllTablesHtmlFormat(pcquerywnd, m_tableinfosres, db, obj);

	return ret;
}

/*HTML formatted info when selected 'Database' or 'Tables' folder
When select Database, look into 5x objects also(if server supports) to add to HTML page
*/
wyBool
ObjectInfo::ShowAllTablesHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyString	columns;
	wyBool		ismysql5 = wyTrue, ret = wyTrue; 
	wyBool	    isdbobject = wyFalse; //this sets wyTrue if selected DB and supports 5x objects 
    
	ismysql5 = IsMySQL5(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql); 
	
	//If selection is DB, then we will display 5x objects also in HTML mode
	isdbobject = (ismysql5 == wyTrue && obj == OBJECT_DATABASE) ? wyFalse : wyTrue;

	//There's a column name difference, 'Type' changed to 'Engine' since version 4.1 
	if(pcquerywnd->m_ismysql41 == wyTrue)
	{
		columns.SetAs("Name,Engine,Rows,Data_length,Index_length,");
	}
	else
	{
		columns.SetAs("Name,Type,Rows,Data_length,Index_length,");
	}

	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, db, OBJECT_TABLES, 
		obj == OBJECT_DATABASE ? wyTrue : wyFalse, isdbobject);

	return ret;
}
	
wyBool
ObjectInfo::ShowAllViews(MDIWindow * pcquerywnd, const wyChar *db, OBJECT obj)
{
	wyBool		ret = wyTrue;
	wyString    query, temptitle, columns;

	if(obj == OBJECT_VIEWS)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	
	}

	//get information about the views
	query.Sprintf ("select `TABLE_NAME` as View_name,`View_definition`,`Check_option`,`Is_updatable`,`Definer`,`Security_type` from `INFORMATION_SCHEMA`.`VIEWS` where `TABLE_SCHEMA` = '%s'",db);
	
	//temptitle.Sprintf("/*View Information For - %s*/\r\n", db);
	temptitle.SetAs(_("/*View Information*/\r\n"));
	AppendLine(temptitle);
    m_viewinfores = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_viewinfores); 
	
	if(!m_viewinfores)
	{
		return wyFalse;
	}
	
	//HTML formatted o/p
	ret = ShowAllViewsHtmlFormat(pcquerywnd, m_viewinfores, db, obj);

	return ret;
}

/*Display info in HTML format, when selected the 'Database' or 'Views folder'
*/
wyBool
ObjectInfo::ShowAllViewsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyString	columns;
	wyBool		ret = wyTrue; 
	wyBool	    isobjectdb = wyFalse; 

	//If selected the DB, then we need to append all 5x objects in HTML buffer
	isobjectdb = (obj == OBJECT_DATABASE) ? wyFalse : wyTrue;

	//Selected columns to display in HTML control
	columns.SetAs("View_name,Is_updatable,Definer,Security_type,");

	//Disply HTML formatted resulted o/p, if the option is selectyed
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, db, OBJECT_VIEWS, 
							obj == OBJECT_DATABASE ? wyTrue : wyFalse, isobjectdb);
    
	return ret;
}

wyBool
ObjectInfo::ShowAllProcedures(MDIWindow *pcquerywnd,const wyChar *db, OBJECT obj)
{	
	wyBool		ret = wyTrue;
	wyString    query, temptitle, columns;
	wyBool		iscollate = wyFalse;

	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;

	if(obj == OBJECT_PROCEDURES)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	
	}

	if(iscollate)
		query.Sprintf("show procedure status where BINARY Db = '%s'", db);
	else
		query.Sprintf("show procedure status where Db = '%s'", db);

	//temptitle.Sprintf("/*Procedure Information For - %s*/\r\n", db);
	temptitle.SetAs(_("/*Procedure Information*/\r\n"));
	AppendLine(temptitle);
	m_procinfores = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_procinfores); 
	
	if(!m_procinfores)
	{
		return wyFalse;
	}

	//HTML formatted o/p
	ret = ShowAllProceduresHtmlFormat(pcquerywnd, m_procinfores, db, obj);
	
	return ret;
}

/*
Display Proc info in HTML format.
'Stored Proc' folder selectd or 'Database' selected
*/
wyBool
ObjectInfo::ShowAllProceduresHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyString	columns;
	wyBool		ret = wyTrue, isobjectdb = wyTrue;

	//Selected columns to display	
	columns.SetAs("Name,Definer,Security_type,Comment,");
	
	//If selection is 'Database' then we need to consider all 5x objects to display
	isobjectdb = (obj == OBJECT_DATABASE) ? wyFalse : wyTrue;

	//Disply HTML formatted resulted o/p, if the option is selected
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, db, OBJECT_PROCEDURES, 
		obj == OBJECT_DATABASE ? wyTrue : wyFalse, isobjectdb);	

	return ret;
}

wyBool 
ObjectInfo::ShowAllFunctions(MDIWindow *pcquerywnd,const wyChar *db, OBJECT obj)
{
	wyString    query, temptitle, columns;
	wyBool		iscollate = wyFalse;


	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;

	if(obj == OBJECT_FUNCTIONS)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	
	}
	
	if(iscollate)
		query.Sprintf("show function status where BINARY Db='%s'", db);
	else
		query.Sprintf("show function status where Db='%s'", db);

	temptitle.SetAs(_("/*Function Information*/\r\n"));
	AppendLine(temptitle);
    m_funcinfores = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_funcinfores); 
    	
	if(!m_funcinfores)
		return wyFalse;

	//Display in HTML format
	ShowAllFunctionsHtmlFormat(pcquerywnd, m_funcinfores, db, obj);

	/*if(myres)
		pcquerywnd->m_tunnel->mysql_free_result(myres);*/
	
	return wyTrue;
}

/*
Function info in HTML format.
'Functions' folder selectd or 'Database' selected
*/
wyBool
ObjectInfo::ShowAllFunctionsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyBool		ret = wyTrue, isobjectdb;
	wyString	columns;

	//Selected columns to display
	columns.SetAs("Name,Definer,Security_type,Comment");

	isobjectdb = (obj == OBJECT_DATABASE) ? wyFalse : wyTrue;

	//Disply HTML formatted resulted o/p, if the option is selected
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, db, OBJECT_FUNCTIONS, 
		obj == OBJECT_DATABASE ? wyTrue : wyFalse, isobjectdb);	

	return ret;
}

wyBool 
ObjectInfo::ShowAllTriggers(MDIWindow *pcquerywnd,const wyChar *db, OBJECT obj)
{
	wyString    query, temptitle, columns;
	wyBool		ret;
	
	if(obj == OBJECT_TRIGGERS)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));	
	}

	query.Sprintf("show triggers from `%s`", db);

	temptitle.SetAs(_("/*Trigger Information*/\r\n"));
	AppendLine(temptitle);
	m_triginfores = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_triginfores); 
	
	if(!m_triginfores)
		return wyFalse;

	//HTML result o/p
	ret = ShowAllTriggersHtmlFormat(pcquerywnd, m_triginfores, db, obj);
	
	return ret;
}

/*
Triggers info in HTML format.
'Triggers' folder selectd or 'Database' selected
*/
wyBool
ObjectInfo::ShowAllTriggersHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyBool		ret = wyTrue, isobjectdb;
	wyString	columns;

	//Selected columns to display
	columns.SetAs("Trigger,Event,Table,Timing,sql_mode,Definer,");
	
	//If server wont support events(if version less than MySQL516) we will display the HTML buffer in to HTL page
	isobjectdb = (obj == OBJECT_DATABASE && IsMySQL516(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql)) ? wyFalse : wyTrue;

	//Disply HTML formatted resulted o/p, if the option is selected
	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);
	
	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, &columns, db, OBJECT_TRIGGERS, 
				obj == OBJECT_DATABASE ? wyTrue : wyFalse, isobjectdb);

	return ret;
}

wyBool
ObjectInfo::ShowAllEvents(MDIWindow *pcquerywnd,const wyChar *db, OBJECT obj)
{
	wyBool		ret;
	wyString    query, temptitle;
	wyBool		iscollate = wyFalse;
	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;
	if(obj == OBJECT_EVENTS)
	{
		temptitle.Sprintf(_("/*Database: %s*/\r\n"), db);
		AppendLine(temptitle);

		VERIFY(SendMessage(m_hwnd, SCI_APPENDTEXT, temptitle.GetLength(),(LPARAM)temptitle.GetString()));
	}

	SendMessage(m_hwnd, SCI_APPENDTEXT, 2, (LPARAM)"\r\n");	
	if(iscollate)
		query.Sprintf("Select `Event_name`,`Definer`,`Event_type`,`Execute_at`,`Interval_value`,`Interval_field`,`Starts`,`Ends`,`Status` from `INFORMATION_SCHEMA`.`EVENTS` where BINARY `EVENT_SCHEMA` = '%s'",db);
	else
		query.Sprintf("Select `Event_name`,`Definer`,`Event_type`,`Execute_at`,`Interval_value`,`Interval_field`,`Starts`,`Ends`,`Status` from `INFORMATION_SCHEMA`.`EVENTS` where `EVENT_SCHEMA` = '%s'",db);

	temptitle.SetAs(_("/*Event Information*/\r\n"));
	AppendLine(temptitle);
    m_eventinfores = AddInfo(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, temptitle, wyFalse, m_eventinfores); 
	
	if(!m_eventinfores)
		return wyFalse;

	//Display result in HTML format
	ret = ShowAllEventsHtmlFormat(pcquerywnd, m_eventinfores, db, obj);

	return ret;
}

/*
Events info in HTML format, If version grater than or equal to 'MySQL516'
'Events' folder selectd or 'Database' selected
*/
wyBool
ObjectInfo::ShowAllEventsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj)
{
	wyBool ret = wyTrue;

	pcquerywnd->m_tunnel->mysql_data_seek(myres, 0);

	ret = AddObjectInfoHtmlFormat(pcquerywnd, myres, NULL, db, OBJECT_EVENTS, 
							obj == OBJECT_DATABASE ? wyTrue : wyFalse);

	return ret;
}


/*Show HTML o/p on Info tab on select 'server' in objectborwser.
SHOW STATUS; & SHOW VARIABLES;
*/
wyBool
ObjectInfo::ShowHtmlFormatResultOnServerObject(MDIWindow *wnd, MYSQL_RES *myres, wyBool isshowvars, wyString *htmlinbuff)					
{	
	wyString	query, variable, value, imgcaption, title, fomattedstr;
	MYSQL_ROW	row = NULL;
	wyInt32		rowcount = 0;
	wyBool		ismysql41 = wyFalse;
	wyBool		ismysql502 = wyTrue, ismysql403 = wyTrue;
	wyInt64		valueint = 0;
		
	if(!wnd || !wnd->m_pctabmodule)
		return wyFalse;

	ismysql41 = wnd->m_ismysql41;
	ismysql502 = IsMySQL502(wnd->m_tunnel, &wnd->m_mysql);
	ismysql403 = IsMySQL403(wnd->m_tunnel, &wnd->m_mysql);
	
	if(isshowvars == wyTrue)
	{
		htmlinbuff->Add("<table>");
		htmlinbuff->Add("<tr style=\"width:100%\">");

		//Session SHOW VARIABLES since 4.0.3
		(ismysql403 == wyTrue) ? title.SetAs("Show Variables [Local]") : title.SetAs("Show Variables [Global]");
	}

	else
	{	
		//Session SHOW STATUS VARIABLES since 5.0.2
		(ismysql502 == wyTrue) ? title.SetAs("Show Status [Local]") : title.SetAs("Show Status [Global]");
	}
	
	if(isshowvars == wyTrue)
		htmlinbuff->AddSprintf("<td style=\"width: 50%; padding:5px;\" valign=\"top\">");
	
	else
		htmlinbuff->AddSprintf("<td style=\"width: 50%; padding:5px;\" valign=\"top\">");

	htmlinbuff->AddSprintf("<div class=\"resultcaptionstyle\"><b>%s</b></div><br>", title.GetString());

	if(isshowvars == wyTrue)
		htmlinbuff->Add("<table class=\"statustablestyle\" style=\"float:left; valign:top\">");

	else
		htmlinbuff->Add("<table class=\"statustablestyle\" style=\"float:right\">");
		
		htmlinbuff->Add("<tbody>");

			htmlinbuff->Add("<tr class=\"captionfontstyle\">");

				htmlinbuff->Add("<th class=\"colcaptionstyleleft\" style=\"width:15%\">Variable_name</th>");
				htmlinbuff->Add("<th class=\"colcaptionstyleleft\" style=\"width:30%; \">Value</th>");
			htmlinbuff->Add("</tr>");
			
			//Show variables/status result sets
			while((row = wnd->m_tunnel->mysql_fetch_row(myres)))
			{
				if(!row || !row[0] || !row[1])
					continue;

				variable.SetAs(row[0], ismysql41);
				
				value.SetAs(row[1], ismysql41);

				//Format the long string
				if(value.GetLength() > 50)
					FormatHtmlLongString(&value);

				//Gets the SERVER - UPTIME
				if((isshowvars == wyFalse) && !variable.CompareI("Uptime"))
				{
					if(m_serveruptime.GetLength())
						m_serveruptime.Clear();

					TimeUnits(value.GetAsUInt32(), &m_serveruptime);
				}

				//Format numeric values(K, M, G)
				valueint = value.GetAsInt64();
			
				if(valueint)
				{	
					if(fomattedstr.GetLength())
						fomattedstr.Clear();

					StorageUnits(valueint, 2, &fomattedstr);					
					value.SetAs(fomattedstr);																
				}

				if(rowcount % 2)
					htmlinbuff->Add("<tr class=\"datafontstylerowodd\">");
				else
					htmlinbuff->Add("<tr class=\"datafontstyleroweven\">");

				htmlinbuff->AddSprintf("<td class=\"cellstyleleft\">%s</td>", variable.GetString());
				htmlinbuff->AddSprintf("<td class=\"cellstyleleft\">%s</td>", value.GetString());
				htmlinbuff->Add("</tr>");

				rowcount++;
			}			

		htmlinbuff->Add("</tbody></table></th>");
		
		if(isshowvars == wyFalse)
			htmlinbuff->Add("</table>");
			
	return wyTrue;
}

/*Handles the HTML formatted o/p for all objects other than 'SERVER' node
'columns' contains selected columns to disply, for showing all columns it's just NULL
*/
wyBool
ObjectInfo::AddObjectInfoHtmlFormat(MDIWindow *wnd, MYSQL_RES *myres, wyString *columns, const wyChar *db, OBJECT obj, wyBool isdbobject, wyBool istopasscontrol)
{
	wyString	strindex, data, htmldata, strfldindex, imgpath, pkimgpath, title, key, imgcaption, strnumeric;
	wyBool		ismysql41 = wyTrue, isshowallcolumns;

	wyString	htmlstrhead;
	wyString htmlhead, *htmlbuffer;
	

	if(!wnd || !db || (!myres && m_istostopcaption == wyFalse))
		return wyFalse;

	ismysql41 = wnd->m_ismysql41;

	//temphtml  = &m_htmlformatstr;
	obj == OBJECT_TABLE ? (htmlbuffer = &m_strtableinfo) : (htmlbuffer = &m_htmlformatstr);

	/*For all mySQL columns 'columns != NULL'
	For selected mySQL columns 'columns == NULL'
	*/
	isshowallcolumns = columns ? wyFalse : wyTrue;

	if(isdbobject == wyTrue)
	{
		if(obj == OBJECT_TABLES)
			HtmlBufferInit(htmlbuffer);

		//else
		//	//htmlbuffer->Add("<br>");
	}
	else
	{		
		switch(obj)
		{
		case OBJECT_INDEX:
			AddObjectInfoHtmlFormatForIndex(wnd, myres, db, htmlbuffer);
			return wyTrue;
		
		case OBJECT_VIEWDDL:
			htmlbuffer->Add("<br/>");
			break;
      
		case OBJECT_TABLEDDL:
			htmlbuffer->Add("<br/>");
			break;

		default:
			HtmlBufferInit(htmlbuffer);
		}
	}

    //Gets the Title if any, Image if any, Table caption if any
	AddHtmlFormatTitleAndImage(obj, db, &title, &imgpath, &imgcaption, isdbobject);
	
	//Adding the Title image
	if(imgpath.GetLength())
		{htmlbuffer->AddSprintf("<div><img  class=\"imgcaptionstyle\"src=\"%s\"/><span class=\"text1\">%s</span></div><br>", 
									imgpath.GetString(), imgcaption.GetString());
	htmlbuffer->Add("<div class=\"blueline\"></div>");}
    htmlbuffer->Add("<br><br><div class=\"captionstyle\"><b>");
	htmlbuffer->Add(title.GetString());
    htmlbuffer->Add("</div><br>");

	if(obj == OBJECT_TABLE && m_istostopcaption == wyFalse 
		&& m_isoptimizationaborted == wyFalse && m_isschemaoptimized == wyFalse && m_istableanalyse == wyFalse)
	{
		htmlbuffer->AddSprintf("<div class=\"resultcaptionstyle\">");

		if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
		{
			htmlbuffer->AddSprintf("<b>" SCHEMA_OPTBUTTON "</b></div>", LINK_SCHEMA_OPTIMIZE);
			htmlbuffer->Add("<div class=\"whitespace\"></div>");
			htmlbuffer->Add("<div class = \"warningstyle\">");
			htmlbuffer->AddSprintf("<b>%s</b></div></div>", OPTIMIZER_HELP);
		}
		
		htmlhead.SetAs(*htmlbuffer);
		htmlbuffer->Clear();
	}
    else if(obj == OBJECT_TABLES)
    {
        //with ultimate only
        if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
        {
            //add Redundant Index Button
            htmlbuffer->AddSprintf("<div class=\"resultcaptionstyle\">");

            if(m_showredundantindex == wyFalse)
                htmlbuffer->AddSprintf("<b>" REDUNDANT_INDEX_FINDBUTTON "</b></div>", LINK_REDUNDANT_INDEX_FIND);
            else
                htmlbuffer->AddSprintf("<b>" REDUNDANT_INDEX_FINDBUTTON "</b></div>", LINK_REDUNDANT_INDEX_HIDE);
        
            //Add the help text
			htmlbuffer->Add("<div class=\"whitespace\"></div>");
            htmlbuffer->Add("<div class = \"warningstyle\">");
	        htmlbuffer->AddSprintf("<b>%s</b></div></div><br>", REDUNDANT_INDEX_DB_HELP);
        }
    }

#ifndef COMMUNITY
	else if(obj == OBJECT_TABLE && m_istostopcaption == wyTrue)
	{
		htmlbuffer->AddSprintf("<div class=\"tabcaptionstyle\">");

		if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
			htmlbuffer->AddSprintf("<b>" SCHEMA_OPTBUTTON "</b></div></html>", LINK_STOP_SCHEMA_OPTIMIZE);

		istopasscontrol = wyTrue;

		PrintHTML(obj, htmlbuffer);
	}

	else if(m_isoptimizationaborted == wyTrue && obj == OBJECT_TABLE)
	{		
		htmlhead.SetAs(*htmlbuffer);
		htmlbuffer->Clear();

		PrintHTML(obj, htmlbuffer);
	}

	else if(obj == OBJECT_TABLE && m_isschemaoptimized == wyFalse)
	{		
		htmlhead.SetAs(*htmlbuffer);
		htmlbuffer->Clear();
	}
#endif

	if(obj == OBJECT_TABLE &&	pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO 
		&& pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
		htmlbuffer->Add("<br/>");

	//htmlbuffer->Add("<div style=width: auto;>");
	htmlbuffer->Add("<div>");
	
	RECT rcthtml;
	GetClientRect(m_hwndhtmleditor, &rcthtml);
	m_pagewidth = rcthtml.right;
	
	if(m_istableanalyse == wyTrue)
		m_pagewidth -= 250;

	else if(obj != OBJECT_TABLEDDL)
		m_pagewidth -= 450;
		
	if(obj == OBJECT_TABLEDDL)
		htmlbuffer->AddSprintf("<table class=\"statustablestyle\">");
	
	else
	{
		if(ismysql41 == wyFalse)
			m_pagewidth -= 250;
		
		htmlbuffer->AddSprintf("<table class=\"statustablestyle\" width=\"%d\">", m_pagewidth);
	}
		
	htmlbuffer->Add("<tbody>");

	//row style
	if(obj == OBJECT_TABLEDDL)
		htmlbuffer->Add("<tr class=\"captionfontstyle\">");

	else
		htmlbuffer->Add("<tr class=\"captionfontstyle\">");

	//For PK column style with Table or Index 		
	//if(m_istableanalyse == wyTrue && obj == OBJECT_TABLE)
	//	htmlbuffer->Add("<td class=\"pkcolcaptionstyle\"></td>");							
		
	//else if(obj == OBJECT_INDEX)
		//htmlbuffer->Add("<td class=\"pkcolcaptionstyle\"></td>");							

	if(obj == OBJECT_TABLE || obj == OBJECT_INDEX)
		htmlbuffer->Add("<td class=\"pkcolcaptionstyle\"></td>");							
    
	//Adding Table data
	if(isshowallcolumns == wyFalse)
		AddHtmlSelectedMySQLColumns(wnd, db, myres,obj, columns, htmlbuffer);

	else
		AddHtmlAllMySQLColumns(wnd, myres,obj);

	htmlbuffer->Add("</tbody>");
	htmlbuffer->Add("</table>");
	htmlbuffer->Add("</div>");
	
	if(obj == OBJECT_TABLE && htmlhead.GetLength())
	{
		if(m_istableanalyse == wyTrue && m_isschemaoptimized == wyTrue)
		{
			htmlhead.AddSprintf("<div class=\"tabcaptionstyle\">");

			if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
			{
                htmlhead.AddSprintf("<b>" SCHEMA_OPTBUTTON "</b>", LINK_HIDE_SCHEMA_OPTIMIZE);
				htmlbuffer->Add("<div class=\"whitespace\"></div>");
				htmlhead.Add("<div class = \"warningstyle\">");
				htmlhead.AddSprintf("<b>%s</b></div></div>", OPTIMIZER_HELP);
			}
		}

		else if(m_istableanalyse == wyTrue && m_isoptimizationaborted == wyTrue)
		{
			htmlhead.AddSprintf("<div class=\"tabcaptionstyle\">");

			if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
			{
				htmlhead.AddSprintf("<b>" SCHEMA_OPTBUTTON "</b></div>", LINK_SCHEMA_OPTIMIZE);
				htmlbuffer->Add("<div class=\"whitespace\"></div>");
				htmlhead.Add("<div class = \"warningstyle\">");
				htmlhead.AddSprintf("<b>%s</b></div></div>", OPTIMIZER_HELP);
			}
		}

		htmlhead.Add(htmlbuffer->GetString());

		m_strtableinfo.SetAs(htmlhead);
	}

	//Send the HTML buffer to control
	if(istopasscontrol == wyTrue)
	{
		PrintHTML(obj, htmlbuffer);
	}

	return wyTrue;
}

void
ObjectInfo::PrintHTML(OBJECT obj, wyString *htmlbuffer)
{

	//Send the HTML buffer to control
		if(obj == OBJECT_TABLEDDL && m_strtableinfo.GetLength())
		{
			m_strtableinfo.Add(m_htmlformatstr.GetString());

			htmlbuffer = &m_strtableinfo;
		}
		
		else if(m_istostopcaption == wyTrue)
			htmlbuffer = &m_strtableinfo;

		else
			htmlbuffer = &m_htmlformatstr;
		
		htmlbuffer->Add("</html>");

		//Passs the HTML buffer to HTML control
		HTMLayoutLoadHtml(m_hwndhtmleditor, (PBYTE)htmlbuffer->GetString(), htmlbuffer->GetLength());

		//html page Selection mode
		HTMLayoutSetMode(m_hwndhtmleditor, HLM_SHOW_SELECTION);
	}

wyBool
ObjectInfo::HandleOptimizedSchemaTable(MDIWindow *wnd, wyString *htmlbuffer, MYSQL_RES *myresfields, wyString *columns)
{	
	//Table style
	htmlbuffer->Add("<table BGCOLOR=\"#FF0000\" class=\"statustablestyle\">");	
	htmlbuffer->Add("<tbody>");

	//row style
	htmlbuffer->Add("<tr class=\"captionfontstyle\">");

	//For PK column style with Table or Index 
	htmlbuffer->Add("<th class=\"pkcolcaptionstyle\"></th>");							
	
	htmlbuffer->Add("</tbody>");
	htmlbuffer->Add("</table>");
	htmlbuffer->Add("</div></html>");

	return wyTrue;
}

//Gets the HTML Title, Image, Table caption if any
wyBool	
ObjectInfo::AddHtmlFormatTitleAndImage(OBJECT obj, const wyChar *db, wyString *title, wyString *image, wyString *imagecaption, wyBool isdbobject)
{
	if(obj == OBJECT_DATABASE && db)
	{		
		image->SetAs("res:DATABASE.PNG");

		//For showing count of objects
		if(m_tableinfosres)
		{
			title->Sprintf(_("Tables (%d) "), m_tableinfosres->row_count);
		}
		else
		{
		title->Sprintf(_("Table Information"));
		}
		imagecaption->Sprintf(_("Database: %s"), db);		
	}

	else if(obj == OBJECT_TABLES && db)
	{
		image->SetAs("res:DATABASE.PNG");

		//For showing count of objects
		if(m_tableinfosres)
		{
			title->Sprintf(_("Tables (%d) "), m_tableinfosres->row_count);
		}
		else
		{
		title->Sprintf(_("Table Information"));
		}

		imagecaption->Sprintf(_("Database: %s"), db);
	}

	else if(obj == OBJECT_TABLE && db)
	{
		image->SetAs("res:TABLE.PNG");

		//For showing count of objects
		if(m_myrescolumninfo)
		{
			title->Sprintf(_("Columns (%d) "), m_myrescolumninfo->row_count);
		}
		else
		{
		title->Sprintf(_("Column Information"));
		}

		imagecaption->Sprintf(_("&nbsp;Table: %s"), db);		
	}

	else if(obj == OBJECT_TABLEDDL || obj == OBJECT_VIEWDDL)
		title->Sprintf(_("DDL Information"));

	else if(obj == OBJECT_PROCEDURES)
	{		
		//For showing count of objects
		if(m_procinfores)
		{
			title->Sprintf(_("Procedures (%d) "), m_procinfores->row_count);
		}
		else
		{
		title->Sprintf(_("Procedure Information"));		
		}
		
		if(isdbobject == wyFalse)
		{
			image->SetAs("res:DATABASE.PNG");
			imagecaption->Sprintf(_("Database: %s"), m_lastseldb.GetString());
		}
	}

	else if(obj == OBJECT_PROCEDURE && db)
	{
		image->SetAs("res:SP.PNG");
		title->Sprintf(_("Procedure Information"));
		imagecaption->Sprintf(_("&nbsp;Stored Proc: %s"), db);
	}

	else if(obj == OBJECT_VIEWS && db)
	{
		//For showing count of objects
		if(m_viewinfores)
		{
			title->Sprintf(_("Views (%d) "), m_viewinfores->row_count);
		}
		else
		{
		title->Sprintf(_("View Information"));
		}
		
		if(isdbobject == wyFalse)
		{
			image->SetAs("res:DATABASE.PNG");
			imagecaption->Sprintf(_("Database: %s"), m_lastseldb.GetString());
		}
	}

	else if(obj == OBJECT_FUNCTIONS && db)
	{
		//For showing count of objects
		if(m_funcinfores)
		{
			title->Sprintf(_("Functions (%d) "), m_funcinfores->row_count);
		}
		else
		{
		title->Sprintf(_("Function Information"));		
		}
		
		if(isdbobject == wyFalse)
		{
			image->SetAs("res:DATABASE.PNG");
			imagecaption->Sprintf(_("Database: %s"), m_lastseldb.GetString());
		}
	}

	else if(obj == OBJECT_FUNCTION && db)
	{
		image->SetAs("res:FUNCTION.PNG");
		title->Sprintf(_("Function Information"));
		imagecaption->Sprintf(_("&nbsp;Function: %s"), db);
	}

	else if(obj == OBJECT_TRIGGERS && db)
	{
		//For showing count of objects
		if(m_triginfores)
		{
			title->Sprintf(_("Triggers (%d) "), m_triginfores->row_count);
		}
		else
		{
		title->Sprintf(_("Trigger Information"));		
		}
		
		if(isdbobject == wyFalse)
		{
			image->SetAs("res:DATABASE.PNG");
			imagecaption->Sprintf(_("Database: %s"), m_lastseldb.GetString());
		}
	}

	else if(obj == OBJECT_TRIGGER && db)
	{
		image->SetAs("res:TRIGGER.PNG");
		title->Sprintf(_("Trigger Information"));
		imagecaption->Sprintf(_("&nbsp;Trigger: %s"), db);
	}

	else if(obj == OBJECT_EVENTS && db)
	{
		//For showing count of objects
		if(m_eventinfores)
		{
			title->Sprintf(_("Events (%d) "), m_eventinfores->row_count);
		}
		else
		{
		title->Sprintf(_("Event Information"));		
		}
		
		if(isdbobject == wyFalse)
		{
			image->SetAs("res:DATABASE.PNG");
			imagecaption->Sprintf(_("Database: %s"), m_lastseldb.GetString());
		}
	}
	
	else if(obj == OBJECT_EVENT && db)
	{
		image->SetAs("res:EVENT.PNG");
		title->Sprintf(_("Event Information"));
		imagecaption->Sprintf(_("&nbsp;Event: %s"), db);
	}

	else if(obj == OBJECT_VIEW && db)
	{
		image->SetAs("res:VIEW.PNG");
		title->Sprintf(_("View Information"));
		imagecaption->Sprintf(_("&nbsp;View: %s"), db);
	}

	return wyTrue;
}

//Displays the 'Index' table when selected 'Table' node in obj-browser.
wyBool
ObjectInfo::AddObjectInfoHtmlFormatForIndex(MDIWindow *wnd, MYSQL_RES *myres, const wyChar *db, wyString *htmlbuffer)
{
	wyInt32     fldindex = -1, colindex = -1, indexcol = -1, rowindex = 0;
	wyBool	    isunique = wyFalse, isfulltext = wyFalse;
	wyString	indexname, columnname, query, colstr, /*imgpath,*/ title, pkimgpath, data, redundantof, colstyle;
	MYSQL_ROW	myrow = NULL;
	wyBool		ismysql41 = wnd->m_ismysql41;
	wyString    tempbuff;
	wyInt32     count = 0;
	
#ifndef COMMUNITY
    wyBool isredundant = wyFalse;
#endif

	////For showing count of objects
	//if(m_myrestableindexinfo)
	//{
	//	title.Sprintf("Indexes (%d) ", m_myrestableindexinfo->row_count);
	//}
	//else
	//{
	//	title.Sprintf("Index Information");// For - %s", db);
	//}
	//	
	//htmlbuffer->AddSprintf("<br/><div id=\"indexinfo\" class=\"captionstyle\"><b>");
	//htmlbuffer->Add(title.GetString());
	//htmlbuffer->Add("</b></div><br>");

    //with ultimate only
    if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
    {
        //add the redundant index button
        tempbuff.Add("<div class=\"resultcaptionstyle\">");

        if(m_showredundantindex == wyFalse)
            tempbuff.AddSprintf("<b>" REDUNDANT_INDEX_FINDBUTTON "</b></div>", LINK_REDUNDANT_INDEX_FIND);
        else
            tempbuff.AddSprintf("<b>" REDUNDANT_INDEX_FINDBUTTON "</b></div>", LINK_REDUNDANT_INDEX_HIDE);

        //add redundant index help
		
        tempbuff.Add("<br><div class = \"warningstyle\">");
	    tempbuff.AddSprintf("<b>%s</b></div></div><br>", REDUNDANT_INDEX_TABLE_HELP);
    }

	

	tempbuff.Add("<table class=\"statustablestyle\">");	
	tempbuff.Add("<tbody>");

	//Setting the table captions
	tempbuff.Add("<tr class=\"captionfontstyle\">");
	tempbuff.Add("<th class=\"pkcolcaptionstyle\"></th>");	
	tempbuff.AddSprintf("<th class=\"colcaptionstyleleft\">Indexes</th>");
	tempbuff.AddSprintf("<th class=\"colcaptionstyleleft\">Columns</th>");
	tempbuff.AddSprintf("<th class=\"colcaptionstyleleft\">Index Type</th>");
    
    if(m_showredundantindex == wyTrue)
    {
        //add the extra columns for redundant indexes info
        tempbuff.AddSprintf("<th class=\"colcaptionstyleleft\">Is Redundant?</th>");
        tempbuff.AddSprintf("<th class=\"colcaptionstyleleft\">Redundant Of</th>");
    }

	tempbuff.AddSprintf("</tr>");

	while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
	{
		fldindex = GetFieldIndex(myres,"key_name", wnd->m_tunnel);
		colindex = GetFieldIndex(myres,"column_name", wnd->m_tunnel);

		if(fldindex == -1 || colindex == -1)
			continue;

		if((strcmp(indexname.GetString(), myrow[fldindex]))== 0)
		{
			colstr.SetAs(myrow[colindex], ismysql41);
			columnname.Add(colstr.GetString());
			columnname.Add(", ");
		}
		else if(indexname.GetLength() == 0)
		{
			indexname.SetAs(myrow[fldindex], ismysql41);
			columnname.SetAs(myrow[colindex], ismysql41);
			columnname.Add(", ");
			count ++;

			// check whether its unique.
			if(stricmp(myrow[GetFieldIndex(myres,"non_unique", wnd->m_tunnel, &wnd->m_mysql)], "0")== 0)
				isunique = wyTrue;

			if(IsMySQL402(wnd->m_tunnel, &wnd->m_mysql))
            {
				indexcol = GetFieldIndex(myres, "index_type", wnd->m_tunnel, &wnd->m_mysql);
				if(indexcol == -1)
					continue;

				if(myrow[indexcol] && strstr(myrow[indexcol], "FULLTEXT"))
					isfulltext = wyTrue;					
			} 
		}
		else if(indexname.GetLength() > 0)
		{
#ifndef COMMUNITY
            //check whether the index is redundant
			if(m_showredundantindex == wyTrue)
                isredundant = m_redindexfinder->IsRedundant(&indexname, &redundantof);

            //if so, set the redundant index style
            if(isredundant == wyTrue)
                tempbuff.Add("<tr class=\"redindexrowstyle\">");
            else
#endif
            {
                //Set alternate color for rows
		        if(rowindex % 2)
			        tempbuff.Add("<tr class=\"datafontstylerowodd\">");
		        else
			        tempbuff.Add("<tr class=\"datafontstyleroweven\">");
            }

			count++;
			columnname.GetLength();
			columnname.Strip(2);
				
			//Sets the 'Key' imge if its PK else keep it empty
			if(!indexname.CompareI("PRIMARY") && isunique == wyTrue)
			{
				pkimgpath.SetAs("res:PK.PNG");
				tempbuff.AddSprintf("<td><img src=\"%s\"/></td>", pkimgpath.GetString());	
			}

			else
				tempbuff.Add("<td></td>");

			tempbuff.AddSprintf("<td class=\"cellstyleleft\">%s</td>", indexname.GetString());

			tempbuff.AddSprintf("<td class=\"cellstyleleft\">%s</td>", columnname.GetString());

			//count++;
														
			//Sets the index type
			if(isunique)
				tempbuff.AddSprintf("<td class=\"cellstyleleft\">Unique</td>");
				
			else if(isfulltext)
				tempbuff.AddSprintf("<td class=\"cellstyleleft\">Fulltext</td>");
				
			else
				tempbuff.AddSprintf("<td class=\"cellstyleleft\">" "</td>");
#ifndef COMMUNITY

            if(m_showredundantindex == wyTrue)
            {
                //use optimize column style if the index is not redundant
                colstyle.SetAs(isredundant ? "cellstyleleft" : "optimizecolstyle");
                tempbuff.AddSprintf("<td class=\"%s\">%s</td>", colstyle.GetString(), isredundant? "Yes" : "No");
                tempbuff.AddSprintf("<td class=\"%s\">%s</td>", colstyle.GetString(), redundantof.GetString());
            }
#endif
			columnname.Strip(columnname.GetLength());
			indexname.Strip(indexname.GetLength());
			
			isunique = wyFalse;
			isfulltext = wyFalse;

			fldindex = GetFieldIndex(myres,"column_name", wnd->m_tunnel, &wnd->m_mysql);
			if(fldindex == -1)
				continue;

			// now copy this key into the buffer.
			columnname.SetAs(myrow[fldindex], ismysql41);
			columnname.Add(", ");

			fldindex = GetFieldIndex(myres,"key_name", wnd->m_tunnel, &wnd->m_mysql);
			if(fldindex == -1)
				continue;

			indexname.SetAs(myrow[fldindex], ismysql41);

			fldindex = GetFieldIndex(myres,"non_unique", wnd->m_tunnel, &wnd->m_mysql);
			if(fldindex == -1)
				continue;

			if(stricmp(myrow[fldindex], "0")== 0)
				isunique = wyTrue;
			
			if(IsMySQL402(wnd->m_tunnel, &wnd->m_mysql))
            {
				indexcol = GetFieldIndex(myres,"index_type", wnd->m_tunnel, &wnd->m_mysql);
				if(indexcol == -1)
					continue;

				if(myrow[indexcol] && strstr(myrow[indexcol], "FULLTEXT"))
					isfulltext = wyTrue;					
			} 

            tempbuff.Add("</tr>");				 				  
            rowindex++;
		}		
	}

	//count of object
	title.Sprintf(_("Indexes (%d) "), count);
		
	htmlbuffer->AddSprintf("<br/><div id=\"indexinfo\" class=\"captionstyle\"><b>");
	htmlbuffer->Add(title.GetString());
	htmlbuffer->Add("</b></div><br>");

	//Add rest details
	htmlbuffer->Add(tempbuff.GetString());


	// now add the last key.
	if(indexname.GetLength() > 0)
	{
		columnname.Strip(2);
#ifndef COMMUNITY
        //check whether the index is redundant
        if(m_showredundantindex == wyTrue)
            isredundant = m_redindexfinder->IsRedundant(&indexname, &redundantof);

        //if so, use the redundant index style
        if(isredundant == wyTrue)
            htmlbuffer->Add("<tr class=\"redindexrowstyle\">");
        
        else
#endif
        {
            //Set alternate color for rows
		    if(rowindex % 2)
			    htmlbuffer->Add("<tr class=\"datafontstylerowodd\">");
		    else
			    htmlbuffer->Add("<tr class=\"datafontstyleroweven\">");
        }

		//Sets the 'Key' imge if its PK else keep it empty
		if(!indexname.CompareI("PRIMARY") && isunique == wyTrue)
		{
			pkimgpath.SetAs("res:PK.PNG");
			htmlbuffer->AddSprintf("<td><img src=\"%s\"/></td>", pkimgpath.GetString());	
		}

		else
			htmlbuffer->Add("<td></td>");

		htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">%s</td>", indexname.GetString());

		htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">%s</td>", columnname.GetString());
							
		//Sets the index type
		if(isunique)
			htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">Unique</td>");
			
		else if(isfulltext)
			htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">Fulltext</td>");
			
		else
			htmlbuffer->AddSprintf("<td class=>" "</td>");		

#ifndef COMMUNITY
        if(m_showredundantindex == wyTrue)
        {
            //use optimize column style if the index is not redundant
            colstyle.SetAs(isredundant ? "cellstyleleft" : "optimizecolstyle");
            htmlbuffer->AddSprintf("<td class=\"%s\">%s</td>", colstyle.GetString(), isredundant? "Yes" : "No");
            htmlbuffer->AddSprintf("<td class=\"%s\">%s</td>", colstyle.GetString(), redundantof.GetString());
        }
#endif

	}

	htmlbuffer->Add("</tbody>");
	htmlbuffer->Add("</table>");
	htmlbuffer->Add("</div>");

#ifndef COMMUNITY
        if(m_showredundantindex == wyTrue)
            AddAlterStmtForRedundantIndex(htmlbuffer);
#endif

	return wyTrue;
}

//Adding the Selected coulmn(s) of MySQL result to Info tab
void
ObjectInfo::AddHtmlSelectedMySQLColumns(MDIWindow *wnd, const wyChar *objectname, MYSQL_RES *myres, OBJECT obj, wyString *columns, wyString *htmlbuffer)
{
	wyBool		isshowall, ismysql41;
	wyChar		*tok = NULL, *dup = NULL;
	wyInt32		keytype = -1;
	wyInt32		formatcolindex = -1; // Index of the coulmn that needs to be Formatted(DLL statements)
	wyString	strindex, data, key, pkimgpath, query;
	wyInt32		datalencolindex = -1, indexlencolindex = -1;
	
	isshowall = columns ? wyFalse : wyTrue;
	ismysql41 = wnd->m_ismysql41;

	//Check the index type of MySQL table column is PK or not. For PK we add key icon to represent that
	if(obj == OBJECT_TABLE)
    {
		keytype = GetFieldIndex(wnd->m_tunnel, myres, "Key");
    }

	/*For selected columns, 1st finds the Index of columns and add to buffer with coma deleimitter.
	Parse this buffer when fetching the column from the MySQL result set
	*/
	if(isshowall == wyFalse)
	{
		dup = strdup(columns->GetString());
		tok = strtok(dup, ",");
	}
		
	//For Table-column information
	if(obj == OBJECT_TABLE)
    {
		AddColumnHeaderForTableColumnInfo(tok, htmlbuffer, strindex, wnd, myres);
    }
	else if(obj != OBJECT_TABLE)
    {
		AddColumnHeaderWithOutSchemaAnalyze(tok, htmlbuffer, strindex, wnd, myres, obj, formatcolindex, datalencolindex, indexlencolindex);
    }

	/*Add an Extra column when selected the 'Database' or 'Tables' folder
	'Total Size = Data Size(Data_Length) + Index Size(Index_Length)*/
	if(obj == OBJECT_TABLES)
    {
		htmlbuffer->Add("<td class=\"colcaptionstyleleft\">Total Size</td>");

#ifndef COMMUNITY
        //add aditional column for showing redundant indexes
        if(m_showredundantindex == wyTrue)
        {
            htmlbuffer->Add("<td class=\"colcaptionstyleleft\">Redundant Indexes</td>");
        }
#endif
    }
							
	if(dup)
    {
		free(dup);
    }
	
	htmlbuffer->Add("</tr>");
	
	if(obj == OBJECT_TABLE)
    {
		AddColumnDataForTableColumnInformation(strindex, keytype, htmlbuffer, wnd, myres);
    }
	else
    {
		AddColumnDataInformation(strindex, keytype, htmlbuffer, wnd, myres, obj, objectname, datalencolindex, indexlencolindex, formatcolindex); 
    }
}

//Add the column for 'optimal_field_type' of PROCEDURE ANALYSE()
void
ObjectInfo::AddOptimazierFieldColumn(wyString *htmlbuffer)
{
#ifndef COMMUNITY
	htmlbuffer->Add("<td class=\"colcaptionstyleleft\">Optimal Datatype</td>");
#endif    
	return;
}

//With Schama Optimize, add new column 'Optimal Field type' with Column information html table
void
ObjectInfo::AddColumnHeaderForTableColumnInfo(wyChar *tok, wyString *htmlbuffer, wyString &strindex, MDIWindow *wnd, MYSQL_RES *myres)
{
	wyInt32 fldindex; 	

	while(tok)
	{
		fldindex = GetFieldIndex(wnd->m_tunnel, myres, tok);        
				
		if(fldindex != -1)
		{			
			strindex.AddSprintf("%d,", fldindex);			
			
			if(!stricmp(tok, "Field"))
            {
				htmlbuffer->AddSprintf("<td class=\"colcaptionstyleleft\">%s</td>", tok);							
            }						
			else if(!strcmp(tok, "Type"))
			{
				htmlbuffer->AddSprintf("<td class=\"colcaptionstyleleft\">%s</td>", tok);											
			
                if(m_istableanalyse == wyTrue)
                {
					AddOptimazierFieldColumn(htmlbuffer);
                }
			}			
			//NULL Coulmn to be part of 'Type' column
			else if(stricmp(tok, "NULL"))
            {
				htmlbuffer->AddSprintf("<td class=\"colcaptionstyleleft\">%s</td>", tok);							
            }
		}

		tok = strtok(NULL, ",");		
	}	
}

void
ObjectInfo::AddColumnHeaderWithOutSchemaAnalyze(wyChar *tok, wyString *htmlbuffer, wyString &strindex,  
											   MDIWindow *wnd, MYSQL_RES *myres, OBJECT obj, 
											   wyInt32 &formatcolindex, wyInt32 &datalencolindex, wyInt32 &indexlencolindex)
{
    wyInt32 fldindex, colinfoindex = 0;
    
	//Add the selected coulmn captions of the table
	while(tok)
	{
		fldindex = GetFieldIndex(wnd->m_tunnel, myres, tok);
				
		//Keep the index of column in a buffer if its valid(ie != -1)
		if(fldindex != -1)
		{
			/*If selected 'Tables' we addd a extra column 'Total Size, (Total size = Data_length + Index_length)
			'Data_length' renamed to 'Data Size'and 'Index_length' renamed to 'Index Size'
			*/
			if(obj == OBJECT_TABLES)
			{
				if(!stricmp(tok, "Data_length"))
				{
					datalencolindex = fldindex; //Keeps the index of column "Data_length"
					strcpy(tok, "Data Size");
				}
				else if(!stricmp(tok, "Index_length"))
				{
					indexlencolindex = fldindex;  //Keeps the index of column "Index_length"
					strcpy(tok, "Index Size");
				}
			}

			strindex.AddSprintf("%d,", fldindex);
			
			if(obj == OBJECT_TABLEDDL)
            {
                htmlbuffer->AddSprintf("<td class=\"colcaptionstyleleft\">%s</td>",tok);							
            }
			else
            {
				htmlbuffer->AddSprintf("<td class=\"colcaptionstyleleft\">%s</td>", tok);							
            }
		}
				
		//If a column to be formatted, keep its index in a buffer
		if((obj == OBJECT_PROCEDURE && !stricmp(tok, "Create Procedure"))
			||(obj == OBJECT_FUNCTION && !stricmp(tok, "Create Function"))
			||(obj == OBJECT_TRIGGER && !stricmp(tok, "Statement"))
			||(obj == OBJECT_EVENT && !stricmp(tok, "Create Event"))
			||(obj == OBJECT_VIEWDDL && !stricmp(tok, "Create View"))
			||(obj == OBJECT_TABLEDDL && !stricmp(tok, "Create Table")))
        {
			formatcolindex = fldindex;  //Index of column to format
        }
		
		tok = strtok(NULL, ",");
		colinfoindex ++;
	}
}

/*
-This fun gets called only when click on 'Analyze
-If click on 'Analyze' then add extra column 'Optimal_fieldtype' and also 'Optimized Schema'
*/
void
ObjectInfo::AddColumnDataForTableColumnInformation(wyString &strindex, wyInt32 keytype, wyString *htmlbuffer, MDIWindow *wnd, MYSQL_RES *myres)
{
	MYSQL_ROW	myrow = NULL, myrowanalyse = NULL;;
	wyInt32		rowcount = 0, colinfoindex = 0, index = 0, indexoptimize = 0, optimizecolcount = 0, autoincr = -1;
	wyChar		*dup = NULL, *tok = NULL;
	wyString	key, pkimgpath, data, strfld, acstr;
	wyBool		ismysql41 = wnd->m_ismysql41, isaccol = wyFalse;

	if(m_istableanalyse == wyTrue)
	{
		indexoptimize = GetFieldIndex(wnd->m_tunnel, m_myrestableanalyse, "Optimal_fieldtype");

		if(m_optimizedschema.GetLength())
        {
			m_optimizedschema.Clear();
        }
		
		optimizecolcount = wnd->m_tunnel->mysql_num_rows(m_myrestableanalyse);
	}
	
	//Find the 'Extra' field index for finding AUTO_INCREMENT column
	if(m_istableanalyse == wyTrue)
    {
		autoincr = GetFieldIndex(wnd->m_tunnel, myres, "Extra");
    }
			
	while(m_isoptimizationaborted == wyFalse && (myrow = wnd->m_tunnel->mysql_fetch_row(myres)) && 
		((optimizecolcount == 0 ) || (myrowanalyse = wnd->m_tunnel->mysql_fetch_row(m_myrestableanalyse))))
	{        		
		if(rowcount % 2)
        {
			htmlbuffer->Add("<tr class=\"datafontstylerowodd\">");
        }
		else
        {
			htmlbuffer->Add("<tr class=\"datafontstyleroweven\">");
        }

		dup = strdup(strindex.GetString());
		tok = strtok(dup, ",");
		
		//Add the 'key' image for 'table' and its 'index' table
		if(myrow && myrow[keytype])
        {
			key.SetAs(myrow[keytype], ismysql41);
        }
		else 
        {
			key.SetAs("(NULL)");
        }
            
		//Key icon to represent PK
		if(key.CompareI("PRI") == 0)
		{
			pkimgpath.SetAs("res:PK.PNG");
			htmlbuffer->AddSprintf("<td><img src=\"%s\"/></td>", pkimgpath.GetString());	
		}
		else
        {
			htmlbuffer->Add("<td></td>");								
        }
					
		colinfoindex = 0;
		
        /*Showing the selected columns.
		parse the selected column index buffer and fetches the column from result set
		*/
		while(tok)
		{			
			index = atoi(tok);
			            			
			if(!myrow || !myrow[index])
            {
				data.SetAs("(NULL)");
            }

			//For NULL coulmn appending to the 'Type' column
			if(index == (ismysql41 == wyTrue ? 3 : 2))
            {
				!stricmp(myrow[index], "YES") ?	data.Add(" NULL") : data.Add(" NOT NULL");
            }			
			else
			{
				data.SetAs(myrow[index], ismysql41);

				if(index == 0) //Field name
                {
					strfld.SetAs(data);  				
                }
			}

			tok = strtok(NULL, ",");
			
			//If coulmn was 'Type' it add to HTML table together with 'NULL' column
			if(index == 1)
            {
				continue;
            }

			colinfoindex ++;
									
			htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">%s</td>", data.GetString());
			
			//Addding the optimal_field type column
			if(m_istableanalyse == wyTrue && colinfoindex == 2)
			{
				isaccol = wyFalse;
				if(autoincr > 0 && myrow && myrow[autoincr])
				{
					acstr.SetAs(myrow[autoincr]);

					if(!acstr.CompareI("auto_increment"))
                    {
						isaccol = wyTrue;
                    }
				}

				if(optimizecolcount > 0)
                {
					FrameOptimizedSchemaStatement(myrowanalyse, indexoptimize, ismysql41, 
												  (wyChar*)strfld.GetString() ,
												  htmlbuffer, isaccol);			
                }
				else
                {
					htmlbuffer->Add("<td class=\"optimizecolstyle\"></td>");
                }
			}
		}
				
		htmlbuffer->Add("</tr>");

		if(dup)
        {
			free(dup);
        }

		rowcount++;
	}

	if(m_isoptimizationaborted == wyFalse)
    {
		m_isschemaoptimized = wyTrue;
    }
}


void
ObjectInfo::AddColumnDataInformation(wyString &strindex, wyInt32 keytype, wyString *htmlbuffer, MDIWindow *wnd, 
									MYSQL_RES *myres, OBJECT obj, const wyChar *objectname, wyInt32 datalencolindex, 
									wyInt32 indexlencolindex, wyInt32 formatcolindex)
{
	MYSQL_ROW	myrow = NULL;
	wyChar		*tok = NULL, *dup = NULL;
	wyString	key, pkimgpath, data, redundantindexes, tablename, tablelink;
	wyUInt32	rowcount = 0; 
	wyInt32		index = 0;
	wyInt64		valueint = 0, totallen = 0, datalen = 0, indexlen = 0;
	wyBool		isnumericdata = wyFalse, ismorewidth = wyFalse;

#ifndef COMMUNITY
    wyBool      isredundant = wyFalse;
#endif

	if(obj == OBJECT_TABLES)
    {
		isnumericdata = wyTrue;
    }

	//Shows the column data
	while((m_istableanalyse == wyFalse && obj != OBJECT_TABLE) && (myrow = wnd->m_tunnel->mysql_fetch_row(myres)))
	{
        tablename.SetAs(myrow[0], wnd->m_ismysql41);
#ifndef COMMUNITY
        //get the redudnat indexes for each table
        if(m_showredundantindex == wyTrue && (obj == OBJECT_TABLES || obj == OBJECT_DATABASE))
        {
            isredundant = m_redindexfinder->GetRedundantIndexes(tablename, redundantindexes);
        }

        //if any redundant index is found, then use redundant index style for html
        if(isredundant == wyTrue)
        {
            htmlbuffer->Add("<tr class=\"redindexcolstyle\">");
        }
        else
#endif
        {
            //give alternate colors for each row
    		if(rowcount % 2)
            {
	    		htmlbuffer->Add("<tr class=\"datafontstylerowodd\">");
            }
    		else
            {
	    		htmlbuffer->Add("<tr class=\"datafontstyleroweven\">");
            }
        }

        dup = strdup(strindex.GetString());
		tok = strtok(dup, ",");
		
		//Add the 'key' image for 'table' and its 'index' table
		if(obj == OBJECT_INDEX)
		{
			if(myrow && myrow[keytype])
            {
				key.SetAs(myrow[keytype], wnd->m_ismysql41);
            }
			else 
            {
				key.SetAs("(NULL)");
            }
            
			//Key icon to represent PK
			if(obj == OBJECT_INDEX && key.CompareI("PRIMARY") == 0)
			{
				pkimgpath.SetAs("res:PK.PNG");
				htmlbuffer->AddSprintf("<td><img src=\"%s\"/></td>", pkimgpath.GetString());	
			}
			else
            {
				htmlbuffer->Add("<td></td>");								
            }
		}
		
		if(obj == OBJECT_TABLES)
		{
			datalen = 0;
			indexlen = 0;
		}

		/*Showing the selected columns.
		parse the selected column index buffer and fetches the column from result set
		*/
		while(tok)
		{
			index = atoi(tok);

			//For 'Database' the column 'Rows' should be replace '(NULL)' by &nbsp;
			if(isnumericdata == wyTrue && obj == OBJECT_DATABASE && (!myrow || !myrow[index]))
            {
				data.SetAs("&nbsp;");
            }
			else if (!myrow || !myrow[index])
            {
				data.SetAs("(NULL)");
            }
			else
			{
				data.SetAs(myrow[index], wnd->m_ismysql41);
                
				//Convert the 'Rows' & 'Avg_Row_Lengt' numeric values to Format(T, G, M, K)
                // stricmp(tok, "4") added to fix the issue with display of no. of rows. From now we will show no. of rows 
                // as   it is 
				if(isnumericdata == wyTrue && obj == OBJECT_TABLES && stricmp(tok, "0") != 0  && stricmp(tok, "4") != 0)
				{
					valueint = data.GetAsInt64();
					
					if(index == datalencolindex) //Data_Length column for for Data Size
                    {
						datalen = valueint;
                    }
					else if(index == indexlencolindex) //Index_Length column for for Index Size
                    {
						indexlen = valueint;
                    }

					if(valueint)
					{
						if(data.GetLength())
                        {
							data.Clear();
                        }

						//Add the unit to numeric value(T, G, M, K)
						StorageUnits(valueint, 2, &data);
					}
				}

				//Checking the column is to Format into HTML or not
				else if(index == formatcolindex)
				{
					if(obj == OBJECT_VIEWDDL)
                    {
						FormatCreateViewStatement(&data, objectname, wyTrue);
                    }                    
					else
                    {
						FormatHtmlModeSQLStatement(&data, ismorewidth);				
                    }
				}
			}

			if(obj == OBJECT_TABLEDDL && ismorewidth == wyTrue)			
            {
				htmlbuffer->AddSprintf("<td width = \"%d\" class=\"cellstyleleft\"><div style=\"white-space:nowrap; overflow:auto;\">%s</div></td>", m_pagewidth - 50, data.GetString());
            }
			else
            {
                //give a link for only tables in DB level;
                if(index == 0 && obj == OBJECT_TABLES)
                {
                    tablelink.Sprintf("<a href=\"%s\" target=\"_blank\">%s</a>", data.GetString(), data.GetString());
                    htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">%s</td>", tablelink.GetString());
                }
                else
                {
                    htmlbuffer->AddSprintf("<td class=\"cellstyleleft\">%s</td>", data.GetString());
                }
            }

			tok = strtok(NULL, ",");
		}	

		/*Add column 'Total size' for Tables section
		Total size = Data_length + Index_length
		*/
		if(obj == OBJECT_TABLES)
		{
			totallen = datalen + indexlen;

			//Add the 'Total Size' coulmn to HTML table
			AddTableTotalDataSize(totallen);			
		}

#ifndef COMMUNITY
        //add the additional column in DB level for tables
        if(m_showredundantindex == wyTrue && (obj == OBJECT_TABLES || obj == OBJECT_DATABASE))
        {
            //if the it has redundant indexes, use redundant index style
            if(isredundant)
            {
                htmlbuffer->AddSprintf("<td class=\"redindexcolstyle\">%s</td>", redundantindexes.GetString());
            }
            else
            {
                htmlbuffer->AddSprintf("<td class=\"optimizecolstyle\">%s</td>", redundantindexes.GetString());
            }
        }
#endif

		htmlbuffer->Add("</tr>");

		if(dup)
        {
			free(dup);
        }

		rowcount++;
	}
}

/*
Optimized Schema contructed from o/p of PROCEDURE ANALYSE() 's 'Optimal_fieldtype' column
*/
void		
ObjectInfo::FrameOptimizedSchemaStatement(MYSQL_ROW myrowanalyse, wyInt32 indexoptimize, wyBool ismysql41, 
										 wyChar* fldname, wyString* htmlbuffer, wyBool isautoincr)
{
#ifndef COMMUNITY
	wyString optimizedata;
	wyBool ismorewidth = wyFalse;
	
	if(!myrowanalyse || !myrowanalyse[indexoptimize])
    {
		optimizedata.SetAs("(NULL)");
    }   
	else
    {
		optimizedata.SetAs(myrowanalyse[indexoptimize], ismysql41);
    }

	if(optimizedata.FindI("enum", 0) != -1)
	{
		FormatHtmlModeSQLStatement(&optimizedata, ismorewidth);
	}

	htmlbuffer->AddSprintf("<td class=\"optimizecolstyle\">%s</td>", optimizedata.GetString());
	
	if(m_optimizedschema.GetLength() == 0)
    {
		m_optimizedschema.Sprintf("ALTER TABLE `%s`.`%s` ", m_lastseldb.GetString(), m_lastselobj.GetString());
    }

	m_optimizedschema.AddSprintf("\r\n CHANGE `%s` `%s` %s", fldname, fldname, optimizedata.GetString());

	//Add AUTO_INCREMEMT 
	if(isautoincr == wyTrue)
    {
		m_optimizedschema.Add(" AUTO_INCREMENT, ");
    }
	else
    {
		m_optimizedschema.Add(", ");	
    }
#endif
}

//adding all coulmn(s) of MySQL result to Info tab
void
ObjectInfo::AddHtmlAllMySQLColumns(MDIWindow *wnd, MYSQL_RES *myres, OBJECT obj)
{
	wyInt32         fldcount = 0, count , formatcolindex = -1, rowcount = 0;
	wyBool          ismysql41 = wnd->m_ismysql41, ismorewidth = wyFalse;
	MYSQL_FIELD*    myfield = NULL;
	MYSQL_ROW       myrow;
	wyString        data;

	fldcount = myres->field_count;
	myfield = wnd->m_tunnel->mysql_fetch_fields(myres);
			
	for(count = 0; count < fldcount; count++)
	{		
		if(!myfield || !myfield[count].name)
        {
			continue;
        }

		data.SetAs(myfield[count].name, ismysql41);

		//If select 'Trigger' folder then the column 'Statement' to format
		if(obj == OBJECT_TRIGGERS && !data.CompareI("Statement"))
        {
			formatcolindex = count;
        }
				
		m_htmlformatstr.AddSprintf("<th class = \"colcaptionstyleleft\">%s</th>", data.GetString());			
	}

	m_htmlformatstr.Add("</tr>");
    
	//shows the column data
	while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
	{
		if(rowcount % 2)
        {
			m_htmlformatstr.Add("<tr class=\"datafontstylerowodd\">");
        }
		else
        {
			m_htmlformatstr.Add("<tr class=\"datafontstyleroweven\">");
        }

		//Showing the all columns
		for(count = 0; count < fldcount; count++)
		{
			if(myrow && myrow[count])
			{
				data.SetAs(myrow[count], ismysql41);

				//This specially for formatting 'Statement' column of select the 'Trigger' folder
				if(count == formatcolindex)
                {
					FormatHtmlModeSQLStatement(&data, ismorewidth);
                }
			}	
			else
            {
				data.SetAs("(NULL)");			
            }

			m_htmlformatstr.AddSprintf("<td class=\"cellstyleleft\">%s</td>", data.GetString());
		}

		m_htmlformatstr.Add("</tr>");		
		rowcount++;
	}
}

/*add column 'Total size' for Tables section
Total size = Data_length + Index_length
*/
void
ObjectInfo::AddTableTotalDataSize(wyInt64 &totallen)
{
	wyString data;

	if(totallen)
    {
		StorageUnits(totallen, 2, &data);
    }	
	else
    {
		data.SetAs("0");
    }

    m_htmlformatstr.AddSprintf("<td class=\"cellstyleleft\">%s</td>", data.GetString());			
}

//initialise the HTML buffer with styles
void
ObjectInfo::HtmlBufferInit(wyString *htmlbuffer)
{	
	RECT recttab;

	GetClientRect(m_hwndhtmleditor, &recttab);
	m_pagewidth = recttab.right - 30;

	htmlbuffer->SetAs("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;\
						  charset=UTF-8\"/></head><body>");
	htmlbuffer->Add("<style type=\"text/css\">");
	htmlbuffer->Add(CSS_INFOTAB_CLASSES);

	if(m_istableanalyse == wyTrue)
    {
		htmlbuffer->Add(CSS_OPTIMIZE_CLASS);
    }

	htmlbuffer->Add("</style>");
	return;
}

//converts time in Seconds to Better format
void
ObjectInfo::TimeUnits(wyUInt32 pcounter, wyString* timebuff)
{	
	wyUInt32  seconds = pcounter, years, days, hours, minutes;
	
	years = seconds / 31536000;
	seconds -=(years * 31536000);

	days = seconds / 86400;
	seconds -=(days * 86400);

	hours = seconds / 3600;
	seconds -= hours * 3600;

	minutes = seconds / 60;
	seconds -=(minutes * 60);

	if(years)
	{
		timebuff->Sprintf("%d", years);	    
		years > 1 ? timebuff->Add(" yrs ") : timebuff->Add(" yr ");
    }
	    
    if(days)
    {
		timebuff->AddSprintf("%d", days);
		days > 1 ? timebuff->Add(" days ") : timebuff->Add(" day ");	    
    }
	    
    if(hours)
    {
		timebuff->AddSprintf("%d", hours);
		hours > 1 ? timebuff->Add(" hrs ") : timebuff->Add(" hr ");	   
    }
	    
    if(minutes)
    {
		timebuff->AddSprintf("%d", minutes);
		minutes > 1 ? timebuff->Add(" mins ") : timebuff->Add(" min ");	    
    }
	    
    if(seconds)
    {
		timebuff->AddSprintf("%d", seconds);
		seconds > 1 ? timebuff->Add(" secs ") : timebuff->Add(" sec ");	    
    }	
}



//convert Numeric value to Better format(K, M, G, T)
void
ObjectInfo::StorageUnits(wyInt64& pBytes, wyInt32 pDecimals, wyString* formattedvalue)
{		
	wyInt64     temptera = 0, tb = 0, gb = 0, mb = 0, kb = 0;
	wyDouble    ret = 0, temp = 0;
			
	if(!formattedvalue)
    {
		return;
    }

	if(!pDecimals)
    {
		pDecimals = 0;
    }
    
	if(!pBytes)
	{
		formattedvalue->SetAs("0");
		return;
	}

	//Terra(T)
	tb = (pBytes /(ONE_KB * ONE_KB)) / (ONE_KB * ONE_KB) ;
		
    if(tb > 0)
	{
		temptera = ONE_KB * ONE_KB;
		pBytes -= (tb * ONE_KB * ONE_KB) * temptera; 		
		temp = (pBytes /double(ONE_KB * ONE_KB)) / (double(ONE_KB * ONE_KB));
		ret = (tb + temp);
		FormatStoarageUnit(&ret, tb, formattedvalue, "T");
		return;
	}
		
	//Giga(G)	
	gb = (pBytes /(ONE_KB * ONE_KB * ONE_KB));
			
	if(gb > 0)
	{
		pBytes -=(gb * ONE_KB * ONE_KB * ONE_KB);
		temp = pBytes /double(ONE_KB * ONE_KB * ONE_KB);
		ret = (gb + temp);
		FormatStoarageUnit(&ret, gb, formattedvalue, "G");
		return;
	}
	
	//Mega(M)
	mb = (pBytes /(ONE_KB * ONE_KB));
	
	if(mb > 0)
	{
		pBytes -=(mb * ONE_KB * ONE_KB);
		temp = pBytes / double(ONE_KB * ONE_KB);
		ret = (mb + temp);
		FormatStoarageUnit(&ret, mb, formattedvalue, "M");
		return ;
	}

	//Kilo(K)
	kb = (pBytes / ONE_KB);
	
	if(kb > 0)
	{
		pBytes -=(kb * ONE_KB);
		temp = pBytes / double(ONE_KB);
        ret = (kb +  temp);
		FormatStoarageUnit(&ret, kb, formattedvalue, "K");
		return;
	}

	formattedvalue->Sprintf("%d", pBytes); 	
}

//append the unit with the formatted storage value
void
ObjectInfo::FormatStoarageUnit(double* ret, wyInt32 value, wyString* formattedvalue, wyChar* unit)
{
	if(!ret || !formattedvalue || !unit)
    {
		return;
    }

	formattedvalue->Sprintf("%0.2f", *ret);

	if(value == formattedvalue->GetAsDouble(NULL)) 
    {
		formattedvalue->Sprintf("%d%s", value, unit);
    }
	else
    {
		formattedvalue->AddSprintf("%s", unit);
    }
}

void
ObjectInfo::SetSelectedResultFormatOption()
{
	wyBool	ishtmlformat = wyFalse;
	wyInt32	statushtml, statustext;

	/*check whether Info tab already selected or not, if Yes return it, 
	If Not set the Format option as per .ini file value
	*/
	if(m_isinfotabalredyselected == wyTrue)
		return;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	m_isinfotabalredyselected = wyTrue;

	ishtmlformat = IsInfoTabHTMLFormat();

	statushtml = (ishtmlformat == wyTrue) ? BST_CHECKED : BST_UNCHECKED;
	statustext = (ishtmlformat == wyTrue) ? BST_UNCHECKED : BST_CHECKED;
    
	SendMessage(m_hwndopthtml, BM_SETCHECK, (WPARAM)statushtml, 0);
	SendMessage(m_hwndopttext, BM_SETCHECK, (WPARAM)statustext, 0);	

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return;
}

//adds the MySQL result set 'selected' columns to scintilla.
void
ObjectInfo::FormatTextModeSQLStatement(MDIWindow* wnd, const wyChar* objectname, MYSQL_RES** res, wyString* temptitle, wyChar* column, OBJECT obj)
{
	wyInt32		fldindex, colindex, strtpos = 0, endpos = 0;
	MYSQL_ROW	myrow;
	wyString	data, strindex, fieldcaption, strcolumn, columnname, result;
	wyBool		ismysql41 = wnd->m_ismysql41;
	wyChar		*tok = NULL, *dup = NULL;
	wyInt32		colcount = 0;

	strcolumn.SetAs(column);
    dup = strdup(column);
	tok = strtok(dup, ",");

	/*gets the index of the 'coulmns' to display, 
	those column names are passed to buffer with comma delemeter*/
	while(tok)
	{
		fldindex = GetFieldIndex(wnd->m_tunnel, *res, tok); 
		
		//if particluar column not present in Result set, just remove it form buffer
		if(fldindex == -1)
		{
			strtpos = strcolumn.FindI(tok, 0);

			if(strtpos != -1)
            {
				strcolumn.Replace(strtpos, strlen(tok) + 1, "");
            }

			tok = strtok(NULL, ",");
			continue;
		}

		colcount++;

		//add the coulmn index to bufffer with delimeter comma
		strindex.AddSprintf("%d,", fldindex);
        tok = strtok(NULL, ",");
	}

	if(dup)
    {
		free(dup);
    }

	result.SetAs(*temptitle);
    strtpos = 0;
	tok = strtok((wyChar*)strindex.GetString(), ",");

	//gets the column data corresponding to the Index(s) in index buffer
	while(myrow = wnd->m_tunnel->mysql_fetch_row(*res))
	{
		while(tok)
		{
			colindex = atoi(tok);

			if(myrow[colindex])
            {
				data.SetAs(myrow[colindex], ismysql41);				   
            }
			else
            {
				data.SetAs("(NULL)");				   
            }
			
			data.FindAndReplace("\n","\r\n");

			//If more than 1 column to display then adding 'column caption' also
			if(colcount > 1)
			{				
				endpos = strcolumn.FindChar(',', strtpos);
				
				if(endpos == -1 || endpos <= strtpos)
                {
					break;
                }

				columnname.SetAs(strcolumn.Substr(strtpos, endpos - strtpos));
				fieldcaption.Sprintf("%s:\r\n", columnname.GetString());
				AppendLine(fieldcaption);
				result.Add(fieldcaption.GetString());
			}
				
			result.Add(data.GetString());
			
			//format the 'Create View' statement
			if(obj == OBJECT_VIEWDDL)
            {
				FormatCreateViewStatement(&result, objectname, wyFalse);
            }
			
			result.Add("\r\n\r\n");	
			tok = strtok(NULL, ",");
			strtpos = endpos + 1;
		}
	}

	//appending buffoer to scintilla editor
	SendMessage(m_hwnd, SCI_APPENDTEXT, result.GetLength(),(LPARAM)result.GetString());// no ret	
}

void
ObjectInfo::FreeTableLevelResultsets()
{
	if(m_myrescolumninfo)
    {
		m_wnd->m_tunnel->mysql_free_result(m_myrescolumninfo);
    }

	if(m_myrestableindexinfo)
    {
		m_wnd->m_tunnel->mysql_free_result(m_myrestableindexinfo);
    }

	if(m_myrestableddlinfo)
    {
		m_wnd->m_tunnel->mysql_free_result(m_myrestableddlinfo);
    }

	m_myrescolumninfo = NULL;
	m_myrestableindexinfo = NULL;
	m_myrestableddlinfo = NULL;
}

void
ObjectInfo::AddTableOptimizeTableSchema()
{
	wyInt32     pagewidth, len;
	RECT	    rcthtml;
	wyString    htmlformatstr;
	wyBool	    ismorewidth = wyFalse;

	htmlformatstr.AddSprintf("<br/><div class=\"resultcaptionstyle\"><b>");
	htmlformatstr.Add("Alter Table With Optimal Datatypes");
	htmlformatstr.Add("</b></div><br/>");	
	htmlformatstr.Add("<div>");

	GetClientRect(m_hwndhtmleditor, &rcthtml);
	pagewidth = rcthtml.right;

	htmlformatstr.Add("<table class=\"statustablestyle\">");
	htmlformatstr.Add("<tbody>");
    htmlformatstr.Add("<tr class=\"captionfontstyle\">");
    htmlformatstr.Add("<td class=\"colcaptionstyleleft\">Alter Table</td>");
    htmlformatstr.Add("<tr class=\"datafontstyleroweven\">");

	len = m_optimizedschema.GetLength();
    m_optimizedschema.Strip(2);
    
    FormatHtmlModeSQLStatement(&m_optimizedschema, ismorewidth);
    
	if(ismorewidth == wyTrue)
    {
		htmlformatstr.AddSprintf("<td width = \"%d\" class=\"optimizecolstyle\"><div style=\"white-space:nowrap; overflow:auto;\">%s<br/>&nbsp;<br/>&nbsp;</div></td>", pagewidth - 50, m_optimizedschema.GetString());
    }
	else
    {
		htmlformatstr.AddSprintf("<td class=\"optimizecolstyle\">%s</td>", m_optimizedschema.GetString());
    }

	htmlformatstr.Add("</tr>");
	htmlformatstr.Add("</tbody>");
	htmlformatstr.Add("</table>");
	htmlformatstr.Add("</div>");
	m_strtableinfo.Add(htmlformatstr.GetString());
}

//function adds alter stmts if any to the html buffer
void
ObjectInfo::AddAlterStmtForRedundantIndex(wyString* htmlbuffer)
{
    wyString alterstmt;

#ifndef COMMUNITY
    //if no alter stmt is available, then return
    if(m_redindexfinder->GenenrateAlterStmt(alterstmt) == wyFalse)
    {
        return;
    }

    //add the caption
	htmlbuffer->Add("<br/><div class=\"resultcaptionstyle\"><b>");
	htmlbuffer->Add("Alter Table For Dropping Redundant Indexes");
	htmlbuffer->Add("</b></div><br/>");	
	
    //add the html table with the alter stmt
    htmlbuffer->Add("<div><table class=\"statustablestyle\">");
	htmlbuffer->Add("<tbody>");

	htmlbuffer->Add("<tr class=\"captionfontstyle\">");
	htmlbuffer->Add("<td class=\"colcaptionstyleleft\">Alter Table</td>");
    htmlbuffer->Add("</tr>");
	htmlbuffer->Add("<tr class=\"datafontstyleroweven\">");
    htmlbuffer->AddSprintf("<td class=\"optimizecolstyle\"><br>%s</td>", alterstmt.GetString());

	htmlbuffer->Add("</tr></tbody></table></div>");
#endif
}

//free the db level result sets
void
ObjectInfo::FreeDBLevelResultsets()
{
    if(!m_wnd)
    {
        return;
    }

    if(m_tableinfosres)
    {
		m_wnd->m_tunnel->mysql_free_result(m_tableinfosres);
    }

    if(m_viewinfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_viewinfores);
    }

    if(m_procinfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_procinfores);
    }

    if(m_funcinfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_funcinfores);
    }

    if(m_triginfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_triginfores);
    }

    if(m_eventinfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_eventinfores);
    }

    if(m_dbddlinfores)
    {
        m_wnd->m_tunnel->mysql_free_result(m_dbddlinfores);
    }

    m_tableinfosres = NULL;
    m_viewinfores = NULL;
    m_procinfores = NULL;
    m_funcinfores = NULL;
    m_triginfores = NULL;
    m_eventinfores = NULL;
    m_dbddlinfores = NULL;
}

void
ObjectInfo::Resize()
{
	RECT	rcparent;
    wyInt32 vpos = CustomTab_GetTabHeight(m_hwndparent);

	GetClientRect(m_hwndparent, &rcparent);
    MoveWindow(m_hwndframe, 0,  vpos, rcparent.right - rcparent.left, rcparent.bottom - vpos, TRUE);
}

void
ObjectInfo::OnResize()
{
	wyInt32 hpos, vpos, width, height;
	RECT	rcparent;

	GetClientRect(m_hwndframe, &rcparent);
	hpos	= 0;
    vpos	= 0;
	width	=(rcparent.right-rcparent.left);
	height	= 28;

	MoveWindow(m_hwndtoolbar, hpos, vpos, width, height, TRUE);
    MoveWindow(m_hwndoptstatic, 5, 7, 40, 15, TRUE);//Format:
    MoveWindow(m_hwndopthtml, 50, 2, 55, 25, TRUE); //Html(radio button)
    MoveWindow(m_hwndopttext, 115, 2, 90, 25, TRUE); //Text(radio button)
    MoveWindow(m_hwndrefreshinfo, 215, 4, 65, 20, TRUE); // Refresh button
		
	hpos	= 0;
	vpos	= height + vpos;
	width	= (rcparent.right-rcparent.left);
	height	= (rcparent.bottom - (rcparent.top + vpos));
	
    MoveWindow(m_hwnd, hpos, vpos, width, height, TRUE);
	MoveWindow(m_hwndhtmleditor, hpos, vpos, width, height, TRUE);
}

LRESULT CALLBACK 
ObjectInfo::FrameProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ObjectInfo* pobjinfo = (ObjectInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    wyInt32     ret;

    if(message == WM_SIZE)
    {
        pobjinfo->OnResize();
    }
    else if(message == WM_NOTIFY)
    {
        if((ret = wyTheme::DrawToolBar(lparam)) != -1)
        {
            return ret;
        }
    }

    return CallWindowProc(pobjinfo->m_origframeproc, hwnd, message, wparam, lparam);
}

void
ObjectInfo::Show(wyBool status)
{
    ShowWindow(m_hwndframe, status);
}


LRESULT CALLBACK 
ObjectInfo::EditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ObjectInfo* pobjinfo = (ObjectInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == (wyUInt32)pGlobals->m_pcmainwin->m_findmsg) 
	{		
		if(pobjinfo->m_findreplace && pobjinfo->m_findreplace->FindReplace(hwnd, lparam) == wyFalse)
		{
			delete(pobjinfo->m_findreplace);
			pobjinfo->m_findreplace = NULL;
		}

		return 0;		
	}    

    switch(message)
    {
        case WM_CONTEXTMENU:
            return pobjinfo->OnContextMenu(lparam);

        case WM_COMMAND:
            return pobjinfo->OnWmCommand(hwnd, wparam);

        case WM_HELP:
	        return 1;

        case WM_SETFOCUS:
        case WM_KEYUP:
        case WM_LBUTTONUP:
            PostMessage(pobjinfo->m_hwndparent, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
	        break;

        case WM_KILLFOCUS:
            PostMessage(pobjinfo->m_hwndparent, UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
            break;

        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, pobjinfo->m_menu);

        case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);	
    }

    return CallWindowProc(pobjinfo->m_wporigproc, hwnd, message, wparam, lparam);
}

wyInt32
ObjectInfo::OnContextMenu(LPARAM lParam)
{
	HMENU   hmenu, htrackmenu;
	POINT   pnt;
	wyInt32 pos;
    RECT    rect;

	if(lParam == -1)
	{		
		pos = SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
		pnt.x = SendMessage(m_hwnd, SCI_POINTXFROMPOSITION, 0, pos) ; 
		pnt.y = SendMessage(m_hwnd, SCI_POINTYFROMPOSITION, 0, pos); 
		ClientToScreen(m_hwnd, &pnt);
	}
	else
	{
		pnt.x   =   (LONG)LOWORD(lParam);
		pnt.y   =   (LONG)HIWORD(lParam);
	}

    GetClientRect(m_hwnd, &rect);
    MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rect, 2);

    if(!PtInRect(&rect, pnt))
    {
        return -1;
    }

	hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_INFOTABMENU));
    LocalizeMenu(hmenu);

    if(SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0) != SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0))
    {
		EnableMenuItem(hmenu, ID_OBJECT_COPY, MF_ENABLED);
    }

	htrackmenu = GetSubMenu(hmenu, 0);
	m_menu = htrackmenu;

    wyTheme::SetMenuItemOwnerDraw(m_menu);
	TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, m_hwnd , NULL);

	FreeMenuOwnerDrawItem(m_menu);
	DestroyMenu(hmenu);
	return 1;
}

wyInt32
ObjectInfo::OnWmCommand(HWND hwnd, WPARAM wparam)
{
	switch(LOWORD(wparam))
	{
		case ID_OBJECT_COPY:
			CopyStyledTextToClipBoard(hwnd);
			break;

		case ID_OBJECT_SELECTALL:
			SendMessage(hwnd, SCI_SELECTALL, 0, 0);
			break;

		case ID_OBJECT_FIND:
            FindTextOrReplace(m_wnd, wyFalse);
			break;

		case ID_OBJECT_FINDNEXT:
			OnAccelFindNext(m_wnd);
			break;
	}

	return 0;
}

HWND
ObjectInfo::GetActiveDisplayWindow()
{
    if(IsWindowVisible(m_hwnd))
    {
        return m_hwnd;
    }
    
    if(IsWindowVisible(m_hwndhtmleditor))
    {
        return m_hwndhtmleditor;
    }

    return NULL;
}



//----------------------------------------------------------