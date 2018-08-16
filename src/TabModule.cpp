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

#include "TabModule.h"
#include "MDIWindow.h"
#include "Global.h"
#include "TabTypes.h"
#include "TabEditor.h"
#include "EditorBase.h"
#include "GUIHelper.h"
#include "EditorFont.h"
#include "TabEditorSplitter.h"
#include "AutoCompleteInterface.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabPreview.h"
#include "TabTableData.h"
#include "TableView.h"
#include "TabIndexes.h"
#include "TabForeignKeys.h"

#ifndef COMMUNITY
#include "HelperEnt.h"
#include "TabQueryBuilder.h"
#include "TabSchemaDesigner.h" 
#include "DatabaseSearch.h"
#else
#include "CommunityRibbon.h"
#endif

#include <scilexer.h>


#define IDC_CTAB		WM_USER+116
extern	PGLOBALS		pGlobals;
#define	SIZE_24	        24
#define	SIZE_12	        12

//constructor sets the window handle
TabModule::TabModule(HWND hwnd)
{
	m_hwndparent    = hwnd;

	m_hwnd          = NULL;
    m_historydata   = NULL;
    m_parentptr     = NULL;
    m_pceditorquery = NULL;
    m_pcadvedit     = NULL;
    m_historydata   = NULL;
    m_pctabqb       = NULL;
    m_pctabeditor   = NULL;
	m_pctabhistory	= NULL;
	m_pcinfotab		= NULL;
    m_isdefault     = wyFalse;
	m_hwndcommtytitle = NULL;

	m_infostatusres = NULL;
	m_infovariablesres = NULL;

	m_istabcreate = wyFalse;
	
	m_tableview = NULL;

#ifdef COMMUNITY	
	m_cribbon = NULL;
#endif	
}

TabModule::~TabModule()
{  
#ifdef COMMUNITY	
	if(m_cribbon)
		delete m_cribbon;

#endif

	if(m_hwnd)
	{
		DestroyWindow(m_hwnd);
		m_hwnd  = NULL;
	}

	if(m_hwndcommtytitle)
	{
		DestroyWindow(m_hwndcommtytitle);
		m_hwndcommtytitle  = NULL;
	}
    
    if(m_historydata)
    {
        delete[] m_historydata;
        m_historydata = NULL;
    }		

	
	if(m_infovariablesres)
	{
		m_parentptr->m_tunnel->mysql_free_result(m_infovariablesres);
		m_infovariablesres = NULL;
	}

	if(m_infostatusres)
	{
		m_parentptr->m_tunnel->mysql_free_result(m_infostatusres);
		m_infostatusres = NULL;
	}
	
	if(m_pctabhistory)
	{
		delete m_pctabhistory;
		m_pctabhistory = NULL;
	}

	if(m_tableview)
	{
		delete m_tableview;
		m_tableview = NULL;
	}
}

//function to create Editortab
wyBool
TabModule::Create(MDIWindow * wnd)
{    
    wyInt32		i,totaltabs = 0;
	tabdetailelem  *temptabdetail;
	tabeditorelem  *temptabeditorele;
    m_isdefault     = wyTrue;
	HTREEITEM	hitem = NULL;
    wyString temptest;   
    CreateTabControl();
	CreateCommunityTabHeader();
    CreateHistoryTab(wnd, wyFalse, wyFalse);
	if(!pGlobals->m_conrestore)
		CreateQueryEditorTab(wnd);    
	else
	{
		totaltabs = wnd->m_listtabdetails->GetCount();
		if(totaltabs > 0)
		{
			temptabdetail = (tabdetailelem*)wnd->m_listtabdetails->GetFirst();
			for(i = 0; i < totaltabs ; i++)
			{
			
				temptabeditorele = new tabeditorelem;
				temptabeditorele->m_ispresent = wyTrue;
				temptabeditorele->m_id = temptabdetail->m_id;
				temptabeditorele->m_tabid = temptabdetail->m_tabid;
				//temptabeditorele->m_position = temptabdetail->m_position;
				temptabeditorele->m_position = i;
				temptabeditorele->m_color = temptabdetail->m_color;
				temptabeditorele->m_fgcolor = temptabdetail->m_fgcolor;
				temptabeditorele->m_isfile = temptabdetail->m_isfile;
				temptabeditorele->m_isedited = temptabdetail->m_isedited;
				temptabeditorele->m_isfocussed = temptabdetail->m_isfocussed;
				temptabeditorele->m_leftortoppercent = temptabdetail->m_leftortoppercent;
				temptabeditorele->m_psztext.SetAs(temptabdetail->m_psztext);
				temptabeditorele->m_tooltiptext.SetAs(temptabdetail->m_tooltiptext);
				temptabeditorele->m_iimage = temptabdetail->m_iimage;
				temptabeditorele->m_tabptr = 0;
				temptabeditorele->m_content = temptabdetail->m_content;
				//if not IDI_QUERY_16 create advanced editor
				//CreateAdvEditorTab(wnd, (wyChar *)temptabeditorele->m_psztext.GetString(), temptabeditorele->m_iimage, hfunctionitem, &temptabeditorele->m_psztext);
				if(temptabdetail->m_iimage != IDI_QUERYBUILDER_16 && temptabdetail->m_iimage != IDI_SCHEMADESIGNER_16)
				{
				if(temptabdetail->m_iimage == IDI_QUERY_16 || temptabeditorele->m_isfile)
				{
					CreateQueryEditorTab(wnd);
					if(!temptabeditorele->m_isfile)
					{
						SetTabRename(temptabdetail->m_psztext.GetAsWideChar());
					}
				}
				else
				{
					CreateAdvEditorTab(wnd, (wyChar *)temptabeditorele->m_psztext.GetString(), temptabdetail->m_iimage, hitem, &temptabeditorele->m_psztext);
				}
					temptabeditorele->m_tabptr = (wyInt64)m_pctabeditor;
				temptabeditorele->m_pctabeditor = m_pctabeditor;
				m_pctabeditor->m_pcetsplitter->SetLeftTopPercent(temptabeditorele->m_leftortoppercent);
				}
				else
				if(temptabdetail->m_iimage == IDI_QUERYBUILDER_16)
				{
					//create querybuildertab
#ifndef COMMUNITY
					CreateQueryBuilderTab(wnd);
					temptabeditorele->m_tabptr = (wyInt64)m_pctabqb;
#endif
				}
				else
				{
					//create querybuildertab
#ifndef COMMUNITY
					CreateSchemaDesigner(wnd);
					temptabeditorele->m_tabptr = (wyInt64)m_pctabsd;
#endif
				}
				if(temptabeditorele->m_tabptr != 0)
				wnd->m_listtabeditor->Insert(temptabeditorele);
				temptabdetail = (tabdetailelem*)temptabdetail->m_next;
			 
			}
		}
		else
			CreateQueryEditorTab(wnd);    
	}
    if(pGlobals->m_istabledataunderquery == wyFalse &&
        GetTabOpenPersistence(IDI_TABLE) == wyTrue)
    {
        CreateTabDataTab(wnd);
    }

    if(pGlobals->m_isinfotabunderquery == wyFalse && 
        GetTabOpenPersistence(IDI_TABLEINDEX) == wyTrue)
	{
		CreateInfoTab(wnd);
	}

    if(pGlobals->m_ishistoryunderquery == wyFalse)
    {
        CreateHistoryTab(wnd, GetTabOpenPersistence(IDI_HISTORY), wyFalse);
    }

	m_isdefault = wyFalse;
    return wyTrue;
}

//Community title window
wyBool				
TabModule::CreateCommunityTabHeader()
{
#ifdef COMMUNITY
	m_cribbon = new CommunityRibbon();
	if(m_cribbon->CreateRibbon(m_hwndparent) == wyTrue)
		m_hwndcommtytitle = m_cribbon->m_hwnd;

	else
		return wyFalse;
#endif
	return wyTrue;
}

//Function to o create the TabControl & initialise the  'm_hwnd'
void
TabModule::CreateTabControl()
{
	HWND		    hwndtabctrl;
    TABCOLORINFO    ci = {0};

	VERIFY(hwndtabctrl = CreateCustomTab(m_hwndparent, 0, 0, 0, 0, 
										  TabWndProc, (LPARAM)IDC_CTAB));
	CustomTab_EnableDrag(hwndtabctrl, pGlobals->m_pcmainwin->m_hwndmain, wyTrue);
    CustomTab_EnableAddButton(hwndtabctrl, wyTrue);
    CustomTab_SetClosable(hwndtabctrl, wyTrue, 1);

    if(wyTheme::GetTabColors(COLORS_QUERYTAB, &ci))
    {
        CustomTab_SetColorInfo(hwndtabctrl, &ci);
    }

	MoveWindow(hwndtabctrl, 0, 0, 0, 0, TRUE);

	m_hwnd = hwndtabctrl;
	//UpdateWindow(m_hwnd);
    ShowWindow(m_hwnd, SW_HIDE);
    CustomTab_SetIconList(m_hwnd, pGlobals->m_hiconlist);

	return ;
}

LRESULT CALLBACK 
TabModule::TabWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool* pishandled)
{
    MDIWindow*      pmdi = (MDIWindow*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
    TabEditor*      ptabeditor;
	CTCITEM	        item = {0};
    wyInt32         seltab, ret;
    TABCOLORINFO    ci;
    COLORREF        clrref;

    *pishandled = wyTrue;

    if((seltab = CustomTab_GetCurSel(hwnd)) != -1)
    {
        item.m_mask = CTBIF_LPARAM | CTBIF_IMAGE;
        CustomTab_GetItem(hwnd, seltab, &item);
    }

    switch(message)
    {
        case WM_NOTIFY:
            if((ret = wyTheme::DrawToolBar(lparam)) != -1)
            {
                return ret;
            }

            if(((LPNMHDR)lparam)->idFrom == IDC_QUERYTAB)
		    {
			    return SendMessage(GetParent(hwnd), WM_NOTIFY, NULL, lparam);
		    }

            if(((LPNMHDR)lparam)->code == NM_GETCOLORS)
            {
                CustomTab_GetColorInfo(hwnd, &ci);
                ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color1 = ci.m_tabtext;
                ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color2 = ci.m_tabbg1;
                return 0;
            }
            else if(((LPNMHDR)lparam)->code == NM_GETHYPERLINKCOLOR)
            {
                CustomTab_GetColorInfo(hwnd, &ci);

                if(ci.m_mask & CTCF_LINK)
                {
                    ((LPNMDVCOLORINFO)lparam)->drawcolor.m_color1 = ci.m_linkcolor;
                }

                return 0;
            }

            if(pmdi && (ptabeditor = pmdi->GetActiveTabEditor()) && ptabeditor->m_peditorbase)
            {
                if(((LPNMHDR)lparam)->hwndFrom == ptabeditor->m_peditorbase->m_hwnd)
                {
                    if(pmdi && pmdi->m_acinterface->OnACNotification(wparam, lparam))
                    {
                        return 1;
                    }            
                    else if(OnScintillaNotification(wparam, lparam, wyTrue))
			        {
                        return 1;
                    }
			    }
            }
			break;

        case UM_SETROWCOUNT:
            pmdi->m_pcquerystatus->AddNumRows(wparam, lparam ? wyTrue : wyFalse);
            return 1;

        case UM_STARTEXECUTION:
            CustomTab_SetClosable(hwnd, wyTrue, CustomTab_GetItemCount(hwnd));
            CustomTab_StartIconAnimation(hwnd, CustomTab_GetCurSel(hwnd));
            SendMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_UPDATEMAINTOOLBAR, WPARAM(0), LPARAM(0)); 
            EnableWindow(pmdi->m_pcqueryobject->m_hwnd, FALSE);
            return 1;

        case UM_ENDEXECUTION:
            CustomTab_SetClosable(hwnd, wyTrue, 1);
            SendMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_UPDATEMAINTOOLBAR, WPARAM(1), LPARAM(0)); 
            EnableWindow(pmdi->m_pcqueryobject->m_hwnd, TRUE);
            CustomTab_StopIconAnimation(hwnd, CustomTab_GetCurSel(hwnd));
            SendMessage(hwnd, UM_SETROWCOUNT, wparam, 0);

            if(lparam)
            {
                SetFocus((HWND)lparam);
            }

            if(item.m_iimage == IDI_TABLE)
            {
                ((TabTableData*)item.m_lparam)->m_isrefreshed = wyTrue;
            }

            return 1;

        case UM_SETSTATUSLINECOL:
            pmdi->m_pcquerystatus->AddLineColNum((HWND)wparam, (wyBool)lparam);
            return 1;

        case WM_CTLCOLORSTATIC:
            if(pmdi && (ptabeditor = pmdi->GetActiveTabEditor()) && 
                ptabeditor->m_peditorbase && ptabeditor->m_peditorbase->m_hwndhelp && (HWND)lparam)
            {
                clrref = SendMessage(ptabeditor->m_peditorbase->m_hwnd, SCI_STYLEGETFORE, STYLE_LINENUMBER, 0);
                SetTextColor((HDC)wparam, clrref);
                SetBkMode((HDC)wparam, TRANSPARENT);
                clrref = SendMessage(ptabeditor->m_peditorbase->m_hwnd, SCI_STYLEGETBACK, STYLE_LINENUMBER, 0);
                SetDCBrushColor((HDC)wparam, clrref);
                return (LRESULT)GetStockObject(DC_BRUSH);
            }

            break;
    }

    *pishandled = wyFalse;
	return 0;
}

// Function creating the normal QueryEditor
wyBool	
TabModule::CreateQueryEditorTab(MDIWindow* wnd, wyInt32 pos, wyBool setfocus)
{
	wyInt32				ret, count;
    wyBool              rstatus;
	CTCITEM				item = {0};

	m_istabcreate = wyTrue;
	
	item.m_psztext    = _("Query");
	item.m_cchtextmax = strlen(_("Query"));
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU | CTBIF_TOOLTIP;
	item.m_iimage     = IDI_QUERY_16;
	item.m_tooltiptext = _("Query");
	
	
	m_pctabeditor = CreateTabEditor(wnd);
	m_pctabeditor->Create(wnd, NULL, wyTrue);
	
	item.m_lparam = (LPARAM)m_pctabeditor;

	ShowWindow(m_pctabeditor->m_peditorbase->m_hwnd, FALSE);

	count = pos == -1 ? CustomTab_GetItemCount(m_hwnd) : pos;
	VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);

	count = CustomTab_GetItemCount(m_hwnd);

	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);

	if(count > 1 && setfocus == wyTrue)
    {		
        CustomTab_SetCurSel(m_hwnd, pos == -1 ? count - 1 : pos);
		rstatus = CustomTab_EnsureVisible(m_hwnd, pos == -1 ? count - 1 : pos, wyFalse);
				
        if(rstatus == wyFalse)
            CustomTab_EnsureVisible(m_hwnd, 0, wyFalse);
    }

	Resize();

	//Add tool tip
	if(count == 1)
	{
		CreateCustomTabTooltip(m_hwnd);
	}

	m_istabcreate = wyFalse;
	return wyTrue;
}


wyBool
TabModule::CreateAdvEditorTab(MDIWindow *wnd, wyChar* title, wyInt32 image, HTREEITEM hitem, wyString* strhitemname)
{
	wyInt32				ret, count;
	CTCITEM				item = {0};
	wyString			objectname, tempfilename;

	objectname.SetAs(title);

	 //truncate object name if length is greater than 24
	//take 1st 12 characters .... and last 12 characters
	if(objectname.GetLength() >  SIZE_24)
	{
		tempfilename.SetAs(objectname);

		//take 1st 12 characters
		tempfilename.Strip(objectname.GetLength() - SIZE_12);
		
		tempfilename.Add("...");

		//take last 12 characters
		tempfilename.Add(objectname.Substr((objectname.GetLength() - SIZE_12), SIZE_12));

		objectname.SetAs(tempfilename);
	}

	item.m_psztext    = (wyChar*)objectname.GetString();
	item.m_cchtextmax = objectname.GetLength();
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	item.m_iimage     = image;
	item.m_tooltiptext = title;
	
	

	m_istabcreate = wyTrue;

	m_pctabeditor = CreateTabEditor(wnd);		// sets the current 'TabEditor' pointer
	m_pctabeditor->Create(wnd, hitem, wyFalse, strhitemname);	// to create EditorProc

	item.m_lparam = (LPARAM)m_pctabeditor;

	ShowWindow(m_pctabeditor->m_hwnd, FALSE);

	count = CustomTab_GetItemCount(m_hwnd);
    //SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);
	VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);
	count = CustomTab_GetItemCount(m_hwnd);
	
	if(count > 1)	
    {
        CustomTab_SetCurSel(m_hwnd, count - 1);
	    CustomTab_EnsureVisible(m_hwnd, count - 1, wyFalse);
    }

	//ShowWindow(m_hwnd, TRUE);			

	Resize();
	m_istabcreate = wyFalse;
		
	return wyTrue;

}

//Function to create a new TabEditor tab
TabEditor *
TabModule::CreateTabEditor(MDIWindow * wnd)
{
	TabEditor		*pctabeditor;

	pctabeditor  =	new TabEditor(m_hwnd);

	pctabeditor->SetParentPtr(this);              // to set the tabmodule pointer
		
	return pctabeditor;
}

wyBool
TabModule::CreateQueryBuilderTab(MDIWindow * wnd)
{
#ifndef COMMUNITY
	wyInt32				ret, count = 0;
    wyBool              rstatus;
	CTCITEM				item = {0};
	
	// get the number of tabs
    count = CustomTab_GetItemCount(m_hwnd);

    item.m_psztext    = _("Query Builder");
	item.m_cchtextmax = strlen(_("Query Builder"));
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	item.m_iimage     = IDI_QUERYBUILDER_16;
	m_pctabqb		  = CreateTabQB(wnd);
	item.m_tooltiptext = _("Query Builder");
	
	
	if(!m_pctabqb)
		return wyFalse;
			
	if(m_pctabqb->Create(wnd) == wyFalse)
	{
		delete m_pctabqb;
		return wyFalse;
	}

	m_istabcreate = wyTrue;
	
	item.m_lparam     = (LPARAM)m_pctabqb;

	// create new tab
    VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);
	
    count = CustomTab_GetItemCount(m_hwnd);

	
	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);
		
    //Checks whether the QB is creating by default or by explicitly by keyboard or menu
    if(count > 1 && m_isdefault == wyFalse)
    {		
        CustomTab_SetCurSel(m_hwnd, count - 1);
		rstatus = CustomTab_EnsureVisible(m_hwnd, count - 1, wyFalse);
				
        if(rstatus == wyFalse)
            CustomTab_EnsureVisible(m_hwnd, 0, wyFalse);
    }

    //ShowWindow(m_hwnd , TRUE);		

	Resize();
	m_istabcreate = wyFalse;

#endif
    return wyTrue;
}
#ifndef COMMUNITY
// Function to instantiate the TabQueryBuilder
TabQueryBuilder*
TabModule::CreateTabQB(MDIWindow * wnd)
{
	TabQueryBuilder *ptabqb = NULL;

	ptabqb = new TabQueryBuilder(m_hwnd);
	ptabqb->SetParentPtr(this); 
	
	return ptabqb;
}
#endif

wyBool
TabModule::IsValidFocusInOB(wyInt32 subtabindex)
{
    wyBool  ret = wyTrue;
    MDIWindow*  wnd = GetActiveWin();
    wyInt32     image = -1;

    image = wnd->m_pcqueryobject->GetSelectionImage();

    switch(image)
    {
    case NFOLDER:
        {
            HTREEITEM   hitem;
            wyWChar     data[SIZE_64 + 1];
            wyString    foldertext;

            hitem = TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd);
            GetNodeText(wnd->m_pcqueryobject->m_hwnd, hitem, data, SIZE_64);

            foldertext.SetAs(data);

            if(foldertext.Compare("Columns") == 0)
            {
                //if(subtabindex != 0 && subtabindex != -1)
                ret = wyTrue;
            }
            else if(foldertext.Compare("Indexes") == 0)
            {
                //if(subtabindex != 1)
                ret = wyTrue;
            }
            else
                ret = wyFalse;
        }
        break;

    case NTABLE:
    case NINDEX:
    case NPRIMARYINDEX:
        ret = wyTrue;
        break;
        
    case NCOLUMN:
    case NPRIMARYKEY:
        if(subtabindex != 0 && subtabindex != -1)
            ret = wyFalse;
        break;

    default:
        ret = wyFalse;
    }

    return ret;
}

wyBool
TabModule::CreateTableTabInterface(MDIWindow *wnd, wyBool isaltertable, wyInt32 setfocustotab)
{
    wyInt32             ret, count = 0, tabindex = -1;
    wyBool              rstatus,retval;
    CTCITEM             item = {0};
    TableTabInterface   *ptabinterface = NULL;
    wyString            tabtitle, objectname, temptabname;
 
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return wyTrue;
	}
#endif
    if(isaltertable)
    {
        if(!IsValidFocusInOB(setfocustotab))
            return wyFalse;

        if(wnd->m_pcqueryobject->m_seltable.GetLength())
            tabtitle.SetAs(wnd->m_pcqueryobject->m_seltable);

        if(IsAlterTableTabOpen(tabtitle, tabindex))
        {
            if(CustomTab_GetCurSel(m_hwnd) != tabindex)
            {
                CustomTab_SetCurSel(m_hwnd, tabindex);
                CustomTab_EnsureVisible(m_hwnd, tabindex);
            }
            

            CTCITEM             tmpitem = {0};
            tmpitem.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;

            CustomTab_GetItem(m_hwnd, tabindex, &tmpitem);

            TableTabInterface* tmptabint =  NULL;
            tmptabint = (TableTabInterface*) tmpitem.m_lparam;
            /*
            if(tmptabint->m_ptabintmgmt->GetActiveTabImage() == IDI_COLUMN && (setfocustotab == TABCOLUMNS))
                return wyTrue;

            if(tmptabint->m_ptabintmgmt->GetActiveTabImage() == IDI_TABIMG_INDEXES && (setfocustotab == TABINDEXES))
                return wyTrue;

            if(tmptabint->m_ptabintmgmt->GetActiveTabImage() == IDI_TABIMG_FOREIGNKEYS && (setfocustotab == TABFK))
                return wyTrue;

            if(tmptabint->m_ptabintmgmt->GetActiveTabImage() == IDI_TABLEOPTIONS && (setfocustotab == TABADVANCED))
                return wyTrue;
            */
            /*
            if(setfocustotab == -1)
                setfocustotab = 0;
            */
            if(setfocustotab != -1 && CustomTab_GetCurSel(tmptabint->m_ptabintmgmt->m_hwnd) != setfocustotab)
                tmptabint->m_ptabintmgmt->SelectTab(setfocustotab);

            tmptabint->SetInitFocus();
            return wyTrue;
        }
    }
    else
        tabtitle.SetAs(_("New Table"));

    objectname.SetAs(tabtitle);

    if(objectname.GetLength() >  SIZE_24)
	{
		temptabname.SetAs(objectname);

		//take 1st 12 characters
		temptabname.Strip(objectname.GetLength() - SIZE_12);
		
		temptabname.Add("...");

		//take last 12 characters
		temptabname.Add(objectname.Substr((objectname.GetLength() - SIZE_12), SIZE_12));

		objectname.SetAs(temptabname);
	}
    
    wyString    tooltipstr;

    if(isaltertable)
    {
        wyString    dbstr, tblstr;
        dbstr.SetAs(wnd->m_pcqueryobject->m_seldatabase);
        tblstr.SetAs(wnd->m_pcqueryobject->m_seltable);

        dbstr.FindAndReplace("`", "``");
        tblstr.FindAndReplace("`", "``");

        tooltipstr.Sprintf("`%s`.`%s`", dbstr.GetString(), tblstr.GetString());
    }
    else
    {
        tooltipstr.SetAs(_("New Table"));
    }

    count = CustomTab_GetItemCount(m_hwnd);
    m_istabcreate = wyTrue;
    item.m_tooltiptext = (wyChar*)tooltipstr.GetString(); //_("New Table");
    //tabtitle.SetAs(objectname);
    item.m_psztext = (wyChar*)objectname.GetString(); //_("New Table");
    item.m_cchtextmax = tabtitle.GetLength();
    item.m_mask = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_TOOLTIP;
    if(isaltertable)
        item.m_iimage = IDI_ALTERTABLE;
    else
        item.m_iimage = IDI_CREATETABLE;
    
	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);
    //ptabinterface = new TableTabInterface(m_hwnd);
    
	ptabinterface = new TableTabInterface(m_hwnd, wyFalse, isaltertable, setfocustotab);
    ptabinterface->SetParentPtr(this);
    if(ptabinterface)
    {
        retval = ptabinterface->Create();
        if(retval == wyFalse)
        {
            SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
            delete ptabinterface;
            return wyFalse;
        }

        item.m_lparam     = (LPARAM)ptabinterface;

        // create new tab
		VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);		
		count = CustomTab_GetItemCount(m_hwnd);

		if(count > 1 && m_isdefault == wyFalse)
		{			
            CustomTab_SetCurSel(m_hwnd, count - 1);
            //CustomTab_SetCurSel(ptabinterface->m_ptabinterfacetabmgmt->m_hwnd, 0);      //Remove this code line after developing Fields Tab
			rstatus = CustomTab_EnsureVisible(m_hwnd, count - 1, wyFalse);
					
			if(rstatus == wyFalse)
				CustomTab_EnsureVisible(m_hwnd, 0, wyFalse);
		}

        //ShowWindow(m_hwnd, TRUE);			
		Resize();
		m_istabcreate = wyFalse;

		return wyTrue;
    }

    return wyFalse;
}

wyBool
TabModule::IsAlterTableTabOpen(wyString& tblname, wyInt32& tabindex)
{
    wyUInt32            count = 0;
    CTCITEM             item = {0};
    TableTabInterface   *ptabint = NULL;

    count   =   CustomTab_GetItemCount(m_hwnd);

    item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;

    for(int i=0; i<count; i++)
    {
        tabindex = i;
        CustomTab_GetItem(m_hwnd, i, &item);
        switch(item.m_iimage)
        {
        case IDI_ALTERTABLE:
            ptabint = (TableTabInterface*) item.m_lparam;
            if(ptabint->m_origtblname.Compare(GetActiveWin()->m_pcqueryobject->m_seltable) == 0 &&
                ptabint->m_dbname.Compare(GetActiveWin()->m_pcqueryobject->m_seldatabase) == 0)
            {
                return wyTrue;
            }
            break;
        }
    }
    
    tabindex = -1;
    return wyFalse;
}

//Handle to create SchemaDesigner
wyBool
TabModule::CreateSchemaDesigner(MDIWindow * wnd)
{
#ifndef COMMUNITY
    wyInt32				ret, count = 0;
	wyBool				rstatus = wyFalse, retval;
	CTCITEM				item = {0};
	TabSchemaDesigner	*ptabschemadesigner = NULL;
	
	// get the number of tabs
    count = CustomTab_GetItemCount(m_hwnd);

	item.m_psztext    = _("Schema Designer");
	item.m_cchtextmax = strlen(_("Schema Designer"));
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	item.m_iimage     = IDI_SCHEMADESIGNER_16;
	item.m_tooltiptext = _("Schema Designer");
	
	

	ptabschemadesigner = new TabSchemaDesigner(m_hwnd);
	m_pctabsd = ptabschemadesigner;
	if(ptabschemadesigner)
	{				
		retval = ptabschemadesigner->Create(wnd);
		if(retval == wyFalse)
		{
			delete ptabschemadesigner;
			return wyFalse;
		}

		m_istabcreate = wyTrue;

		ptabschemadesigner->SetParentPtr(this);

		item.m_lparam     = (LPARAM)ptabschemadesigner;

		// create new tab
		VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);		
		count = CustomTab_GetItemCount(m_hwnd);

		
		
		SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);

		if(count > 1 && m_isdefault == wyFalse)
		{			
            CustomTab_SetCurSel(m_hwnd, count - 1);
			rstatus = CustomTab_EnsureVisible(m_hwnd, count - 1, wyFalse);
					
			if(rstatus == wyFalse)
				CustomTab_EnsureVisible(m_hwnd, 0, wyFalse);
		}

		//ShowWindow(m_hwnd, TRUE);			
		Resize();		
		m_istabcreate = wyFalse;

		return wyTrue;
	}
#endif

	return wyFalse;
}

//Search tab
wyBool
TabModule::CreateDatabaseSearchTab(MDIWindow * wnd, wyBool isdefault)
{
#ifndef COMMUNITY	
	TabDbSearch  *pdbsearch = NULL;
	
	pdbsearch = new(std::nothrow)TabDbSearch(wnd);
	if(!pdbsearch )
	{
		return wyFalse;
	}
	
	SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);

	pdbsearch->SetParentPtr(this);
	pdbsearch->CreateInterface(isdefault);

	Resize();
#endif
	return wyTrue;
}


//History tab
wyBool
TabModule::CreateHistoryTab(MDIWindow * wnd, wyBool showtab, wyBool setfocus)
{
    TabEditor* pte;

    if(!m_pctabhistory)
	{
		m_pctabhistory = new(std::nothrow)TabHistory(m_hwnd, wnd);
		
        if(!m_pctabhistory )
		{
			return wyFalse;
		}

		m_pctabhistory->Create();
    }

    if(showtab)
	{
        if(pGlobals->m_ishistoryunderquery == wyTrue)
        {
            if(!(pte = GetActiveTabEditor()))
            {
                CreateQueryEditorTab(wnd);
                pte = GetActiveTabEditor();
            }

            pte->m_pctabmgmt->SelectFixedTab(IDI_HISTORY);
        }
        else
        {
            m_pctabhistory->Resize();
            m_pctabhistory->Show(setfocus);
            GetTabOpenPersistence(IDI_HISTORY, wyTrue, wyTrue);
        }
	}

	return wyTrue;
}


//Info tab
wyBool
TabModule::CreateInfoTab(MDIWindow* wnd, wyBool setfocus)
{
	wyInt32     i, count;
    CTCITEM     ctci = {0};
    TabObject*  ptabobj;
    TabEditor*  pte;

    if(pGlobals->m_isinfotabunderquery == wyTrue)
    {
        if(!(pte = GetActiveTabEditor()))
        {
            CreateQueryEditorTab(wnd);
            pte = GetActiveTabEditor();
        }

        pte->m_pctabmgmt->SelectFixedTab(IDI_TABLEINDEX);
        return wyTrue;
    }

    count = CustomTab_GetItemCount(m_hwnd);
    
    for(i = 0; i < count; ++i)
    {
        ctci.m_mask = CTBIF_IMAGE;
        CustomTab_GetItem(m_hwnd, i, &ctci);

        if(ctci.m_iimage == IDI_TABLEINDEX)
        {
            if(setfocus == wyTrue)
            {
                CustomTab_SetCurSel(m_hwnd, i, 0);
                CustomTab_EnsureVisible(m_hwnd, i);
            }

            return wyTrue;
        }
    }

	ptabobj = new(std::nothrow) TabObject(m_hwnd, wnd);
    SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);
	count = ptabobj->CreateInfoTab(wnd, m_hwnd);
	Resize();
    GetTabOpenPersistence(IDI_TABLEINDEX, wyTrue, wyTrue);

    if(setfocus)
	{
		CustomTab_SetCurSel(m_hwnd, count);
        CustomTab_EnsureVisible(m_hwnd, count);
	}

	return wyTrue;
}

//Info tab
wyBool
TabModule::CreateTabDataTab(MDIWindow * wnd, wyBool isnewtab, wyBool setfocus)
{
	wyInt32             count;
    wyInt32				i, seltab;
    CTCITEM				tmp = {0};
	wyString			buffer;
    TabTableData*       temptab = NULL;
    MySQLTableDataEx*   tempdata;
    wyChar              buff[140];
    wyBool              istabfound = wyFalse, istableselected = wyFalse;
    TabEditor*          pte;

    if(isnewtab == wyFalse && pGlobals->m_istabledataunderquery == wyTrue)
    {
        if(!(pte = GetActiveTabEditor()))
        {
            CreateQueryEditorTab(wnd);
            pte = GetActiveTabEditor();
        }

        pte->m_pctabmgmt->SelectFixedTab(IDI_TABLE);
        return wyTrue;
    }

	if(!m_tableview)
	{
		m_tableview	= new TableView(wnd, m_hwnd);
		m_tableview->Create();
        m_tableview->Resize();
	}
    
	seltab = CustomTab_GetCurSel(m_hwnd);
	count = CustomTab_GetItemCount(m_hwnd);
    tmp.m_mask = CTBIF_IMAGE | CTBIF_LPARAM | CTBIF_TOOLTIP;
    tmp.m_tooltiptext = buff;

    if(wnd->m_pcqueryobject->IsSelectionOnTable() == wyTrue)
    {
        istableselected = wyTrue;
        buffer.Sprintf("`%s`.`%s`", wnd->m_pcqueryobject->m_seldatabase.GetString(), wnd->m_pcqueryobject->m_seltable.GetString());
    }
    else
    {
        buffer.Sprintf(_("Table Data"));
    }

    for(i = 0; i < count; i++)
    {
        CustomTab_GetItem(m_hwnd, i, &tmp);

        if(tmp.m_iimage == IDI_TABLE && ((temptab = (TabTableData*)tmp.m_lparam))->m_istabsticky == isnewtab && isnewtab == wyFalse)
        {
            if(istableselected == wyTrue && 
                (!temptab->m_tabledata || 
                temptab->m_tabledata->m_db.Compare(wnd->m_pcqueryobject->m_seldatabase) ||  
                temptab->m_tabledata->m_table.Compare(wnd->m_pcqueryobject->m_seltable)))
            {
                tempdata = temptab->m_tabledata;
                temptab->m_tabledata = new MySQLTableDataEx(wnd);
                temptab->m_tableview->SetData(temptab->m_tabledata);
                delete tempdata;
                buffer.Sprintf("`%s`.`%s`", temptab->m_tabledata->m_db.GetString(), temptab->m_tabledata->m_table.GetString());
                tmp.m_mask = CTBIF_TOOLTIP | CTBIF_TEXT | CTBIF_IMAGE | CTBIF_LPARAM;
                tmp.m_tooltiptext = (wyChar*)buffer.GetString();
                tmp.m_psztext = _("Table Data");
                tmp.m_cchtextmax = strlen(_("Table Data"));
                CustomTab_SetItem(m_hwnd, i, &tmp);
                temptab->m_isrefreshed = wyFalse;
            }
            else
            {
                temptab->m_isrefreshed = wyTrue;
            }
            
            if(setfocus == wyTrue)
            {
                CustomTab_EnsureVisible(m_hwnd, i);
                    
                if(seltab != i)
                {
                    CustomTab_SetCurSel(m_hwnd, i);
                }
                else if(temptab->m_isrefreshed == wyFalse)
                {
                    temptab->ShowTabContent(i, wyTrue);
                }
            }

            istabfound = wyTrue;
            break;
        }
        else
        {
            if(tmp.m_iimage == IDI_TABLE && ((temptab = (TabTableData*)tmp.m_lparam))->m_istabsticky == isnewtab 
                && isnewtab == wyTrue && !buffer.Compare(tmp.m_tooltiptext))
            {
                CustomTab_SetCurSel(m_hwnd, i);
                CustomTab_EnsureVisible(m_hwnd, i);
                istabfound = wyTrue;
                return wyTrue;
            }
        }
    }

    if(istabfound == wyFalse)
    {
        temptab = new TabTableData(wnd, m_hwnd, isnewtab);

        if(setfocus == wyFalse)
        {
            delete temptab->m_tabledata;
            temptab->m_tabledata = NULL;
        }

        temptab->CreateTab(setfocus);

        if(isnewtab == wyFalse)
        {
            GetTabOpenPersistence(IDI_TABLE, wyTrue, wyTrue);
        }
    }

	return wyTrue;
}

/* Resize for Tab
If this fun calls not during the Create any Tab, the m_istabcreate is wyFalse and handling the flickering.
While Creating tab, the Create tab function itself handles the WM_SETREDRAW
*/
void
TabModule::Resize(wyBool issetredraw)  
{
	RECT				rcmain, rcvsplitter;
	wyInt32				hpos, vpos, width, height; 
    wyInt32				tabcount, selindex, headht = 0;
    CTCITEM				item;
	MDIWindow			*pcmdiwindow;
	TabTypes			*ptabtypes;
	FrameWindowSplitter *pcqueryvsplitter;

	pcmdiwindow		 =	(MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
	pcqueryvsplitter = pcmdiwindow->GetVSplitter();

	VERIFY(GetClientRect(m_hwndparent, &rcmain));
	
	VERIFY(GetWindowRect(pcqueryvsplitter->GetHwnd(), &rcvsplitter));

	VERIFY(MapWindowPoints(NULL, m_hwndparent,(LPPOINT)&rcvsplitter, 2));
	
	hpos = (rcvsplitter.right);

	vpos = 2;
	width =	(rcmain.right - hpos) - 2;
	height = rcmain.bottom - vpos - 2;			
	
	if(issetredraw == wyFalse &&  m_istabcreate == wyFalse)
		SendMessage(m_hwnd, WM_SETREDRAW, FALSE, NULL);

#ifdef COMMUNITY
	wyInt32 headvpos = 0;
	
	headvpos = vpos;
    headht = 20;
	VERIFY(MoveWindow(m_hwndcommtytitle, hpos, headvpos, width, headht, TRUE));
    InvalidateRect(m_hwndcommtytitle, NULL, TRUE);
    UpdateWindow(m_hwndcommtytitle);
	height = height - 20;
	
#endif

	vpos = vpos + headht;

	VERIFY(MoveWindow(m_hwnd, hpos, vpos, width, height, TRUE));

    if(m_tableview)
    {
        m_tableview->Resize();
    }

    	
	selindex = CustomTab_GetCurSel(m_hwnd);

	item.m_mask = CTBIF_LPARAM;

	for(tabcount = 0; tabcount < CustomTab_GetItemCount(m_hwnd); tabcount++)
	{
		CustomTab_GetItem(m_hwnd, tabcount, &item);
		ptabtypes =(TabTypes *)item.m_lparam;
				
		ptabtypes->Resize();
		ptabtypes->HandleTabControls(tabcount,  selindex);

		if(m_istabcreate == wyFalse && issetredraw == wyFalse)
			ptabtypes->HandleFlicker();		
	}	

	if(issetredraw == wyFalse)
	{
		SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hwnd, NULL, TRUE);
		UpdateWindow(m_hwnd);
	}

	return;
}

//To get the Active Tab
TabTypes*
TabModule::GetActiveTabType()
{
	CTCITEM         item;
	TabTypes		*ptabtypes;
	wyInt32         itemindex;

    item.m_mask       = CTBIF_IMAGE | CTBIF_LPARAM;

	itemindex	 =	CustomTab_GetCurSel(m_hwnd);
	
	//Avoids handling index of '-1'
	if(itemindex < 0)
        return NULL;

	CustomTab_GetItem(m_hwnd, itemindex, &item);

	ptabtypes = (TabTypes *)item.m_lparam;

	return ptabtypes;
}

TabEditor*
TabModule::GetActiveTabEditor()
{	
    return GetTabEditorAt(CustomTab_GetCurSel(m_hwnd));
}

TabEditor*	
TabModule::GetTabEditorAt(wyInt32 index)
{
    CTCITEM	item = {0};

	if(index < 0)
    {
        return NULL;
    }

    item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
	CustomTab_GetItem(m_hwnd, index, &item);

	if(item.m_iimage != IDI_QUERYBUILDER_16 && item.m_iimage != IDI_SCHEMADESIGNER_16 && item.m_iimage != IDI_HISTORY 
		&& item.m_iimage != IDI_TABLEINDEX && item.m_iimage != IDI_CREATETABLE && item.m_iimage != IDI_ALTERTABLE
        && item.m_iimage != IDI_TABLE && item.m_iimage != IDI_DATASEARCH) 
	{
		return (TabEditor*)item.m_lparam;
	}

	return NULL;
}

TabTableData* 
TabModule::GetActiveTabTableData()
{
    CTCITEM	item = {0};
	wyInt32 itemindex;

    item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
	itemindex =	CustomTab_GetCurSel(m_hwnd);
	
	if(itemindex < 0)
    {
        return NULL;
    }

	CustomTab_GetItem(m_hwnd, itemindex, &item);

	if(item.m_iimage == IDI_TABLE) 
	{
		return (TabTableData*)item.m_lparam;
	}

    return NULL;
}

TabHistory*
TabModule::GetActiveHistoryTab()
{	
	return m_pctabhistory;
}

TabObject*
TabModule::GetActiveInfoTab()
{	
	CTCITEM	item = {0};
	wyInt32 itemindex;

    item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
	itemindex =	CustomTab_GetCurSel(m_hwnd);
	
	if(itemindex < 0)
    {
        return NULL;
    }

	CustomTab_GetItem(m_hwnd, itemindex, &item);

	if(item.m_iimage == IDI_TABLEINDEX) 
	{
		return (TabObject*)item.m_lparam;
	}

    return NULL;
}

wyInt32
TabModule::GetActiveTabImage()
{
	CTCITEM				item;
	wyInt32				itemindex;

    item.m_mask       = CTBIF_IMAGE | CTBIF_LPARAM;

	itemindex	 =	CustomTab_GetCurSel(m_hwnd);
	
	if(itemindex < 0)
        return 0;

	CustomTab_GetItem(m_hwnd, itemindex, &item);

	return item.m_iimage;	
}


LPSTR
TabModule::GetActiveTabText()
{
	CTCITEM				item;
	wyInt32				itemindex;
	wyString			name;

    item.m_mask       = CTBIF_IMAGE | CTBIF_LPARAM;

	itemindex	 =	CustomTab_GetCurSel(m_hwnd);
	
	if(itemindex < 0)
        return 0;

	CustomTab_GetItem(m_hwnd, itemindex, &item);
	return item.m_psztext;	
}


// Set the EditorBase font
void
TabModule::SetTabFont()
{
	CTCITEM         item;

	TabEditor		*pctabeditor;
	TabHistory		*pctabhistory;
	TabObject		*pcinfotab;
    TabTableData    *pctabtabledata;
    TableTabInterface  *pctabint;

#ifndef COMMUNITY
    TabQueryBuilder *ptabbq;
    TabDbSearch     *ptabdbsearch;
#endif

	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);
    item.m_mask  =	CTBIF_LPARAM | CTBIF_IMAGE;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);
		if(item.m_iimage != IDI_QUERYBUILDER_16 && item.m_iimage != IDI_SCHEMADESIGNER_16 && item.m_iimage != IDI_HISTORY 
		    && item.m_iimage != IDI_TABLEINDEX && item.m_iimage != IDI_CREATETABLE && item.m_iimage != IDI_ALTERTABLE
            && item.m_iimage != IDI_TABLE && item.m_iimage != IDI_DATASEARCH)
		//if(item.m_iimage == IDI_QUERY_16)
        {
		    pctabeditor = (TabEditor *)item.m_lparam;
		    EditorFont::SetFont(pctabeditor->m_peditorbase->m_hwnd, "EditFont", wyTrue);
            if(pctabeditor->m_pctabmgmt->m_presultview)
            {
                pctabeditor->m_pctabmgmt->m_presultview->SetAllFonts();
            }
            if(pctabeditor->m_pctabmgmt->m_pcquerymessageedit != NULL)
            {
                pctabeditor->m_pctabmgmt->m_pcquerymessageedit->SetFont();
            }
            if(pctabeditor->m_pctabmgmt->m_ptabletab && pctabeditor->m_pctabmgmt->m_ptabletab->m_ptableview)
            {
                pctabeditor->m_pctabmgmt->m_ptabletab->m_ptableview->SetAllFonts();
            }

            if(pctabeditor->m_pctabmgmt->m_phistory)
            {
                pctabeditor->m_pctabmgmt->m_phistory->SetFont();
            }

            if(pctabeditor->m_pctabmgmt->m_pqueryobj)
            {
                pctabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->SetFont();
            }
        }
		if(item.m_iimage == IDI_HISTORY)
		{
			pctabhistory = (TabHistory *)item.m_lparam;
		    EditorFont::SetFont(pctabhistory->m_hwnd, "HistoryFont", wyTrue);
		}

		if(item.m_iimage == IDI_TABLEINDEX)
		{
			pcinfotab = (TabObject *)item.m_lparam;
			EditorFont::SetFont(pcinfotab->m_hwnd, "HistoryFont", wyTrue);
		}

        if(item.m_iimage == IDI_TABLE)
        {
            pctabtabledata = (TabTableData*)item.m_lparam;
            pctabtabledata->m_tableview->SetAllFonts();
        }

#ifndef COMMUNITY
        if(item.m_iimage == IDI_QUERYBUILDER_16)
        {        
		    ptabbq = (TabQueryBuilder *)item.m_lparam;
		    EditorFont::SetFont(ptabbq->m_hwndedit, "EditFont", wyTrue);
        }

        if(item.m_iimage == IDI_DATASEARCH)
        {
            ptabdbsearch = (TabDbSearch*)item.m_lparam;
            ptabdbsearch->m_pdataview->SetAllFonts();
        }
#endif
        if(item.m_iimage == IDI_CREATETABLE || item.m_iimage == IDI_ALTERTABLE)
        {
            pctabint = (TableTabInterface *) item.m_lparam;
            EditorFont::SetFont(pctabint->m_ptabintmgmt->m_tabpreview->m_hwndpreview, "HistoryFont", wyTrue);
            pctabint->SetAllFonts();
        }
	}
	return;
}

// Set the EditorBase font color.
void  
TabModule::SetTabFontColor()
{
	CTCITEM         item;
	TabEditor		*pctabeditor;
	TabHistory		*pctabhistory;
	TabObject		*pcinfotab;
    TabTableData    *pctabtabledata;
#ifndef COMMUNITY
    TabQueryBuilder *ptabbq;
    TabDbSearch*    pdbsearch;
#endif
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);
    item.m_mask  =	CTBIF_LPARAM | CTBIF_IMAGE;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);

        if(item.m_iimage != IDI_QUERYBUILDER_16 && item.m_iimage != IDI_SCHEMADESIGNER_16 && item.m_iimage != IDI_HISTORY 
		    && item.m_iimage != IDI_TABLEINDEX && item.m_iimage != IDI_CREATETABLE && item.m_iimage != IDI_ALTERTABLE
            && item.m_iimage != IDI_TABLE && item.m_iimage != IDI_DATASEARCH) 
        {
		    pctabeditor = (TabEditor *)item.m_lparam;

		    EditorFont::SetColor(pctabeditor->m_peditorbase->m_hwnd, wyTrue);
			EditorFont::SetCase(pctabeditor->m_peditorbase->m_hwnd);
		    EditorFont::SetWordWrap(pctabeditor->m_peditorbase->m_hwnd);

            // Change Folding Colors
            EnableFolding(pctabeditor->m_peditorbase->m_hwnd);

            //Change Brace light and Brace Bad color
            SetParanthesisHighlighting(pctabeditor->m_peditorbase->m_hwnd);

            if(pctabeditor->m_pctabmgmt->m_pcquerymessageedit != NULL)
            {
                pctabeditor->m_pctabmgmt->m_pcquerymessageedit->SetColor();
            }

            if(pctabeditor->m_pctabmgmt->m_presultview)
            {
                pctabeditor->m_pctabmgmt->m_presultview->SetColor();
            }

            if(pctabeditor->m_pctabmgmt->m_ptabletab && pctabeditor->m_pctabmgmt->m_ptabletab->m_ptableview)
            {
                pctabeditor->m_pctabmgmt->m_ptabletab->m_ptableview->SetColor();
            }

            if(pctabeditor->m_pctabmgmt->m_phistory)
            {
                pctabeditor->m_pctabmgmt->m_phistory->SetColor();
            }

            if(pctabeditor->m_pctabmgmt->m_pqueryobj)
            {
                pctabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->SetColor();
            }

            if(pGlobals->m_isautocompletehelp == wyTrue && pctabeditor->m_peditorbase->m_hwndhelp)
            {
                InvalidateRect(pctabeditor->m_peditorbase->m_hwndhelp,NULL,FALSE);
            }
        }

        if(item.m_iimage == IDI_TABLE)
        {
            pctabtabledata = (TabTableData*)item.m_lparam;
            if(pctabtabledata->m_tableview)
            {
                pctabtabledata->m_tableview->SetColor();
            }
        }

		if(item.m_iimage == IDI_HISTORY)
        {
		    pctabhistory = (TabHistory *)item.m_lparam;
			EditorFont::SetColor(pctabhistory->m_hwnd, wyTrue);
			EditorFont::SetCase(pctabhistory->m_hwnd);
		    EditorFont::SetWordWrap(pctabhistory->m_hwnd);
        }

		if(item.m_iimage == IDI_TABLEINDEX)
		{
			pcinfotab = (TabObject *)item.m_lparam;
            pcinfotab->m_pobjinfo->SetColor();
			EditorFont::SetCase(pcinfotab->m_hwnd);
		    EditorFont::SetWordWrap(pcinfotab->m_hwnd);
        }
       
#ifndef COMMUNITY
        if(item.m_iimage == IDI_QUERYBUILDER_16)
        {
            ptabbq = (TabQueryBuilder *)item.m_lparam;
            EditorFont::SetColor(ptabbq->m_hwndedit, wyTrue);
			EditorFont::SetCase(ptabbq->m_hwndedit);
            EditorFont::SetWordWrap(ptabbq->m_hwndedit);
        }

        if(item.m_iimage == IDI_DATASEARCH)
        {
            pdbsearch = (TabDbSearch*)item.m_lparam;
            pdbsearch->m_pdataview->SetColor();
        }
#endif
        if(item.m_iimage == IDI_CREATETABLE || item.m_iimage == IDI_ALTERTABLE)
        {
            TableTabInterface *ptabint = (TableTabInterface *) item.m_lparam;
            EditorFont::SetColor(ptabint->m_ptabintmgmt->m_tabpreview->m_hwndpreview, wyTrue);
			EditorFont::SetCase(ptabint->m_ptabintmgmt->m_tabpreview->m_hwndpreview);
        }

	}
	return;
}

// Set font color for TabHistory for all TabEditor(s).
void
TabModule::SetHistoryColor()
{
	CTCITEM         item;
	TabEditor		*pctabeditor;
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);
	item.m_mask  =	CTBIF_LPARAM | CTBIF_IMAGE;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);

        if(item.m_iimage == IDI_QUERYBUILDER_16 || item.m_iimage == IDI_SCHEMADESIGNER_16  || item.m_iimage == IDI_DATASEARCH)
            continue;

        if(item.m_iimage == IDI_CREATETABLE || item.m_iimage == IDI_ALTERTABLE)
        {
            TableTabInterface *ptabint;
            ptabint = (TableTabInterface *)item.m_lparam;
            EditorFont::SetColor(ptabint->m_ptabintmgmt->m_tabpreview->m_hwnd, wyTrue);
		    EditorFont::SetCase(ptabint->m_ptabintmgmt->m_tabpreview->m_hwnd);
            continue;
        }
		pctabeditor = (TabEditor *)item.m_lparam;

}

	return;
}

//Set the TabHistory font common for all TabEditor(s).
void
TabModule::SetHistoryFont()
{
	CTCITEM         item;
	TabTypes		*pctabtype;
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);
    item.m_mask  =	CTBIF_LPARAM | CTBIF_IMAGE;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);
        
        if(item.m_iimage == IDI_QUERYBUILDER_16 || item.m_iimage == IDI_SCHEMADESIGNER_16 || item.m_iimage == IDI_DATASEARCH)
            continue;

		if(item.m_iimage == IDI_HISTORY || item.m_iimage == IDI_TABLEINDEX)
		{
			pctabtype = (TabTypes *)item.m_lparam;
			EditorFont::SetFont(pctabtype->m_hwnd,  "HistoryFont", wyTrue);
		}

        if(item.m_iimage == IDI_CREATETABLE)
        {
            TableTabInterface *ptabint;
            ptabint = (TableTabInterface *)item.m_lparam;
            EditorFont::SetFont(ptabint->m_ptabintmgmt->m_tabpreview->m_hwnd,  "HistoryFont", wyTrue);
            continue;
        }
	}

	return;
}

void
TabModule::SetBackQuotesOption()
{
#ifndef COMMUNITY
    CTCITEM         item;
    TabQueryBuilder *ptabqb;
    
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);
    item.m_mask  =	CTBIF_LPARAM | CTBIF_IMAGE;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);
        
        if(item.m_iimage != IDI_QUERYBUILDER_16)
            continue;

        ptabqb = (TabQueryBuilder*)item.m_lparam;

        ptabqb->SetBackTickOption();
        
        //set m_isautomated to prevent OnGenerateQuery() from setting the dirty title
        ptabqb->m_isautomated = wyTrue;
        ptabqb->OnGenerateQuery(0);		
        ptabqb->m_isautomated = wyFalse;
	}
#endif

	return;
}

void
TabModule::Refresh()
{
	wyInt32			tabicon = 0;

	tabicon = GetActiveTabImage();
	if (IDI_CREATETABLE == tabicon || IDI_ALTERTABLE == tabicon)
	{
		TableTabInterfaceTabMgmt *tabintmgmt;
		tabintmgmt = ((TableTabInterface*)GetActiveTabType())->m_ptabintmgmt;
		tabintmgmt->m_tabindexes->Refresh();
		tabintmgmt->m_tabfk->Refresh();

		if (tabintmgmt->GetActiveTabImage() == IDI_TABPREVIEW)
			tabintmgmt->m_tabpreview->GenerateAndSetPreviewContent();
	}

}

// function to handle Edit menu items for each TabEditor ( CTRL+L, CTL+2, CTRL+3).
wyBool
TabModule::HandleCheckMenu(MDIWindow * wnd, wyBool ischecked, wyUInt32 menuid)
{
    long        lstyle;		
	HMENU       hmenu, hsubmenu;

	VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
	
	lstyle = GetWindowLongPtr(wnd->m_hwnd, GWL_STYLE);

	if(lstyle & WS_MAXIMIZE && wyTheme::IsSysmenuEnabled(wnd->m_hwnd))
		VERIFY(hsubmenu = GetSubMenu(hmenu, 2));
	else
		VERIFY(hsubmenu = GetSubMenu(hmenu, 1));
	
	lstyle = (ischecked == wyFalse)?(MF_UNCHECKED):(MF_CHECKED);

	CheckMenuItem(hsubmenu, menuid, MF_BYCOMMAND | lstyle);

	return wyTrue;
}

//Setting tab name
void
TabModule::SetTabName(wyWChar *filename, wyBool isshowext, wyBool isedited)
{
	CTCITEM         item;
	wyInt32         itemindex, tabimage;
    wyWChar			fname[MAX_PATH] = {0}, ext[MAX_PATH] = {0};
	wyString		file, extn, tempfilename;
	wyString        path;
	
	tabimage = GetActiveTabImage();
	_wsplitpath(filename, NULL, NULL, fname, ext);

	file.SetAs(fname);
	extn.SetAs(ext);

	path.SetAs(filename);

    //truncate file name if length is greater than 24
	//take 1st 12 characters .... and last 12 characters
	if(file.GetLength() >  SIZE_24)
	{
		tempfilename.SetAs(file);

		//take 1st 12 characters
		tempfilename.Strip(file.GetLength() - SIZE_12);
		
		tempfilename.Add("...");

		//take last 12 characters
		tempfilename.Add(file.Substr((file.GetLength() - SIZE_12), SIZE_12));

		file.SetAs(tempfilename);
	}

	if(isshowext == wyTrue && extn.GetLength())
		file.AddSprintf("%s", extn.GetString());	

	if(isedited == wyTrue)
	{
		file.AddSprintf("%s", "*");
		
	}
	
		
	itemindex	 =	CustomTab_GetCurSel(m_hwnd);
	
	item.m_mask         = CTBIF_TEXT | CTBIF_IMAGE | CTBIF_CMENU | CTBIF_TOOLTIP;
	item.m_psztext      = (wyChar*)file.GetString();
	item.m_cchtextmax   = file.GetLength();	
	item.m_iimage		= tabimage;
	item.m_tooltiptext  = (wyChar*)path.GetString();
	
	VERIFY(CustomTab_SetItem(m_hwnd, itemindex, &item));
	return;	
}


//Setting tab name
void
TabModule::SetTabRename(wyWChar *name, wyBool isedited)
{
	CTCITEM         item;
	wyInt32         itemindex, tabimage;
	wyString		newname;
	
	tabimage = GetActiveTabImage();
	newname.SetAs(name);
	if(isedited == wyTrue)
	{
		newname.AddSprintf("%s", "*");
		
	}
	

	itemindex	 =	CustomTab_GetCurSel(m_hwnd);
	
	item.m_mask         = CTBIF_TEXT | CTBIF_IMAGE | CTBIF_CMENU | CTBIF_TOOLTIP;
	item.m_psztext      = (wyChar*)newname.GetString();
	item.m_cchtextmax   = newname.GetLength();	
	item.m_iimage		= tabimage;
	item.m_tooltiptext  = (wyChar*)newname.GetString();
	
	//m_pctabeditor->m_tabtitle.SetAs(item.m_psztext);
	VERIFY(CustomTab_SetItem(m_hwnd, itemindex, &item));
	return;	
}
	
// sets parent MDIwindow pointer.
wyBool	
TabModule::SetParentPtr(MDIWindow *wnd)
{
	m_parentptr = wnd;

	return wyTrue;
}

//Get the parent connection window pointer.
MDIWindow *
TabModule::GetParentPtr()
{
	return m_parentptr;
}

//to get current window handle.
HWND
TabModule::GetHwnd()
{
	return m_hwnd;
}

//to get parent Conn. Window handle.
HWND
TabModule::GetparentHwnd()
{
	return m_hwndparent;
}

