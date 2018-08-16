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


#include "PreferenceBase.h"
#include "EditorFont.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "FrameWindowHelper.h"
#include "TableTabInterface.h"

extern PGLOBALS		pGlobals;


//General Preference Default Values
#define		WORDWRAP_DEFAULT			0
#define		JSONFORMAT_DEFAULT			0
#define		BACKQUOTES_DEFAULT			1
#define		FOCUSAFTERQUERY_DEFAULT		1
#define		CONFIRMONTABCLOSE_DEFAULT	1
#define		GETTEXTONDBCLICK_DEFAULT	1
#define		TRANSACTIONENABLE_DEFAULT	1
#define		SETMAXROW_DEFAULT			1
#define		FKCHKIMPORT_DEFAULT			1
#define		DEF_BULK_SIZE_DEFAULT		1
#define		ROW_LIMIT_DEFAULT			1000
#define		BULKINSERT_DEFAULT			1
#define		SHOWWARNINGS_DEFAULT		0
#define		HALTEXEC_DEFAULT			0
#define     INFOTAB_POS_DEFAULT         1
#define     TABLEDATA_POS_DEFAULT       1
#define     HISTORY_POS_DEFAULT         0

//Other Preference Default Values
#define		COLUMNWIDTH_DEFAULT				0
#define		REFRESHTABLEDATA_DEFAULT		0
#define		GETINFOALWAYS_DEFAULT			0
#define		GETINFOKW_DEFAULT				1
#define		SWITCHSHORTCUT_DEFAULT			0
#define		OPENTABLES_DEFAULT				0
#define		CONRESTORE_DEFAULT				1
#define		ENABLEUPGRADE_DEFAULT			1
#define		UPDATEPROMPT_DEFAULT			1
#define		PROMPTTRANSACTION_DEFAULT		1
#define		PROMPTTRANSACTIONCLOSE_DEFAULT	1
#define		SHOWALLINTABLEDATA_DEFAULT		0
#define		LOWLIMIT_DEFAULT				0
#define		RETAINCOLUMNWIDTH_DEFAULT		1
#define		RESULTTABPAGE_DEFAULT			1
#define     RESULTRETAINPAGE_DEFAULT		1



PreferenceBase::PreferenceBase()
{
	wyWChar	*lpfileport=0;
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, m_directory, &lpfileport);
	m_ispreferenceapplied   = wyFalse;
	m_isrestorealldefaults	= wyFalse;
	m_startpage = GENERALPREF_PAGE;
    m_isthemechanged = wyFalse;
    m_istabledataunderquery = pGlobals->m_istabledataunderquery;
    m_isinfotabunderquery = pGlobals->m_isinfotabunderquery;
    m_ishistoryunderquery = pGlobals->m_ishistoryunderquery;
}

PreferenceBase::~PreferenceBase()
{
}

void
PreferenceBase::Create(wyInt32 startpage)
{
	wyInt32 pagecount = 0;
	if(!pGlobals->m_entlicense.CompareI("Professional") && startpage == OTHERS_PAGE )
		startpage = startpage - 1;
	m_startpage = startpage;

	// we have to create a propertysheet.
    PROPSHEETPAGE   psp =       {0}; //defines the property sheet pages
    HPROPSHEETPAGE  ahpsp[5] =  {0}; //an array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADER psh =       {0}; //defines the property sheet

    //Create the prefence page(s)
    //Opening page
    psp.dwSize		=	sizeof(psp);
    psp.dwFlags		=	PSP_DEFAULT | PSH_NOAPPLYNOW | PSP_USETITLE;
    psp.hInstance	=	pGlobals->m_hinstance;
    psp.lParam		=   (LPARAM)this; //The shared data structure
    psp.pfnDlgProc	=   GeneralPrefDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_PREFGENERAL);
    psp.pszTitle    =   _(L"General");

    ahpsp[pagecount++] =          CreatePropertySheetPage(&psp);

	psp.dwSize		=	sizeof(psp);
    psp.dwFlags		=	PSP_DEFAULT | PSH_NOAPPLYNOW | PSP_USETITLE;
    psp.hInstance	=	pGlobals->m_hinstance;
    psp.lParam		=   (LPARAM)this; //The shared data structure
    psp.pfnDlgProc	=   ACPrefDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_PREF3);
    psp.pszTitle    =   _(L"Power Tools");
    
    ahpsp[pagecount++] =          CreatePropertySheetPage(&psp);
    
	psp.dwSize		=	sizeof(psp);
    psp.dwFlags		=	PSP_DEFAULT | PSH_NOAPPLYNOW | PSP_USETITLE | PSP_PREMATURE;
    psp.hInstance	=	pGlobals->m_hinstance;
    psp.lParam		=   (LPARAM)this; //The shared data structure
    psp.pfnDlgProc	=   FontPrefDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_PREF2);
    psp.pszTitle    =   _(L"Fonts && Editor Settings");

    ahpsp[pagecount++] =          CreatePropertySheetPage(&psp);

	if(pGlobals->m_entlicense.CompareI("Professional"))
	{
		psp.dwSize		=	sizeof(psp);
		psp.dwFlags		=	PSP_DEFAULT | PSH_NOAPPLYNOW | PSP_USETITLE;
		psp.hInstance	=	pGlobals->m_hinstance;
		psp.lParam		=   (LPARAM)this; //The shared data structure
		psp.pfnDlgProc	=   FormatterPrefDlgProc;
		psp.pszTemplate =   MAKEINTRESOURCE(IDD_PREFFORMATTER);
		psp.pszTitle    =   _(L"SQL Formatter");

		ahpsp[pagecount++] =          CreatePropertySheetPage(&psp);
	}
	
	psp.dwSize		=	sizeof(psp);
    psp.dwFlags		=	PSP_DEFAULT | PSH_NOAPPLYNOW | PSP_USETITLE;
    psp.hInstance	=	pGlobals->m_hinstance;
    psp.lParam		=   (LPARAM)this; //The shared data structure
    psp.pfnDlgProc	=   OthersPrefDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_PREFOTHERS);
    psp.pszTitle    =   _(L"Others");

    ahpsp[pagecount++] =          CreatePropertySheetPage(&psp);

	//////////////////////////////////////////////////////////////////
	// Display the page using additional code
	//////////////////////////////////////////////////////////////////

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags		= PSH_DEFAULT | PSH_NOCONTEXTHELP | PSH_NOAPPLYNOW;
	psh.phpage		= ahpsp;
	psh.hwndParent  = pGlobals->m_pcmainwin->m_hwndmain;
	psh.pszCaption	= _(L"Preferences");
	psh.nPages		= pagecount;
	psh.nStartPage  = startpage;
	
	PropertySheet(&psh);

    if(m_isthemechanged == wyTrue)
    {
        wyTheme::ApplyTheme();
    }
}

// dlgprocs for the pages.
INT_PTR CALLBACK
PreferenceBase::GeneralPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreferenceBase		*pref = (PreferenceBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
        VERIFY(pref = (PreferenceBase*)((LPPROPSHEETPAGE)lParam)->lParam);
	    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pref);
        LocalizeWindow(hwnd);
		pref->GenPrefHandleWmInitDialog(hwnd, lParam);
		return TRUE;
	
	case WM_COMMAND:
		pref->GenPrefHandleWmCommand(hwnd, wParam);					
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/131-preferences");
		return TRUE;

	case WM_NOTIFY:
		pref->GenPrefHandleWmNotify(hwnd, lParam);
		break;
	}

	return 0;
}

INT_PTR CALLBACK
PreferenceBase::ACPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreferenceBase		*pref = (PreferenceBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		VERIFY(pref = (PreferenceBase*)((LPPROPSHEETPAGE)lParam)-> lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)pref);
        LocalizeWindow(hwnd);
		pref->EnterprisePrefHandleWmInitDialog(hwnd);
		return TRUE;
		
	case WM_COMMAND:
	    pref->EntPrefHandleWmCommand(hwnd, wParam);
		return TRUE;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/131-preferences");
		return TRUE;

	case WM_NOTIFY:
	    pref->ACPrefHandleWmNotify(hwnd, lParam);
		break;
	}
	
	return 0;
}

INT_PTR CALLBACK
PreferenceBase::FormatterPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreferenceBase		*pref = (PreferenceBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		VERIFY(pref = (PreferenceBase*)((LPPROPSHEETPAGE)lParam)-> lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)pref);
        LocalizeWindow(hwnd);
		pref->FormatterPrefHandleWmInitDialog(hwnd);
		return TRUE;
		
	case WM_COMMAND:
	    pref->FormatterPrefHandleWmCommand(hwnd, wParam);
		return TRUE;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/131-preferences");
		return TRUE;

	case WM_NOTIFY:
	    pref->FormatterPrefHandleWmNotify(hwnd, lParam);
		break;
	}
	
	return 0;
}

void 
PreferenceBase::GenPrefHandleWmInitDialog(HWND hwnd, LPARAM lParam)
{
	m_hwnd = hwnd;

    if(m_startpage == GENERALPREF_PAGE)
        HandlerToSetWindowPos(hwnd);

	InitGeneralPrefValues();
}

void 
PreferenceBase::GenPrefHandleWmCommand(HWND hwnd, WPARAM wParam)
{
    switch(LOWORD(wParam))
	{
	case IDC_SETMAXROW:
		if(SendMessage(GetDlgItem(hwnd, IDC_SETMAXROW), BM_GETCHECK, 0, 0)== BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDC_ROW_LIMIT), FALSE);
		else
			EnableWindow(GetDlgItem(hwnd, IDC_ROW_LIMIT), TRUE);

		break;

	case IDC_DEF_BULK_SIZE:
		if(SendMessage(GetDlgItem(hwnd, IDC_DEF_BULK_SIZE), BM_GETCHECK, 0, 0)== BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDC_BULKINSERT), FALSE);
		else
			EnableWindow(GetDlgItem(hwnd, IDC_BULKINSERT), TRUE);

		break;

	case IDC_GENRESTORETAB:
		SetGenPrefDefaultValues(hwnd);
		break;

	case IDC_GENRESTOREALL:
		RestoreAllDefaults();
        break;

	}
}

void 
PreferenceBase::GenPrefHandleWmNotify(HWND hwnd, LPARAM lParam)
{
    LPNMHDR lpnm = (LPNMHDR)lParam;
	wyString dirstr;
    switch (lpnm->code)
    {
	case PSN_SETACTIVE:
		dirstr.SetAs(m_directory);
		m_hwnd = hwnd;
		pGlobals->m_prefpersist=GENERALPREF_PAGE;
		wyIni::IniWriteInt(GENERALPREFA, "PrefPersist", GENERALPREF_PAGE, dirstr.GetString());
		break;
	
    case PSN_APPLY: //user pressed the OK button.		
		Apply();
		break;
	}	
}


void PreferenceBase::ChangeBkFrColor(HWND hwnd,COLORREF frGnd,COLORREF bkGnd)
{
    if(bkGnd >= 0)
        m_rgbbgcolor=bkGnd;
    if(frGnd >= 0)
        m_rgbfgcolor=frGnd;
    InvalidateRect(GetDlgItem(hwnd,IDC_STATIC_TEST),NULL,TRUE);
}

INT_PTR CALLBACK
PreferenceBase::FontPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreferenceBase		*pref = (PreferenceBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		VERIFY(pref = (PreferenceBase*)((LPPROPSHEETPAGE)lParam)-> lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pref);
        LocalizeWindow(hwnd);
		pref->FontPrefHandleWmInitDialog(hwnd);
		return TRUE;

	case WM_CTLCOLORSTATIC:
        return pref->FontPrefHandleWmCtlColorStatic(hwnd, wParam, lParam);
        break;
	case WM_COMMAND:
		pref->FontPrefHandleWmCommand(hwnd, wParam);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/131-preferences");
		return TRUE;

	case WM_NOTIFY:
	    pref->FontPrefHandleWmNotify(hwnd, lParam);
		break;

    case WM_DESTROY:
        pref->OnDestroy(hwnd);
        break;
	}

	return 0;
}


void PreferenceBase::OnDestroy(HWND hwnd)
{
    TREEVIEWDATA *tvd1=NULL,*tvd=NULL;
    TVITEM tvi;
    HTREEITEM tvi_root=NULL,tvi_sibling=NULL;
    HWND htv=GetDlgItem(m_hwnd,IDC_TREE2);
    
    if(htv)
    {
        tvi_root=TreeView_GetRoot(htv);
        while(tvi_root)
        {
            tvi.mask=TVIF_PARAM;
            tvi.hItem=tvi_root;
            if(TreeView_GetItem(htv,&tvi))
            {
                tvd1=(TREEVIEWDATA *)tvi.lParam;
                switch(tvd1->style)
                {
                case EDITOR:
                    delete(tvd1->bgColor);
                    tvd1->bgColor=NULL;
                    tvd1->fgColor=NULL;
                    break;
                case CANVAS:
                    delete(tvd1->bgColor);
                    tvd1->bgColor=NULL;
                    tvd1->fgColor=NULL;
                    break;
                case MTI:
                    delete(tvd1->bgColor);
                    delete(tvd1->fgColor);
                    tvd1->bgColor=NULL;
                    tvd1->fgColor=NULL;
                    break;
                }
            }
            tvi_sibling=TreeView_GetNextItem(htv,tvi_root,TVGN_CHILD);
            while(tvi_sibling)
            {
                tvi.mask=TVIF_PARAM;
                tvi.hItem=tvi_sibling;
                if(TreeView_GetItem(htv,&tvi))
                {
                    tvd=(TREEVIEWDATA *)tvi.lParam;
                    switch(tvd->style)
                    {
                    case EDITOR_DEFAULT:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_COMMENTS:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_STRINGS:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_FUNCTIONS:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_KEYWORDS:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_NUMBER:
                        delete(tvd->fgColor);
                        tvd->bgColor=NULL;
                        tvd->fgColor=NULL;
                        
                    break;
                    case EDITOR_OPERATOR:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_HIDDENCOMMAND:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case EDITOR_LINENUMBERMARGIN:
                        delete(tvd->fgColor);
                        delete(tvd->bgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;

                    break;
                    case EDITOR_FOLDINGMARGIN:
                        delete(tvd->fgColor);
                        delete(tvd->bgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;

                    break;
                    case EDITOR_TEXTSELECTION:
                        delete(tvd->bgColor);
                        tvd->bgColor=NULL;
                        tvd->fgColor=NULL;
                    break;
                    case EDITOR_BRACESMATCH:
                        delete(tvd->fgColor);
                        delete(tvd->bgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;

                    break;
                    case EDITOR_BRACESUNMATCH:
                        delete(tvd->fgColor);
                        delete(tvd->bgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;

                        
                    break;
                    case CANVAS_LINE:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case CANVAS_TEXT:
                        delete(tvd->fgColor);
                        tvd->fgColor=NULL;
                        tvd->bgColor=NULL;
                    break;
                    case MTI_SELECTION:
                        delete(tvd->bgColor);
                        tvd->bgColor=NULL;
                        tvd->fgColor=NULL;
                    break;
                    }
                }
                tvi_sibling=TreeView_GetNextSibling(htv,tvi_sibling);
                delete(tvd);
                tvd=NULL;
            }
            tvi_root=TreeView_GetNextSibling(htv,tvi_root);
            delete(tvd1);
            tvd1=NULL;
        }
    }
}



void 
PreferenceBase::FontPrefHandleWmInitDialog(HWND hwnd)
{
    m_hwnd = hwnd;
    InitFontPrefValues();
    TREEVIEWDATA *tvd;
    COLORREF *color1;
    COLORREF *color2;
    COLORREF *color3;
    HTREEITEM hTvItem;
    TV_INSERTSTRUCT tvinsert;
   
    wyString dirstr(m_directory);
    if(m_startpage == FONT_PAGE)
		HandlerToSetWindowPos(hwnd);
    m_htv = GetDlgItem(hwnd,IDC_TREE2);
    m_rgbtexturecolor   =   wyIni::IniGetInt(GENERALPREFA, "FoldingMarginTextureColor",   COLOR_WHITE, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    color1=new COLORREF;
    color2=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "EditorBgColor",   DEF_BKGNDEDITORCOLOR, dirstr.GetString());
    *color2=wyIni::IniGetInt(GENERALPREFA, "NormalColor",   DEF_NORMALCOLOR, dirstr.GetString());
    
    m_rgbbgcolor=*color1;
    m_rgbfgcolor=*color2;
    
    tvd->mask=IF_BACKGROUND_MASK;
    tvd->style=EDITOR;
    tvd->bgColor=color1;
    tvd->fgColor=color2;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.hParent=NULL;
    tvinsert.hInsertAfter=TVI_ROOT;
    tvinsert.item.mask=TVIF_TEXT | TVIF_PARAM;
    tvinsert.item.pszText=_(L"Editor");
    tvinsert.item.iImage=0;
    tvinsert.item.iSelectedImage=1;
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
    
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_DEFAULT;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.hParent=hTvItem;
    tvinsert.hInsertAfter=TVI_LAST;
    tvinsert.item.pszText=_(L"Default");
    tvinsert.item.lParam=(LPARAM)tvd;
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "CommentColor",   DEF_COMMENTCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_COMMENTS;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Comments");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "StringColor",   DEF_STRINGCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_STRINGS;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Strings");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "FunctionColor",   DEF_FUNCTIONCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_FUNCTIONS;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Functions");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "KeywordColor",   DEF_KEYWORDCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_KEYWORDS;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Keywords");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "NumberColor",   DEF_NORMALCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_NUMBER;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Number");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "OperatorColor",   DEF_OPERATORCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_OPERATOR;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Operator");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "HiddenCmdColor",   DEF_NORMALCOLOR, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_HIDDENCOMMAND;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Hidden Command");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color1=new COLORREF;
    color3=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "NumberMarginbackgroundColor",   DEF_MARGINNUMBER, dirstr.GetString());
    *color3=wyIni::IniGetInt(GENERALPREFA, "NumberMarginforegroundColor",   DEF_MARGINNUMBERFG, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_LINENUMBERMARGIN;
    tvd->mask=IF_FOREGROUND_MASK|IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;

    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Line Number Margin");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color1=new COLORREF;
    color3=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "FoldingMarginbackgroundColor",   DEF_MARGINNUMBER, dirstr.GetString());
    *color3=wyIni::IniGetInt(GENERALPREFA, "FoldingMarginFgColor",   RGB(0,0,0), dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_FOLDINGMARGIN;
    tvd->mask=IF_FOREGROUND_MASK|IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Folding Margin");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color1=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "SelectionBgColor",   DEF_TEXTSELECTION, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_TEXTSELECTION;
    tvd->mask=IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Text Selection");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color1=new COLORREF;
    color3=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "BraceLightBgColor",   DEF_BRACELIGHTBG, dirstr.GetString());
    *color3=wyIni::IniGetInt(GENERALPREFA, "BraceLightFgColor",   DEF_BRACELIGHTFG, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_BRACESMATCH;
    tvd->mask=IF_FOREGROUND_MASK|IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Braces Match");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);


    color1=new COLORREF;
    color3=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "BraceBadBgColor",   DEF_BRACEBADBG, dirstr.GetString());
    *color3=wyIni::IniGetInt(GENERALPREFA, "BraceBadFgColor",   DEF_BRACEBADFG, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=EDITOR_BRACESUNMATCH;
    tvd->mask=IF_FOREGROUND_MASK|IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Braces UnMatch");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);



    color1=new COLORREF;
    color2=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "CanvasBgColor",   DEF_BKGNDCANVASCOLOR, dirstr.GetString());
    *color2=wyIni::IniGetInt(GENERALPREFA, "CanvasTextColor",   GetSysColor(COLOR_GRAYTEXT), dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=CANVAS;
    tvd->mask=IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.hParent=NULL;
    tvinsert.item.pszText=_(L"Canvas");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color3=new COLORREF;
    *color3=wyIni::IniGetInt(GENERALPREFA, "CanvasLineColor",  RGB(160,160,160), dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=CANVAS_LINE;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color3;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.hParent=hTvItem;
    tvinsert.item.pszText=_(L"Line");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    tvd=new TREEVIEWDATA;
    tvd->style=CANVAS_TEXT;
    tvd->mask=IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.item.pszText=_(L"Text");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

    color1=new COLORREF;
    color2=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "MTIBgColor",   COLOR_WHITE, dirstr.GetString());
    *color2=wyIni::IniGetInt(GENERALPREFA, "MTIFgColor",   RGB(0,0,0), dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=MTI;
    tvd->mask=IF_BACKGROUND_MASK|IF_FOREGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.hParent=NULL;
    tvinsert.item.pszText=_(L"Messages/Table Data/Info");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
    
    
    color1=new COLORREF;
    *color1=wyIni::IniGetInt(GENERALPREFA, "MTISelectionColor",   DEF_TEXTSELECTION, dirstr.GetString());
    tvd=new TREEVIEWDATA;
    tvd->style=MTI_SELECTION;
    tvd->mask=IF_BACKGROUND_MASK;
    tvd->bgColor=color1;
    tvd->fgColor=color2;
    
    tvinsert.item.lParam=(LPARAM)tvd;
    tvinsert.hParent=hTvItem;
    tvinsert.item.pszText=_(L"Text Selection");
    hTvItem=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
    
    wchar_t *strg=_(L"\nPreview");
    SendMessage (GetDlgItem(hwnd,IDC_EDITPREFDEMO), EM_SETSEL, (WPARAM)0, (LPARAM)0);
    SendMessage (GetDlgItem(hwnd,IDC_EDITPREFDEMO), EM_REPLACESEL, 0, (LPARAM) ((LPSTR) strg));
    
    
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTONBACKGROUNDPREF2),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTONFOREGROUNDPREF),FALSE);
    
}
	
wyInt32  
PreferenceBase::FontPrefHandleWmCtlColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	wyInt32	identifier;
	HDC     hdc = (HDC)wParam;
	
	identifier = GetDlgCtrlID((HWND)lParam);
	
	SetBkMode(hdc, TRANSPARENT);
	
	if(identifier == IDC_STATIC_TEST)
    {
        SetBkMode(hdc,TRANSPARENT);
        SetTextColor(hdc,m_rgbfgcolor);
        SetDCBrushColor(hdc, m_rgbbgcolor);
        return (BOOL)GetStockObject(DC_BRUSH);
    }

    return 0;
}

void PreferenceBase::SetColorsInStaticControl(HWND hwnd)
{
    HTREEITEM selected =NULL;
    selected=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selected);
    TVITEM tvi;
    COLORREF fg=NULL,bg=NULL;
    TREEVIEWDATA *tvd;
    if(selected != NULL)
    {
        tvi.hItem=selected;
        tvi.mask=TVIF_PARAM;
        if(TreeView_GetItem(GetDlgItem(hwnd,IDC_TREE2),&tvi))
        {
            tvd=(TREEVIEWDATA*)tvi.lParam;
            fg=*(tvd->fgColor);
            bg=*(tvd->bgColor);
        }
    }
    else
    {
        fg=DEF_NORMALCOLOR;
        bg=DEF_BKGNDEDITORCOLOR;
    } 
    ChangeBkFrColor(hwnd,fg,bg);            
}
        


void 
PreferenceBase::FontPrefHandleWmCommand(HWND hwnd, WPARAM wParam)
{
    TVITEM tvi;
    COLORREF *ref=NULL;
    TREEVIEWDATA *tvd;
    HTREEITEM selected =NULL;
                
    switch(LOWORD(wParam))
    {
	case IDC_SQLCHANGE:
		if(ChFont(&m_editfont) == wyTrue)
			PrintFontDetails();
		break;

	case IDC_DATACHANGE:
		if(ChFont(&m_datafont) == wyTrue)
			PrintFontDetails();
		break;

	case IDC_HISTORYCHANGE:
		if(ChFont(&m_historyfont) == wyTrue)
			PrintFontDetails();
		break;

    case IDC_OBCHANGE:
        if(ChFont(&m_obfont) == wyTrue)
			PrintFontDetails();
        break;

	case IDC_BLOBCHANGE:
		if(ChFont(&m_blobfont) == wyTrue)
			PrintFontDetails();
		break;

	case IDC_EDITORRESTORETAB: 
		SetDefaultFontDetails(m_hwnd);
		SetColorsInStaticControl(hwnd);
        break;

	case IDC_EDITORRESTOREALL:
		RestoreAllDefaults();
        SetColorsInStaticControl(hwnd);
        break;
    case IDC_BUTTONFOREGROUNDPREF:
        {
            selected=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selected);
            
            if(selected != NULL)
            {
                tvi.mask=TVIF_PARAM;
                tvi.hItem=selected;
                if(TreeView_GetItem(GetDlgItem(hwnd,IDC_TREE2),&tvi))
                {
                    tvd=(TREEVIEWDATA*)tvi.lParam;
                    ref=tvd->fgColor;
                }
                else
                {
                    return;
                }

                ChColor(m_hwnd,ref);
                ChangeBkFrColor(hwnd,*ref,*(tvd->bgColor));
                UpdateWindow(GetDlgItem(hwnd,IDC_EDITPREFDEMO));
            }

        }
        break;
    case IDC_BUTTONBACKGROUNDPREF2:
        {
            selected=(HTREEITEM)SendDlgItemMessage(hwnd,IDC_TREE2,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selected);
            if(selected != NULL)
            {
                
                tvi.mask=TVIF_PARAM;
                tvi.hItem=selected;
                if(TreeView_GetItem(GetDlgItem(hwnd,IDC_TREE2),&tvi))
                {
                    tvd=(TREEVIEWDATA*)tvi.lParam;
                    ref=tvd->bgColor;
                }
                else
                {
                    return;
                }

                ChColor(m_hwnd,ref);
                if(tvd->style == EDITOR_FOLDINGMARGIN)
                    m_rgbtexturecolor=*ref;
                
                ChangeBkFrColor(hwnd,*(tvd->fgColor),*ref);
            }
        }
        break;
	}
}

//IN condition: true for editor and canvas; false for children
void
PreferenceBase::SetFrgndBkgndBtn(HWND hwnd,BOOL frgnd, BOOL bkgnd)
{
            EnableWindow(GetDlgItem(hwnd,IDC_BUTTONFOREGROUNDPREF),frgnd);
            EnableWindow(GetDlgItem(hwnd,IDC_BUTTONBACKGROUNDPREF2),bkgnd);
}

void 
PreferenceBase::FontPrefHandleWmNotify(HWND hwnd, LPARAM lParam)
{
    LPNMHDR lpnm = (LPNMHDR)lParam;
    TREEVIEWDATA *tvd;
	wyString dirstr;

    if(lpnm->code == PSN_SETACTIVE)
	{	
		dirstr.SetAs(m_directory);
		pGlobals->m_prefpersist=FONT_PAGE;
		m_hwnd = hwnd;
		wyIni::IniWriteInt(GENERALPREFA, "PrefPersist", FONT_PAGE, dirstr.GetString());
	}
    else if(lpnm->code == TVN_SELCHANGED)
    {
        LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lParam;
        tvd=(TREEVIEWDATA *)pnmtv->itemNew.lParam;
        
        SetFrgndBkgndBtn(hwnd, 
            tvd->mask & IF_FOREGROUND_MASK ? TRUE: FALSE, 
            tvd->mask & IF_BACKGROUND_MASK ? TRUE : FALSE);

        ChangeBkFrColor(hwnd,*(tvd->fgColor),*(tvd->bgColor));
        UpdateWindow(hwnd);
    }
    else if(lpnm->code == TVN_ITEMEXPANDING)
    {
        LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lParam;
        if(pnmtv->action == 2)
            SendDlgItemMessage(hwnd,IDC_TREE2,TVM_SELECTITEM,TVGN_CARET,(LPARAM)pnmtv->itemNew.hItem);
    }
	else if(lpnm->code == PSN_APPLY) //user pressed the OK button.		
		Apply();
		
}

// dlgprocs for the pages.
INT_PTR CALLBACK
PreferenceBase::OthersPrefDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreferenceBase		*pref = (PreferenceBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HWND hwndcombo;
    LPTHEMEINFO pthemeinfo;

	switch(message)
	{
	case WM_INITDIALOG:
        VERIFY(pref = (PreferenceBase*)((LPPROPSHEETPAGE)lParam)->lParam);
	    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pref);
        LocalizeWindow(hwnd);
		pref->OthersPrefHandleWmInitDialog(hwnd, lParam);
		return TRUE;
	
	case WM_COMMAND:
		pref->OthersPrefHandleWmCommand(hwnd, wParam);					
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/131-preferences");
		return TRUE;

	case WM_NOTIFY:
		pref->OthersPrefHandleWmNotify(hwnd, lParam);
		break;

    case WM_DESTROY:
        hwndcombo = GetDlgItem(hwnd, IDC_THEMECOMBO);

        if((pthemeinfo = (LPTHEMEINFO)GetWindowLongPtr(hwndcombo, GWLP_USERDATA)))
        {
            wyTheme::FreeThemes(pthemeinfo, SendMessage(hwndcombo, CB_GETCOUNT, 0, 0));
        }

        break;
	}

	return 0;
}

void 
PreferenceBase::OthersPrefHandleWmInitDialog(HWND hwnd, LPARAM lParam)
{
	m_hwnd = hwnd;
    wyInt32 i=0;
	InitOthersPrefValues();
	if(!pGlobals->m_entlicense.CompareI("Professional"))
		i=1;
	if(m_startpage == OTHERS_PAGE - i)
		HandlerToSetWindowPos(hwnd);
}

void 
PreferenceBase::OthersPrefHandleWmCommand(HWND hwnd, WPARAM wParam)
{
    switch(LOWORD(wParam))
	{	
#ifndef COMMUNITY
	case IDC_PROMPTTRANSACTION:
		{
			if(Button_GetCheck(GetDlgItem(hwnd, IDC_PROMPTTRANSACTION)) == BST_CHECKED)
			{
				pGlobals->m_pcmainwin->m_topromptonimplicit = wyTrue;
			}
			else
				pGlobals->m_pcmainwin->m_topromptonimplicit = wyFalse;
		}
		break;
	case IDC_PROMPTCLOSETRANSACTION:
		{
			if(Button_GetCheck(GetDlgItem(hwnd, IDC_PROMPTCLOSETRANSACTION)) == BST_CHECKED)
			{
				pGlobals->m_pcmainwin->m_topromptonclose = wyTrue;
			}
			else
				pGlobals->m_pcmainwin->m_topromptonclose = wyFalse;
		}
		break;
#endif
	case IDC_OTHERSRESTORETAB:
		SetOthersPrefDefaultValues(hwnd);
		break;

	case IDC_OTHERSRESTOREALL:
		RestoreAllDefaults();
        break;

	case IDC_RESUTTABPAGE:
		{
			if(Button_GetCheck(GetDlgItem(hwnd, IDC_RESUTTABPAGE)) == BST_CHECKED)
			{
				//EnableWindow(GetDlgItem(hwnd, IDC_HIGHLIMIT), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_RESUTTABPAGERETAIN), TRUE);				
			}

			else
			{
				//EnableWindow(GetDlgItem(hwnd, IDC_HIGHLIMIT), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_RESUTTABPAGERETAIN), FALSE);				
			}
		}
		
		break;
	}
}

void 
PreferenceBase::OthersPrefHandleWmNotify(HWND hwnd, LPARAM lParam)
{
    LPNMHDR lpnm = (LPNMHDR)lParam;
	wyString dirstr;
    switch (lpnm->code)
    {
	case PSN_SETACTIVE:
		dirstr.SetAs(m_directory);
		m_hwnd = hwnd;
		pGlobals->m_prefpersist=OTHERS_PAGE;
		wyIni::IniWriteInt(GENERALPREFA, "PrefPersist", OTHERS_PAGE, dirstr.GetString());
		break;
	
    case PSN_APPLY: //user pressed the OK button.		
		Apply();
		pGlobals->m_isrefreshkeychange = IsRefreshOptionChange();

		if(pGlobals->m_menurefreshobj)
			free(pGlobals->m_menurefreshobj);

		if(pGlobals->m_menucurrentquery)
			free(pGlobals->m_menucurrentquery);
			
		if(pGlobals->m_menuselectedquery)
			free(pGlobals->m_menuselectedquery);

		if(pGlobals->m_menuallquery)
			free(pGlobals->m_menuallquery); 

		pGlobals->m_menurefreshobj = NULL;
		pGlobals->m_menucurrentquery = NULL;
		pGlobals->m_menuselectedquery = NULL;
		pGlobals->m_menuallquery = NULL;
        break;
	}	
}

// initialize the general dialog page with its default values.
void
PreferenceBase::InitGeneralPrefValues()
{
	wyInt32	    truncdata = 0;
	wyString	dirstr(m_directory);

	if(m_isrestorealldefaults == wyTrue) //If user clicks Restore all defaults,then we need to set default values for all tabs.
	{
		SetGenPrefDefaultValues(m_hwnd);
		return;
	}

	SendMessage(GetDlgItem(m_hwnd, IDC_BULKINSERT), EM_LIMITTEXT, 5, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "AppendBackQuotes", BACKQUOTES_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_BACKQUOTES), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "FocusOnEdit", FOCUSAFTERQUERY_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_FOCUSAFTERQUERY), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "PromptOnTabClose", CONFIRMONTABCLOSE_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_CONFIRMONTABCLOSE), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "GetTextOnDBClick", GETTEXTONDBCLICK_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_GETTEXTONDBCLICK), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "WordWrap", WORDWRAP_DEFAULT/* from 5.1 RC1 default value is false */, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_WORDWRAP), BM_SETCHECK, truncdata, 0);

	truncdata = wyIni::IniGetInt(GENERALPREFA, "JsonFormat", JSONFORMAT_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_JSON), BM_SETCHECK, truncdata, 0);

	truncdata   = wyIni::IniGetInt(GENERALPREFA, "StartTransaction", TRANSACTIONENABLE_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_TRANSACTIONENABLE), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "SetMaxRow", SETMAXROW_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_SETMAXROW), BM_SETCHECK, truncdata, 0);

	EnableRowLimit(m_hwnd, !truncdata);
	
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "ShowWarnings", SHOWWARNINGS_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_SHOWWARNING), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "HaltExecutionOnError", HALTEXEC_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_HALTEXEC), BM_SETCHECK, truncdata, 0);

	//disable fk check in http import batch
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "FKcheckImport", FKCHKIMPORT_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_FKCHKIMPORT), BM_SETCHECK, truncdata, 0);
    
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "DefBulkSize", DEF_BULK_SIZE_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_DEF_BULK_SIZE), BM_SETCHECK, truncdata, 0);

    truncdata = wyIni::IniGetInt(GENERALPREFA, "TableDataUnderQuery", TABLEDATA_POS_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_TABLEDATAPOSITION), BM_SETCHECK, truncdata, 0);

    truncdata = wyIni::IniGetInt(GENERALPREFA, "InfoTabUnderQuery", INFOTAB_POS_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_INFOPOSITION), BM_SETCHECK, truncdata, 0);

    truncdata = wyIni::IniGetInt(GENERALPREFA, "HistoryUnderQuery", HISTORY_POS_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_HISTORYPOSITION), BM_SETCHECK, truncdata, 0);

	EnableBulkInsert(m_hwnd, truncdata);

    GetGeneralPrefSizeValues(m_hwnd);

    return;
}

void 
PreferenceBase::EnableRowLimit(HWND hwnd, wyInt32 enable)
{
    
	EnableWindow(GetDlgItem(hwnd, IDC_ROW_LIMIT), enable);
	/*if(enable)
		EnableWindow(GetDlgItem(hwnd, IDC_ROW_LIMIT), FALSE);
	else
		EnableWindow(GetDlgItem(hwnd, IDC_ROW_LIMIT), TRUE);*/
}

void 
PreferenceBase::EnableBulkInsert(HWND hwnd, wyInt32 enable)
{
	if(enable)
		EnableWindow(GetDlgItem(hwnd, IDC_BULKINSERT), FALSE);
	else
		EnableWindow(GetDlgItem(hwnd, IDC_BULKINSERT), TRUE);
}

void 
PreferenceBase::GetGeneralPrefSizeValues(HWND hwnd)
{
    wyInt32	    bulksize = 0;
	wyWChar     limit[32] = {0};
	wyString	dirstr(m_directory);
    	
	bulksize	= wyIni::IniGetInt(GENERALPREFA, "RowLimit", ROW_LIMIT_DEFAULT, dirstr.GetString());
	_snwprintf(limit, 31, L"%d", bulksize);
	SetWindowText(GetDlgItem(hwnd, IDC_ROW_LIMIT), limit);
	
	bulksize	= wyIni::IniGetInt(GENERALPREFA, "BulkSize", BULKINSERT_DEFAULT, dirstr.GetString());
	_snwprintf(limit, 31, L"%d", bulksize);
	SetWindowText(GetDlgItem(hwnd, IDC_BULKINSERT), limit);
}

void
PreferenceBase::InitFontPrefValues()
{
	wyInt32	    px, height, tabsize;
	HDC	        hdc;
    wyString    buff, fontnamestr, dirstr(m_directory);
	wyString	casestring;
	wyInt32		ret;
    wyInt32     isinsertspaces;

	FillCaseCombo(GetDlgItem(m_hwnd, IDC_KEYWORDCASE));
	FillCaseCombo(GetDlgItem(m_hwnd, IDC_FUNCTIONCASE));

	//To limit the hard spaces in tab size to two digits
	SendMessage(GetDlgItem(m_hwnd, IDC_TAB), EM_LIMITTEXT, 2, 0);

	VERIFY(hdc = GetDC(m_hwnd));
	height = GetDeviceCaps(hdc, LOGPIXELSY);

	if(m_isrestorealldefaults == wyTrue)//if user select Restore all defaults, then we need to set default values for all tabs 
	{
		SetDefaultFontDetails(m_hwnd);
		return;
	}

	// set the font details in the static control.
	SetFontDetails();
	
	//InsertTab();
	
	//VERIFY(m_hwndfonttab = GetDlgItem(m_hwnd, IDC_FONTEDITTAB));
	
	px = (wyInt32)(-m_editfont.lfHeight * 72.0 / height + 0.5);
    //px = -MulDiv(m_editfont.lfHeight, height, 72);
	fontnamestr.SetAs(m_editfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_SQLFONT), buff.GetAsWideChar());

	px =(wyInt32)(-m_datafont.lfHeight * 72.0 / height + 0.5);
    //px = -MulDiv(m_datafont.lfHeight, height, 72);
	fontnamestr.SetAs(m_datafont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_DATAFONT), buff.GetAsWideChar());

	px =(wyInt32)(-m_historyfont.lfHeight * 72.0 / height + 0.5);
    //px = -MulDiv(m_historyfont.lfHeight, height, 72);
	fontnamestr.SetAs(m_historyfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_HISTORYFONT), buff.GetAsWideChar());

    px =(wyInt32)(-m_obfont.lfHeight * 72.0 / height + 0.5);
	fontnamestr.SetAs(m_obfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_OBFONT), buff.GetAsWideChar());

	px =(wyInt32)(-m_blobfont.lfHeight * 72.0 / height + 0.5);
    //px = -MulDiv(m_blobfont.lfHeight, height, 72);
	fontnamestr.SetAs(m_blobfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString() , px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_BLOBFONT), buff.GetAsWideChar());

	//Keyword case
	wyIni::IniGetString(GENERALPREFA, "KeywordCase", KEYWORDCASE_DEFAULT, &casestring, dirstr.GetString());
	ret = SendMessage(GetDlgItem(m_hwnd, IDC_KEYWORDCASE), CB_SELECTSTRING, -1, (LPARAM)casestring.GetAsWideChar());
	if(ret == CB_ERR)
		SendMessage(GetDlgItem(m_hwnd, IDC_KEYWORDCASE), CB_SETCURSEL, 0,(LPARAM)0);

	//Function case
	wyIni::IniGetString(GENERALPREFA, "FunctionCase", FUNCTIONCASE_DEFAULT, &casestring,dirstr.GetString());
	ret = SendMessage(GetDlgItem(m_hwnd, IDC_FUNCTIONCASE), CB_SELECTSTRING, -1, (LPARAM)casestring.GetAsWideChar());
	if(ret == CB_ERR)
		SendMessage(GetDlgItem(m_hwnd, IDC_FUNCTIONCASE), CB_SETCURSEL, 0,(LPARAM)0);

    tabsize	= wyIni::IniGetInt(GENERALPREFA, "TabSize", DEF_TAB_SIZE, dirstr.GetString());
	buff.Sprintf("%d", tabsize);
	SetWindowText(GetDlgItem(m_hwnd, IDC_TAB), buff.GetAsWideChar());

    //Insert Spaces for Tabs 
    isinsertspaces	= wyIni::IniGetInt(GENERALPREFA, "InsertSpacesForTab", INSERTSPACES_DEFAULT, dirstr.GetString());
  	SendMessage(GetDlgItem(m_hwnd, IDC_INSERTSPACES), BM_SETCHECK, (isinsertspaces == 1)?1:0 , 0);
    SendMessage(GetDlgItem(m_hwnd, IDC_KEEPTABS), BM_SETCHECK, (isinsertspaces == 0)?1:0, 0);

	ReleaseDC(m_hwnd, hdc);
}

void
PreferenceBase::SetFontDetails()
{
    HFONT hfont = GetStockFont(DEFAULT_GUI_FONT);

	FillEditFont(&m_editfont, m_hwnd);
	FillEditFont(&m_datafont, m_hwnd);
	FillEditFont(&m_historyfont, m_hwnd);
    GetObject(hfont, sizeof(m_obfont), &m_obfont);
	FillEditFont(&m_blobfont, m_hwnd);

    GetFontDetails(TEXT(EDITFONT), &m_editfont);
	GetFontDetails(TEXT(HISTORYFONT), &m_historyfont);
    GetFontDetails(TEXT(OBFONT), &m_obfont, "");
	GetFontDetails(TEXT(BLOBFONT), &m_blobfont);
    GetFontDetails(TEXT(DATAFONT), &m_datafont);
		
	PrintFontDetails();
    DeleteObject(hfont);

	return;
}

void
PreferenceBase::GetFontDetails(wyWChar *appname, PLOGFONT font, wyChar* deffontface)
{
    HDC         hdc;
    wyInt32     height, px;
	wyString	dirstr, fontnamestr, appnamestr(appname);
		
	VERIFY(hdc = GetDC(m_hwnd));
	height = GetDeviceCaps(hdc, LOGPIXELSY);
	
	dirstr.SetAs(m_directory);

	wyIni::IniGetString(appnamestr.GetString(), "FontName", deffontface ? deffontface : FONTNAME_DEFAULT, &fontnamestr, dirstr.GetString());

    if(!fontnamestr.GetLength())
    {
        return;
    }
	
	wcscpy(font->lfFaceName, fontnamestr.GetAsWideChar());

	px = wyIni::IniGetInt(appnamestr.GetString(), "FontSize", FONTSIZE_DEFAULT, dirstr.GetString()); 	
	font->lfHeight =(wyInt32)((- px) * height / 72.0);
	font->lfWeight = wyIni::IniGetInt(appnamestr.GetString(), "FontStyle", FONTSTYLE_DEFAULT, dirstr.GetString()); 	
	font->lfItalic = wyIni::IniGetInt(appnamestr.GetString(), "FontItalic", FONTITALIC_DEFAULT, dirstr.GetString()); 	
	font->lfCharSet = wyIni::IniGetInt(appnamestr.GetString(), "FontCharSet", DEFAULT_CHARSET, dirstr.GetString()); 	

    ReleaseDC(m_hwnd, hdc);
}

// function initializes the choosefont structure and opens up the choosefont dialog.
wyBool 
PreferenceBase::ChFont(PLOGFONT lf)
{
	CHOOSEFONT		cf = {0};
	LOGFONT			templf;
	wyBool			chfontret = wyTrue;;

	templf = *lf;

	cf.lStructSize	= sizeof(cf);
	cf.hwndOwner	= m_hwnd;
	cf.lpLogFont	= &templf;
	cf.Flags		= CF_NOVECTORFONTS | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

	chfontret = (wyBool) ChooseFont(&cf);
	if(chfontret == wyTrue)
		*lf = templf;

	return chfontret;
}


/* shows the font details in the two static text boxes. */
void	
PreferenceBase::PrintFontDetails()
{
	wyString    fontdetail, fontnamestr;
    wyInt32     px;
	
    px = GetFontSize(&m_editfont);
	fontnamestr.SetAs(m_editfont.lfFaceName);
	fontdetail.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_SQLFONT), fontdetail.GetAsWideChar());

	px = GetFontSize(&m_datafont);
	fontnamestr.SetAs(m_datafont.lfFaceName);
	fontdetail.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_DATAFONT), fontdetail.GetAsWideChar());
			
	px = GetFontSize(&m_historyfont);
	fontnamestr.SetAs(m_historyfont.lfFaceName);
	fontdetail.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_HISTORYFONT), fontdetail.GetAsWideChar());

    px = GetFontSize(&m_obfont);
	fontnamestr.SetAs(m_obfont.lfFaceName);
	fontdetail.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_OBFONT), fontdetail.GetAsWideChar());

	px = GetFontSize(&m_blobfont);
	fontnamestr.SetAs(m_blobfont.lfFaceName);
	fontdetail.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(m_hwnd, IDC_BLOBFONT), fontdetail.GetAsWideChar());

	return;
}

wyInt32 	
PreferenceBase::GetFontSize(PLOGFONT font)
{
    wyInt32     px, height;
	HDC	        hdc;

	VERIFY(hdc = GetDC(m_hwnd));
	height = GetDeviceCaps(hdc, LOGPIXELSY);

    px =(wyInt32)(-font->lfHeight * 72.0 / height + 0.5);
    ReleaseDC(m_hwnd, hdc);

    return px;
}
	
// initialize the Others dialog page with its default values.
void
PreferenceBase::InitOthersPrefValues()
{
	wyInt32	    truncdata = 0, ret;
	wyString	dirstr(m_directory);
	wyString	tempstr;

	// fills Iconsize combo box
	FillSizeCombo(GetDlgItem(m_hwnd, IDC_ICONSIZE));
    FillThemeCombo(GetDlgItem(m_hwnd, IDC_THEMECOMBO));

	if(m_isrestorealldefaults == wyTrue)
	{
		SetOthersPrefDefaultValues(m_hwnd);
		return;
	}

	//Added an option for Retaining  user modified column width
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "RetainColumnWidth", RETAINCOLUMNWIDTH_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_RETAINCOLUMNWIDTH), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "TruncData", COLUMNWIDTH_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_COLUMNWIDTH), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "RefreshTableData", REFRESHTABLEDATA_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_REFRESHTABLEDATA), BM_SETCHECK,truncdata, 0);

	/*truncdata	= wyIni::IniGetInt(GENERALPREFA, "GetInfoAlways", GETINFOALWAYS_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_GETINFOALWAYS), BM_SETCHECK, truncdata, 0);*/

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "SmartKeyword", GETINFOKW_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_GETINFOKW), BM_SETCHECK, truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "SwitchShortcut", SWITCHSHORTCUT_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_SWITCHSHORTCUT), BM_SETCHECK,truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "OBOpenTablesByDefault", OPENTABLES_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_OPENTABLES), BM_SETCHECK,truncdata, 0);

	truncdata	= wyIni::IniGetInt(GENERALPREFA, "SessionRestore", CONRESTORE_DEFAULT, dirstr.GetString());
    SendMessage(GetDlgItem(m_hwnd, IDC_CONRESTORE), BM_SETCHECK, truncdata, 0);

	truncdata   = wyIni::IniGetInt(GENERALPREFA, "UpdateCheck", ENABLEUPGRADE_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_ENABLEUPGRADE), BM_SETCHECK, truncdata, 0);
	
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "PromptUpdate", UPDATEPROMPT_DEFAULT, dirstr.GetString());
	SendMessage(GetDlgItem(m_hwnd, IDC_UPDATEPROMPT), BM_SETCHECK, truncdata, 0);

    // Iconsize combo box
	wyIni::IniGetString(GENERALPREFA, "ToolIconSize", TOOLBARICONSIZE_DEFAULT, &tempstr, dirstr.GetString());
	ret = SendMessage(GetDlgItem(m_hwnd, IDC_ICONSIZE), CB_SELECTSTRING, -1, (LPARAM)tempstr.GetAsWideChar());
	if(ret == CB_ERR)
		SendMessage(GetDlgItem(m_hwnd, IDC_ICONSIZE), CB_SETCURSEL, 1,(LPARAM)0);

	GetOthersPrefSizeValues(m_hwnd);

	if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_ULTIMATE && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_TRIAL)
	{
		SetWindowText(GetDlgItem(m_hwnd, IDC_TRANSACTION), _(L"Transaction options (Enterprise/Ultimate only)"));
		EnableWindow(GetDlgItem(m_hwnd, IDC_PROMPTTRANSACTION), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_PROMPTCLOSETRANSACTION), FALSE);
		return;
	}
	else
	{
		truncdata	= wyIni::IniGetInt(GENERALPREFA, "PromptinTransaction", PROMPTTRANSACTION_DEFAULT, dirstr.GetString());
		SendMessage(GetDlgItem(m_hwnd, IDC_PROMPTTRANSACTION), BM_SETCHECK, truncdata, 0);
		truncdata	= wyIni::IniGetInt(GENERALPREFA, "PromptinTransactionClose", PROMPTTRANSACTIONCLOSE_DEFAULT, dirstr.GetString());
		SendMessage(GetDlgItem(m_hwnd, IDC_PROMPTCLOSETRANSACTION), BM_SETCHECK, truncdata, 0);
	}

    return;
}

void 
PreferenceBase::FillThemeCombo(HWND hwnd)
{
    LPTHEMEINFO pthemeinfo;
    wyInt32 count, i, index;
    THEMEINFO* pactivetheme;

    count = wyTheme::GetThemes(&pthemeinfo);
    pactivetheme = wyTheme::GetActiveThemeInfo();
    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

    for(i = count - 1; i >= 0; --i)
    {
        if(i == 0)
        {
            index = SendMessage(hwnd, CB_INSERTSTRING, 0, (LPARAM)(pthemeinfo + i)->m_name.GetAsWideChar());
        }
        else
        {
            index = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(pthemeinfo + i)->m_name.GetAsWideChar());
        }

        SendMessage(hwnd, CB_SETITEMDATA, index, i);

        if(!pactivetheme)
        {
            if((pthemeinfo + i)->m_type == NO_THEME)
            {
                SendMessage(hwnd, CB_SETCURSEL, index, 0);
            }
        }
        else
        {
            if(!(pthemeinfo + i)->m_filename.CompareI(pactivetheme->m_filename) &&
                (pthemeinfo + i)->m_type == pactivetheme->m_type)
            {
                SendMessage(hwnd, CB_SETCURSEL, index, 0);
            }
        }
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pthemeinfo);
}

void 
PreferenceBase::GetOthersPrefSizeValues(HWND hwnd)
{
    wyInt32	    setpage = 0, numrows = 0, retainpage = 0;
	wyWChar     limit[32] = {0};
	wyString	dirstr(m_directory);

	//Result tab paging option
	setpage	= wyIni::IniGetInt(GENERALPREFA, "ResuttabPaging", RESULTTABPAGE_DEFAULT, dirstr.GetString());
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_RESUTTABPAGE), setpage);
	
    //Resulttab No. of rows 
	numrows	= wyIni::IniGetInt(GENERALPREFA, "ResulttabPageRows", HIGHLIMIT_DEFAULT, dirstr.GetString());
	
	//Sets to def. value if its zero
	if(!numrows)
		numrows = HIGHLIMIT_DEFAULT;

	_snwprintf(limit, 31, L"%d", numrows);

	SetWindowText(GetDlgItem(m_hwnd, IDC_HIGHLIMIT), limit); 

	SendMessage(GetDlgItem(m_hwnd, IDC_HIGHLIMIT), EM_LIMITTEXT, 6, 0);

	//RetainPage size option
	retainpage = wyIni::IniGetInt(GENERALPREFA, "ResuttabRetainsPage", RESULTTABPAGE_DEFAULT, dirstr.GetString());
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_RESUTTABPAGERETAIN), retainpage);
		
	if(setpage)
	{
		pGlobals->m_resuttabpageenabled = wyTrue;		
		
		pGlobals->m_retainpagevalue = (retainpage ? wyTrue : wyFalse);

		//EnableWindow(GetDlgItem(m_hwnd, IDC_HIGHLIMIT), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_RESUTTABPAGERETAIN), TRUE);
	}
        
	else
	{
		pGlobals->m_resuttabpageenabled = wyFalse;
		pGlobals->m_retainpagevalue = wyFalse;

        //EnableWindow(GetDlgItem(m_hwnd, IDC_HIGHLIMIT), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_RESUTTABPAGERETAIN), FALSE);
	}
}

wyBool 
PreferenceBase::SaveGeneralPreferences(HWND hwndbase, wyInt32 page)
{
    HWND	hwnd;

    // gets the handle to the General preferences page
	VERIFY(hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page));

    if(!hwnd)
	{	if(m_isrestorealldefaults == wyTrue)//if user clicks Restore all defaults, then we write default values for all the tabs.
			SaveDefaultGeneralPreferences();
        return wyFalse;
	}

    SetBoolProfileString(hwnd, GENERALPREF, L"AppendBackQuotes", IDC_BACKQUOTES);
	SetBoolProfileString(hwnd, GENERALPREF, L"FocusOnEdit", IDC_FOCUSAFTERQUERY);
	SetBoolProfileString(hwnd, GENERALPREF, L"PromptOnTabClose", IDC_CONFIRMONTABCLOSE);
	SetBoolProfileString(hwnd, GENERALPREF, L"GetTextOnDBClick", IDC_GETTEXTONDBCLICK);
	SetBoolProfileString(hwnd, GENERALPREF, L"WordWrap", IDC_WORDWRAP);
	SetBoolProfileString(hwnd, GENERALPREF, L"JsonFormat", IDC_JSON);

	// for supporting transaction
	SetBoolProfileString(hwnd, GENERALPREF, L"StartTransaction", IDC_TRANSACTIONENABLE );
    
    SetBoolProfileString(hwnd, GENERALPREF, L"SetMaxRow", IDC_SETMAXROW);
	SetIntProfileString(hwnd, GENERALPREF, L"RowLimit", IDC_ROW_LIMIT);

	SetIntProfileString(hwnd, GENERALPREF, L"BulkSize", IDC_BULKINSERT);
    SetBoolProfileString(hwnd, GENERALPREF, L"DefBulkSize", IDC_DEF_BULK_SIZE);	

	SetBoolProfileString(hwnd, GENERALPREF, L"FKcheckImport", IDC_FKCHKIMPORT);

	SetBoolProfileString(hwnd, GENERALPREF, L"ShowWarnings", IDC_SHOWWARNING);
	SetBoolProfileString(hwnd, GENERALPREF, L"HaltExecutionOnError", IDC_HALTEXEC);
    SetBoolProfileString(hwnd, GENERALPREF, L"TableDataUnderQuery", IDC_TABLEDATAPOSITION);
    SetBoolProfileString(hwnd, GENERALPREF, L"InfoTabUnderQuery", IDC_INFOPOSITION);
    SetBoolProfileString(hwnd, GENERALPREF, L"HistoryUnderQuery", IDC_HISTORYPOSITION);
    
    return wyTrue;
}


wyBool 
PreferenceBase::SaveFontPreferences(HWND hwndbase, wyInt32 page)
{
    HWND		hwnd;
	wyString	dirstr(m_directory);
	wyInt32		index;
	wyWChar		keywordcase[SIZE_1024]={0};
	wyString	keywordcasestr;
    wyInt32     isinsertspaces = 0;
	
    hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page);

	if(!hwnd)
	{   if(m_isrestorealldefaults == wyTrue) //if user clicks Restore all defaults, then we write default values for all the tabs.
			SaveDefaultEditorPreferences();
		return wyFalse;
	}

    SetFontPrefDetails(hwnd, TEXT(EDITFONT), &m_editfont);
    SetFontPrefDetails(hwnd, TEXT(DATAFONT), &m_datafont);
	SetFontPrefDetails(hwnd, TEXT(HISTORYFONT), &m_historyfont);
    SetFontPrefDetails(hwnd, TEXT(OBFONT), &m_obfont);
	SetFontPrefDetails(hwnd, TEXT(BLOBFONT), &m_blobfont);
    SetColorPrefDetails();

	VERIFY((index	= SendMessage(GetDlgItem(hwnd, IDC_KEYWORDCASE), CB_GETCURSEL, 0, 0)) != CB_ERR); 
	SendMessage(GetDlgItem(hwnd, IDC_KEYWORDCASE),CB_GETLBTEXT, index, (LPARAM)keywordcase);
	keywordcasestr.SetAs(keywordcase);
	wyIni::IniWriteString(GENERALPREFA, "KeywordCase", keywordcasestr.GetString(), dirstr.GetString());
	
	VERIFY((index	= SendMessage(GetDlgItem(hwnd, IDC_FUNCTIONCASE), CB_GETCURSEL, 0, 0)) != CB_ERR); 
	SendMessage(GetDlgItem(hwnd,IDC_FUNCTIONCASE),CB_GETLBTEXT, index, (LPARAM)keywordcase);
	keywordcasestr.SetAs(keywordcase);
	wyIni::IniWriteString(GENERALPREFA, "FunctionCase", keywordcasestr.GetString(), dirstr.GetString());
   
	SetIntProfileString(hwnd, GENERALPREF, L"TabSize", IDC_TAB);

    //spaces for tabs 
    if(SendMessage(GetDlgItem(hwnd, IDC_INSERTSPACES), BM_GETCHECK, 0, 0) == BST_CHECKED)
        isinsertspaces = 1;

    wyIni::IniWriteInt(GENERALPREFA, "InsertSpacesForTab", isinsertspaces, dirstr.GetString());

    return wyTrue;
}

wyBool 
PreferenceBase::SaveOthersPreferences(HWND hwndbase, wyInt32 page)
{
    HWND	    hwnd, hwndcombo;
	wyString	dirstr(m_directory);
    LPTHEMEINFO pthemeinfo, pactivetheme;
    wyInt32     i;

    // gets the handle to the General preferences page
	VERIFY(hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page));

    if(!hwnd)
	{	if(m_isrestorealldefaults == wyTrue) //if user clicks Restore all defaults, then we write default values for all the tabs.
			SaveDefaultOthersPreferences();

		return wyFalse;
	}

	//Added an option for Retaining  user modified column width
	SetBoolProfileString(hwnd, GENERALPREF, L"RetainColumnWidth", IDC_RETAINCOLUMNWIDTH);
    SetBoolProfileString(hwnd, GENERALPREF, L"TruncData", IDC_COLUMNWIDTH);
	SetBoolProfileString(hwnd, GENERALPREF, L"RefreshTableData", IDC_REFRESHTABLEDATA);
	//SetBoolProfileString(hwnd, GENERALPREF, L"GetInfoAlways", IDC_GETINFOALWAYS);
	SetBoolProfileString(hwnd, GENERALPREF, L"SmartKeyword", IDC_GETINFOKW);

	//For interchanging F5 & F9 functionalities
	SetBoolProfileString(hwnd, GENERALPREF, L"SwitchShortcut", IDC_SWITCHSHORTCUT);

	//when we are expanding a database, whether we need to expand Tables folderor not 
	SetBoolProfileString(hwnd, GENERALPREF, L"OBOpenTablesByDefault", IDC_OPENTABLES);
	
	// for enabling connection restore
	SetBoolProfileString(hwnd, GENERALPREF, L"SessionRestore", IDC_CONRESTORE);

	//enable implicity upgrade check if it is checck
	SetBoolProfileString(hwnd, GENERALPREF, L"UpdateCheck", IDC_ENABLEUPGRADE);

	SetBoolProfileString(hwnd, GENERALPREF, L"PromptUpdate", IDC_UPDATEPROMPT);
#ifndef COMMUNITY
	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
	{
	SetBoolProfileString(hwnd, GENERALPREF, L"PromptinTransaction", IDC_PROMPTTRANSACTION);

	SetBoolProfileString(hwnd, GENERALPREF, L"PromptinTransactionClose", IDC_PROMPTCLOSETRANSACTION);
	}/* starting from 4.0 RC1 we support lowlimit and high limit also */
#endif    
    SetIntProfileString(hwnd, GENERALPREF, L"ResulttabPageRows", IDC_HIGHLIMIT);
	SetBoolProfileString(hwnd, GENERALPREF, L"ResuttabPaging", IDC_RESUTTABPAGE);
	SetBoolProfileString(hwnd, GENERALPREF, L"ResuttabRetainsPage", IDC_RESUTTABPAGERETAIN);

	// Iconsize combo box
	SetIntProfileString(hwnd, GENERALPREF, L"ToolIconSize", IDC_ICONSIZE);
    
    hwndcombo = GetDlgItem(hwnd, IDC_THEMECOMBO);

    if((pthemeinfo = (LPTHEMEINFO)GetWindowLongPtr(hwndcombo, GWLP_USERDATA)))
    {
        pactivetheme = wyTheme::GetActiveThemeInfo();
        i = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);
        i = SendMessage(hwndcombo, CB_GETITEMDATA, i, 0);
            
        if((!pactivetheme && (pthemeinfo + i)->m_type != NO_THEME) ||
            (pactivetheme && pactivetheme->m_filename.CompareI((pthemeinfo + i)->m_filename)/* && pactivetheme->m_type != (pthemeinfo + i)->m_type)*/))//fixing issue--user was not able to switch between same theme type.
        {
            m_isthemechanged = wyTrue;
            wyTheme::GetSetThemeInfo(SET_THEME, pthemeinfo + i);
        }
    }
        
    SetToolBarIconSize();  
    return wyTrue;
}

wyBool
PreferenceBase::SetToolBarIconSize()
{
	wyString	dirstr(m_directory);
	wyString	iconsize;
	wyString	tempiconsize;
	MDIWindow   *pcquerywnd = NULL;
	
	wyIni::IniGetString(GENERALPREFA, "ToolIconSize", TOOLBARICONSIZE_DEFAULT, &iconsize, dirstr.GetString());
	
	//Get the current index of combo as per the icon size value
	switch(pGlobals->m_pcmainwin->m_toolbariconsize)
	{
	case ICON_SIZE_32:
		tempiconsize.SetAs("Large");
		break;

	case ICON_SIZE_28:
		tempiconsize.SetAs("Normal");
		break;

	case ICON_SIZE_24:
		tempiconsize.SetAs("Small");
		break;

	default:
		tempiconsize.SetAs("Large");

	}

	//check the combo value change before saving. if so set the new icon size and call for resize
	if(iconsize.CompareI(tempiconsize) != 0)
	{
		if(iconsize.CompareI("Large") == 0)
			pGlobals->m_pcmainwin->m_toolbariconsize = ICON_SIZE_32;
		else if(iconsize.CompareI("Small") == 0)
			pGlobals->m_pcmainwin->m_toolbariconsize = ICON_SIZE_24;
		else
			pGlobals->m_pcmainwin->m_toolbariconsize = ICON_SIZE_28;
	
		//Re-arrange the tool bar with this new icon size
		pGlobals->m_pcmainwin->ReArranageToolBar();
	}

	//Setting the Db-combo itemm
	VERIFY(pcquerywnd = GetActiveWin());
	if(pcquerywnd && pcquerywnd->m_pcqueryobject && pcquerywnd->m_pcqueryobject->m_seldatabase.GetLength())
		pGlobals->m_pcmainwin->AddTextInCombo(pcquerywnd->m_pcqueryobject->m_seldatabase.GetAsWideChar());

	return wyTrue;
}

wyBool  
PreferenceBase::SetBoolProfileString(HWND hwnd, wyWChar *appname, wyWChar *keyname, wyInt32 id)
{
	wyString	appnamestr(appname), keynamestr(keyname), dirstr(m_directory);
	wyInt32		status = 0;

	if(SendMessage(GetDlgItem(hwnd, id), BM_GETCHECK, 0, 0) == BST_CHECKED)
		status = 1;

	else
		status = 0;

	wyIni::IniWriteInt(appnamestr.GetString(), keynamestr.GetString(), status, dirstr.GetString());

	switch(id)
	{
#ifndef COMMUNITY
	case IDC_PROMPTTRANSACTION:
		pGlobals->m_pcmainwin->m_topromptonimplicit = (status ? wyTrue : wyFalse);
		break;

	case IDC_PROMPTCLOSETRANSACTION:
		pGlobals->m_pcmainwin->m_topromptonclose = (status ? wyTrue : wyFalse);
		break;
#endif
	case IDC_RESUTTABPAGE:
		pGlobals->m_resuttabpageenabled = (status ? wyTrue : wyFalse);
		break;

	case IDC_RESUTTABPAGERETAIN:
		pGlobals->m_retainpagevalue = (status && pGlobals->m_resuttabpageenabled == wyTrue ? wyTrue : wyFalse);
		break;

    case IDC_INFOPOSITION:
        pGlobals->m_isinfotabunderquery = (status ? wyTrue : wyFalse);
        break;

    case IDC_TABLEDATAPOSITION:
        pGlobals->m_istabledataunderquery = (status ? wyTrue : wyFalse);
        break;

    case IDC_HISTORYPOSITION:
        pGlobals->m_ishistoryunderquery = (status ? wyTrue : wyFalse);
        break;

	case IDC_CONRESTORE:
		pGlobals->m_sessionrestore = (status ? wyTrue : wyFalse);
        break;

	default:
		return (status ? wyTrue : wyFalse);
	}

	return (status ? wyTrue : wyFalse);
}

wyInt32 
PreferenceBase::SetIntProfileString(HWND hwnd, wyWChar *appname, wyWChar *keyname, wyInt32 id)
{
    wyWChar		limit[16] = {0};
    wyInt32		txtlen = 0;
	wyString	appnamestr(appname), keynamestr(keyname), limitstr, dirstr(m_directory);

	txtlen = GetWindowText(GetDlgItem(hwnd, id), limit, 15);
	limitstr.SetAs(limit);
	wyIni::IniWriteString(appnamestr.GetString(), keynamestr.GetString(), limitstr.GetString(), dirstr.GetString());

	switch(id)
	{
	case IDC_HIGHLIMIT:
		if(limitstr.GetAsInt32() == 0)//reset to default value if its zero
		{
			limitstr.SetAs("1000");		
			SetWindowText(GetDlgItem(hwnd, id), limitstr.GetAsWideChar());
		}
		pGlobals->m_highlimitglobal = limitstr.GetAsInt32();		
		break;

	default:
		txtlen = GetWindowText(GetDlgItem(hwnd, id), limit, 15);
		limitstr.SetAs(limit);
		wyIni::IniWriteString(appnamestr.GetString(), 
							  keynamestr.GetString(), 
							  limitstr.GetString(), 
							  dirstr.GetString());	

	}

    return txtlen;
}

void 
PreferenceBase::SetFontPrefDetails(HWND hwnd, wyWChar * appname, LPLOGFONT font)
{
    HDC			hdc;
    wyInt32		height, px;
    wyWChar		fontdetails[32] = {0};
	wyString	appnamestr(appname), fontnamestr, fontdetstr, dirstr;

    VERIFY(hdc = GetDC(hwnd));
	
	height = GetDeviceCaps(hdc, LOGPIXELSY);

	px =(wyInt32)(- font->lfHeight * 72.0 / height + 0.5);
	
	fontnamestr.SetAs(font->lfFaceName);
	dirstr.SetAs(m_directory);
	
	WritePrivateProfileSectionA(appnamestr.GetString(), "", dirstr.GetString());
	wyIni::IniWriteString(appnamestr.GetString(), "FontName", fontnamestr.GetString(), dirstr.GetString());

	_snwprintf(fontdetails, 31, L"%d", px);
	fontdetstr.SetAs(fontdetails);
	wyIni::IniWriteString(appnamestr.GetString(), "FontSize", fontdetstr.GetString(), dirstr.GetString());

	_snwprintf(fontdetails, 31, L"%d", font->lfWeight);
	fontdetstr.SetAs(fontdetails);
	wyIni::IniWriteString(appnamestr.GetString(), "FontStyle", fontdetstr.GetString(), dirstr.GetString());

	_snwprintf(fontdetails, 31, L"%d", font->lfItalic);
	fontdetstr.SetAs(fontdetails);
	wyIni::IniWriteString(appnamestr.GetString(), "FontItalic", fontdetstr.GetString(), dirstr.GetString());

	_snwprintf(fontdetails, 31, L"%d", font->lfCharSet);
	fontdetstr.SetAs(fontdetails);
	wyIni::IniWriteString(appnamestr.GetString(), "FontCharSet", fontdetstr.GetString(), dirstr.GetString());

    ReleaseDC(hwnd, hdc);
}

void 
PreferenceBase::SetColorPrefDetails()
{
    wyWChar		color[32] = {0};
	wyString	colorstr, dirstr(m_directory);
    TREEVIEWDATA *tvd=NULL;
    TVITEM tvi;
    HTREEITEM tvi_root=NULL,tvi_sibling=NULL;
    
    if(m_htv)
    {
        tvi_root=TreeView_GetRoot(m_htv);
        while(tvi_root)
        {
            tvi.mask=TVIF_PARAM;
            tvi.hItem=tvi_root;
            if(TreeView_GetItem(m_htv,&tvi))
            {
                tvd=(TREEVIEWDATA *)tvi.lParam;
                switch(tvd->style)
                {
                case EDITOR:
                    _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                colorstr.SetAs(color);
	                wyIni::IniWriteString(GENERALPREFA, "EditorBgColor",	colorstr.GetString(),	dirstr.GetString());
                    break;
                case CANVAS:
                    _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                colorstr.SetAs(color);
	                wyIni::IniWriteString(GENERALPREFA, "CanvasBgColor",	colorstr.GetString(),	dirstr.GetString());
                    break;
                case MTI:
                    _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                colorstr.SetAs(color);
	                wyIni::IniWriteString(GENERALPREFA, "MTIBgColor",	colorstr.GetString(),	dirstr.GetString());
                    _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                colorstr.SetAs(color);
	                wyIni::IniWriteString(GENERALPREFA, "MTIFgColor",	colorstr.GetString(),	dirstr.GetString());
                    break;
                }
            }
            tvi_sibling=TreeView_GetNextItem(m_htv,tvi_root,TVGN_CHILD);
            while(tvi_sibling)
            {
                tvi.mask=TVIF_PARAM;
                tvi.hItem=tvi_sibling;
                if(TreeView_GetItem(m_htv,&tvi))
                {
                    tvd=(TREEVIEWDATA *)tvi.lParam;
                    switch(tvd->style)
                    {
                    case EDITOR_DEFAULT:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "NormalColor",	colorstr.GetString(),	dirstr.GetString());
                    break;
                    case EDITOR_COMMENTS:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "CommentColor",	colorstr.GetString(),	dirstr.GetString());
                    break;
                    case EDITOR_STRINGS:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "StringColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_FUNCTIONS:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "FunctionColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_KEYWORDS:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "KeywordColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_NUMBER:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "NumberColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_OPERATOR:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "OperatorColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_HIDDENCOMMAND:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "HiddenCmdColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_LINENUMBERMARGIN:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "NumberMarginbackgroundColor",	colorstr.GetString(),	dirstr.GetString());
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "NumberMarginforegroundColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_FOLDINGMARGIN:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "FoldingMarginbackgroundColor",	colorstr.GetString(),	dirstr.GetString());
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "FoldingMarginFgColor",	colorstr.GetString(),	dirstr.GetString());
                        _snwprintf(color, 31, L"%d", m_rgbtexturecolor);
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "FoldingMarginTextureColor",	colorstr.GetString(),	dirstr.GetString());
                       
                    break;
                    case EDITOR_TEXTSELECTION:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "SelectionBgColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_BRACESMATCH:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "BraceLightBgColor",	colorstr.GetString(),	dirstr.GetString());
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "BraceLightFgColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case EDITOR_BRACESUNMATCH:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "BraceBadBgColor",	colorstr.GetString(),	dirstr.GetString());
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "BraceBadFgColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case CANVAS_LINE:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "CanvasLineColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case CANVAS_TEXT:
                        _snwprintf(color, 31, L"%d", *(tvd->fgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "CanvasTextColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    case MTI_SELECTION:
                        _snwprintf(color, 31, L"%d", *(tvd->bgColor));
	                    colorstr.SetAs(color);
	                    wyIni::IniWriteString(GENERALPREFA, "MTISelectionColor",	colorstr.GetString(),	dirstr.GetString());
                        
                    break;
                    }
                }
                tvi_sibling=TreeView_GetNextSibling(m_htv,tvi_sibling);
            }
            tvi_root=TreeView_GetNextSibling(m_htv,tvi_root);
        }
    }
}

/* callback function for enumerated.*/
BOOL CALLBACK
PreferenceBase::EnumChildProc(HWND hwnd, LPARAM lParam)
{
	MDIWindow*		wnd;
	wyWChar  		classname[SIZE_128]={0};
    PreferenceBase* pref = (PreferenceBase*)lParam;
    wyBool          isupdtabledata = wyTrue, isupdhistory = wyTrue, isupdinfo = wyTrue;
    
	VERIFY(GetClassName(hwnd, classname, SIZE_128-1));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
	{
		VERIFY(wnd =(MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        if(pref)
        {
            isupdtabledata = pref->m_istabledataunderquery == pGlobals->m_istabledataunderquery ? wyFalse : wyTrue;
            isupdinfo = pref->m_isinfotabunderquery == pGlobals->m_isinfotabunderquery ? wyFalse : wyTrue;
            isupdhistory = pref->m_ishistoryunderquery == pGlobals->m_ishistoryunderquery ? wyFalse : wyTrue;
        }

        wnd->PositionTabs(isupdtabledata, isupdhistory, isupdinfo);
 
        /*Change font for all tabs*/
		wnd->m_pctabmodule->SetTabFont();
		
		/*Change font color and case for all tabs */
        wnd->m_pctabmodule->SetTabFontColor();
        wnd->m_pctabmodule->SetBackQuotesOption();

        //..Setting Object-browser font
        wnd->m_pcqueryobject->SetFont();

		wnd->m_pctabmodule->Refresh();
	}

	return TRUE;
}

void
PreferenceBase::SetGenPrefDefaultValues(HWND hwnd)
{	
	SendMessage(GetDlgItem(hwnd, IDC_WORDWRAP), BM_SETCHECK, WORDWRAP_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_JSON), BM_SETCHECK, JSONFORMAT_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_BACKQUOTES), BM_SETCHECK, BACKQUOTES_DEFAULT	, 0);
	SendMessage(GetDlgItem(hwnd, IDC_FOCUSAFTERQUERY), BM_SETCHECK, FOCUSAFTERQUERY_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CONFIRMONTABCLOSE), BM_SETCHECK, CONFIRMONTABCLOSE_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_GETTEXTONDBCLICK), BM_SETCHECK, GETTEXTONDBCLICK_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_TRANSACTIONENABLE), BM_SETCHECK, TRANSACTIONENABLE_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SETMAXROW), BM_SETCHECK, SETMAXROW_DEFAULT, 0);
    EnableRowLimit(hwnd, !SETMAXROW_DEFAULT);
	SendMessage(GetDlgItem(hwnd, IDC_DEF_BULK_SIZE), BM_SETCHECK, DEF_BULK_SIZE_DEFAULT, 0);
    EnableBulkInsert(hwnd, DEF_BULK_SIZE_DEFAULT);

	SendMessage(GetDlgItem(hwnd, IDC_SHOWWARNING), BM_SETCHECK, SHOWWARNINGS_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_HALTEXEC), BM_SETCHECK, HALTEXEC_DEFAULT, 0);
    SendMessage(GetDlgItem(hwnd, IDC_TABLEDATAPOSITION), BM_SETCHECK, TABLEDATA_POS_DEFAULT, 0);
    SendMessage(GetDlgItem(hwnd, IDC_INFOPOSITION), BM_SETCHECK, INFOTAB_POS_DEFAULT, 0);
    SendMessage(GetDlgItem(hwnd, IDC_HISTORYPOSITION), BM_SETCHECK, HISTORY_POS_DEFAULT, 0);

	SetGeneralPrefDefaultSizeValues(hwnd);

	return;
}

void 
PreferenceBase::SetGeneralPrefDefaultSizeValues(HWND hwnd)
{    
	wyWChar     limit[32] = {0};
	    	
	_snwprintf(limit, 31, L"%d", ROW_LIMIT_DEFAULT);
	SetWindowText(GetDlgItem(hwnd, IDC_ROW_LIMIT), limit);
	
	_snwprintf(limit, 31, L"%d", BULKINSERT_DEFAULT);
	SetWindowText(GetDlgItem(hwnd, IDC_BULKINSERT), limit);

}

// initialize the Others dialog page with its default values.
void
PreferenceBase::SetOthersPrefDefaultValues(HWND hwnd)
{
	wyString iconsize,defaulttheme;

	//Added an option for Retaining  user modified column width
	SendMessage(GetDlgItem(hwnd, IDC_RETAINCOLUMNWIDTH), BM_SETCHECK, RETAINCOLUMNWIDTH_DEFAULT, 0);
    SendMessage(GetDlgItem(hwnd, IDC_COLUMNWIDTH), BM_SETCHECK, COLUMNWIDTH_DEFAULT, 0);
	//SendMessage(GetDlgItem(hwnd, IDC_GETINFOALWAYS), BM_SETCHECK, GETINFOALWAYS_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_GETINFOKW), BM_SETCHECK, GETINFOKW_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SWITCHSHORTCUT), BM_SETCHECK, SWITCHSHORTCUT_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_OPENTABLES), BM_SETCHECK, OPENTABLES_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CONRESTORE), BM_SETCHECK, CONRESTORE_DEFAULT, 0);
	SendMessage(GetDlgItem(hwnd, IDC_ENABLEUPGRADE), BM_SETCHECK, ENABLEUPGRADE_DEFAULT, 0);
		
	SendMessage(GetDlgItem(hwnd, IDC_REFRESHTABLEDATA), BM_SETCHECK, REFRESHTABLEDATA_DEFAULT, 0);
	
	SendMessage(GetDlgItem(hwnd, IDC_UPDATEPROMPT), BM_SETCHECK, UPDATEPROMPT_DEFAULT, 0);  
#ifndef community
	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
	{
	SendMessage(GetDlgItem(hwnd, IDC_PROMPTTRANSACTION), BM_SETCHECK, PROMPTTRANSACTION_DEFAULT, 0);  

	SendMessage(GetDlgItem(hwnd, IDC_PROMPTCLOSETRANSACTION), BM_SETCHECK, PROMPTTRANSACTIONCLOSE_DEFAULT, 0);
	}
#endif
	//Iconsize combo box
    iconsize.SetAs(TOOLBARICONSIZE_DEFAULT);
	//default theme
	defaulttheme.SetAs("Flat");
	SendMessage(GetDlgItem(hwnd, IDC_ICONSIZE), CB_SELECTSTRING, -1, (LPARAM)iconsize.GetAsWideChar());

	SendMessage(GetDlgItem(hwnd, IDC_THEMECOMBO), CB_SELECTSTRING, -1,(LPARAM) defaulttheme.GetAsWideChar());
   
	SetOthersPrefDefaultLimitValues(hwnd);
	
	return;
}

void 
PreferenceBase::SetOthersPrefDefaultLimitValues(HWND hwnd)
{
    wyInt32	    highlimit = 0;
	wyWChar     limit[32] = {0};
		
	highlimit	= HIGHLIMIT_DEFAULT;
	_snwprintf(limit, 31, L"%d", highlimit);
	SetWindowText(GetDlgItem(hwnd, IDC_HIGHLIMIT), limit); 

	highlimit = RESULTTABPAGE_DEFAULT;

	Button_SetCheck(GetDlgItem(hwnd, IDC_RESUTTABPAGE), BST_CHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_RESUTTABPAGERETAIN), BST_CHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_RESUTTABPAGERETAIN), TRUE);
}

void
PreferenceBase::SetDefaultFontDetails(HWND hwnd)
{
    wyInt32     px, tabsize;
	wyString	fontnamestr, buff;
	wyString    casedefault;
    HFONT       hfont = GetStockFont(DEFAULT_GUI_FONT);
	
    FillDefaultEditFont(&m_editfont, hwnd);
	FillDefaultEditFont(&m_datafont, hwnd);
	FillDefaultEditFont(&m_historyfont, hwnd);
    GetObject(hfont, sizeof(m_obfont), &m_obfont);
	FillDefaultEditFont(&m_blobfont, hwnd);		
	
    DeleteObject(hfont);

	px =  9;
    fontnamestr.SetAs(m_editfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(hwnd, IDC_SQLFONT), buff.GetAsWideChar());

	px =  9;
    fontnamestr.SetAs(m_datafont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(hwnd, IDC_DATAFONT), buff.GetAsWideChar());

	px =  9;
    fontnamestr.SetAs(m_historyfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(hwnd, IDC_HISTORYFONT), buff.GetAsWideChar());

    px =  9;
    fontnamestr.SetAs(m_obfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString(), px);
	SetWindowText(GetDlgItem(hwnd, IDC_OBFONT), buff.GetAsWideChar());

	px =  9;
    fontnamestr.SetAs(m_blobfont.lfFaceName);
	buff.Sprintf("%s, %dpx", fontnamestr.GetString() , px);
	SetWindowText(GetDlgItem(hwnd, IDC_BLOBFONT), buff.GetAsWideChar());

	//keyword and function case
	casedefault.SetAs(KEYWORDCASE_DEFAULT);
	SendMessage(GetDlgItem(m_hwnd, IDC_KEYWORDCASE), CB_SELECTSTRING, -1, (LPARAM)casedefault.GetAsWideChar());
   
	casedefault.SetAs(FUNCTIONCASE_DEFAULT);
	SendMessage(GetDlgItem(m_hwnd, IDC_FUNCTIONCASE), CB_SELECTSTRING, -1, (LPARAM)casedefault.GetAsWideChar());
	
    tabsize	= DEF_TAB_SIZE;
	buff.Sprintf("%d", tabsize);
	SetWindowText(GetDlgItem(hwnd, IDC_TAB), buff.GetAsWideChar());

    SendMessage(GetDlgItem(hwnd, IDC_INSERTSPACES), BM_SETCHECK, INSERTSPACES_DEFAULT, 0);
    SendMessage(GetDlgItem(hwnd, IDC_KEEPTABS), BM_SETCHECK, 1, 0);
    
	SetDefaultColorDetails(hwnd);
	InvalidateRect(hwnd,NULL,TRUE);
}

void
PreferenceBase::SetDefaultColorDetails(HWND hwnd)
{	
    TREEVIEWDATA *tvd=NULL;
    TVITEM tvi;
    HTREEITEM tvi_root=NULL,tvi_sibling=NULL;
    
    if(m_htv)
    {
        tvi_root=TreeView_GetRoot(m_htv);
        while(tvi_root)
        {
            tvi.mask=TVIF_PARAM;
            tvi.hItem=tvi_root;
            if(TreeView_GetItem(m_htv,&tvi))
            {
                tvd=(TREEVIEWDATA *)tvi.lParam;
                switch(tvd->style)
                {
                case EDITOR:
                    *(tvd->bgColor)=DEF_BKGNDEDITORCOLOR;
                    break;
                case CANVAS:
                    *(tvd->bgColor)=DEF_BKGNDCANVASCOLOR;
                    break;
                case MTI:
                    *(tvd->bgColor)=COLOR_WHITE;
                    *(tvd->fgColor)=RGB(0,0,0);
                    break;
                }
            }
            tvi_sibling=TreeView_GetNextItem(m_htv,tvi_root,TVGN_CHILD);
            while(tvi_sibling)
            {
                tvi.mask=TVIF_PARAM;
                tvi.hItem=tvi_sibling;
                if(TreeView_GetItem(m_htv,&tvi))
                {
                    tvd=(TREEVIEWDATA *)tvi.lParam;
                    switch(tvd->style)
                    {
                    case EDITOR_DEFAULT:
                        *(tvd->fgColor)=DEF_NORMALCOLOR;
                    break;
                    case EDITOR_COMMENTS:
                        *(tvd->fgColor)=DEF_COMMENTCOLOR;
                    break;
                    case EDITOR_STRINGS:
                        *(tvd->fgColor)=DEF_STRINGCOLOR;
                    break;
                    case EDITOR_FUNCTIONS:
                        *(tvd->fgColor)=DEF_FUNCTIONCOLOR;
                    break;
                    case EDITOR_KEYWORDS:
                        *(tvd->fgColor)=DEF_KEYWORDCOLOR;
                    break;
                    case EDITOR_NUMBER:
                        *(tvd->fgColor)=DEF_NORMALCOLOR;
                    break;
                    case EDITOR_OPERATOR:
                        *(tvd->fgColor)=DEF_OPERATORCOLOR;
                    break;
                    case EDITOR_HIDDENCOMMAND:
                        *(tvd->fgColor)=DEF_NORMALCOLOR;
                    break;
                    case EDITOR_LINENUMBERMARGIN:
                        *(tvd->bgColor)=DEF_MARGINNUMBER;
                        *(tvd->fgColor)=RGB(59,125,187);
                    break;
                    case EDITOR_FOLDINGMARGIN:
                        *(tvd->bgColor)=DEF_MARGINNUMBER;
                        *(tvd->fgColor)=RGB(0,0,0);
                        m_rgbtexturecolor=COLOR_WHITE;
                    break;
                    case EDITOR_TEXTSELECTION:
                        *(tvd->bgColor)=DEF_TEXTSELECTION;
                    break;
                    case EDITOR_BRACESMATCH:
                        *(tvd->bgColor)=DEF_BRACELIGHTBG;
                        *(tvd->fgColor)=DEF_BRACELIGHTFG;
                    break;
                    case EDITOR_BRACESUNMATCH:
                        *(tvd->bgColor)=DEF_BRACEBADBG;
                        *(tvd->fgColor)=DEF_BRACEBADFG;
                    break;
                    case CANVAS_LINE:
                        *(tvd->fgColor)=RGB(160,160,160);
                    break;
                    case CANVAS_TEXT:
                        *(tvd->fgColor)=GetSysColor(COLOR_GRAYTEXT);
                    break;
                    case MTI_SELECTION:
                        *(tvd->bgColor)=DEF_TEXTSELECTION;
                    break;
                    }
                }
                tvi_sibling=TreeView_GetNextSibling(m_htv,tvi_sibling);
            }
            tvi_root=TreeView_GetNextSibling(m_htv,tvi_root);
        }
    if(hwnd == NULL)
        SetColorsInStaticControl(m_hwnd);
    else
        SetColorsInStaticControl(hwnd);
    }
    return;
}

//Default Font for Edit window
void
PreferenceBase::FillDefaultEditFont(PLOGFONT logf, HWND hwnd)
{
	HDC		hdc;
	wyInt32 height;

    memset(logf, 0, sizeof(LOGFONT));

	VERIFY(hdc = GetDC(m_hwnd));
	height = GetDeviceCaps(hdc, LOGPIXELSY);
	
	logf->lfHeight		= (wyInt32)((- 9) * height / 72.0);;
	logf->lfWeight		= 0;
	wcscpy(logf->lfFaceName, TEXT(FONTNAME_DEFAULT));
	logf->lfItalic = FONTITALIC_DEFAULT;
	logf->lfCharSet = DEFAULT_CHARSET;
	
	ReleaseDC(m_hwnd, hdc);
	
	return;
}

void
PreferenceBase::SaveDefaultGeneralPreferences()
{	
	wyString	dirstr(m_directory);

	wyIni::IniWriteInt(GENERALPREFA, "WordWrap", WORDWRAP_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "JsonFormat", JSONFORMAT_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "AppendBackQuotes", BACKQUOTES_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "FocusOnEdit", FOCUSAFTERQUERY_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "PromptOnTabClose", CONFIRMONTABCLOSE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "GetTextOnDBClick", GETTEXTONDBCLICK_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "StartTransaction", TRANSACTIONENABLE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "SetMaxRow", SETMAXROW_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "BulkSize", DEF_BULK_SIZE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "RowLimit", ROW_LIMIT_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "DefBulkSize", BULKINSERT_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "FKcheckImport", FKCHKIMPORT_DEFAULT, dirstr.GetString());
    wyIni::IniWriteInt(GENERALPREFA, "TableDataUnderQuery", TABLEDATA_POS_DEFAULT, dirstr.GetString());
    wyIni::IniWriteInt(GENERALPREFA, "InfoTabUnderQuery", INFOTAB_POS_DEFAULT, dirstr.GetString());
    wyIni::IniWriteInt(GENERALPREFA, "HistoryUnderQuery", HISTORY_POS_DEFAULT, dirstr.GetString());
		
    return ;
}

void
PreferenceBase::SaveDefaultOthersPreferences()
{
	wyString    dirstr(m_directory);

	//Added an option for Retaining  user modified column width
	wyIni::IniWriteInt(GENERALPREFA, "RetainColumnWidth", RETAINCOLUMNWIDTH_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "TruncData", COLUMNWIDTH_DEFAULT, dirstr.GetString());
	//wyIni::IniWriteInt(GENERALPREFA, "GetInfoAlways", GETINFOALWAYS_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "SmartKeyword", GETINFOKW_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "SwitchShortcut", SWITCHSHORTCUT_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "OBOpenTablesByDefault", OPENTABLES_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "UpdateCheck", ENABLEUPGRADE_DEFAULT, dirstr.GetString());
	
	wyIni::IniWriteInt(GENERALPREFA, "RefreshTableData", REFRESHTABLEDATA_DEFAULT, dirstr.GetString());

	wyIni::IniWriteInt(GENERALPREFA, "PromptUpdate", UPDATEPROMPT_DEFAULT, dirstr.GetString());

	wyIni::IniWriteInt(GENERALPREFA, "PromptinTransaction", PROMPTTRANSACTION_DEFAULT, dirstr.GetString());

	wyIni::IniWriteInt(GENERALPREFA, "PromptinTransactionClose", PROMPTTRANSACTION_DEFAULT, dirstr.GetString());
		
	wyIni::IniWriteInt(GENERALPREFA, "ResulttabPageRows", HIGHLIMIT_DEFAULT, dirstr.GetString());
	pGlobals->m_highlimitglobal = HIGHLIMIT_DEFAULT;
	
	wyIni::IniWriteInt(GENERALPREFA, "ResuttabPaging", RESULTTABPAGE_DEFAULT, dirstr.GetString());
	pGlobals->m_resuttabpageenabled = wyTrue;
	
	wyIni::IniWriteInt(GENERALPREFA, "ResuttabRetainsPage", RESULTRETAINPAGE_DEFAULT, dirstr.GetString());
	pGlobals->m_retainpagevalue = wyTrue;
		
	wyIni::IniWriteString(GENERALPREFA, "ToolIconSize", TOOLBARICONSIZE_DEFAULT, dirstr.GetString());
	SetToolBarIconSize();

    if(wyTheme::GetActiveThemeInfo())
    {
        m_isthemechanged = wyTrue;
    }

    wyTheme::GetSetThemeInfo(SET_THEME, NULL);	
	return;
}

void
PreferenceBase::SaveDefaultEditorPreferences()
{
	wyString	dirstr(m_directory);
	
	SaveDefaultFontPreferences(TEXT(EDITFONT));
	SaveDefaultFontPreferences(TEXT(DATAFONT));
	SaveDefaultFontPreferences(TEXT(HISTORYFONT));
	SaveDefaultFontPreferences(TEXT(BLOBFONT));
	SetDefaultColorDetails();
	SetColorPrefDetails();

	wyIni::IniWriteString(GENERALPREFA, "KeywordCase", KEYWORDCASE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteString(GENERALPREFA, "FunctionCase", FUNCTIONCASE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(GENERALPREFA, "TabSize", DEF_TAB_SIZE, dirstr.GetString());
    wyIni::IniWriteInt(GENERALPREFA, "InsertSpacesForTab", INSERTSPACES_DEFAULT, dirstr.GetString());
}

void
PreferenceBase::SaveDefaultFontPreferences(wyWChar * appname)
{
	wyString	appnamestr(appname),dirstr;
	dirstr.SetAs(m_directory);

	WritePrivateProfileSectionA(appnamestr.GetString(), "", dirstr.GetString());
	wyIni::IniWriteString(appnamestr.GetString(), "FontName", FONTNAME_DEFAULT, dirstr.GetString());

	wyIni::IniWriteInt(appnamestr.GetString(), "FontSize", FONTSIZE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(appnamestr.GetString(), "FontStyle", FONTSTYLE_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(appnamestr.GetString(), "FontItalic", FONTITALIC_DEFAULT, dirstr.GetString());
	wyIni::IniWriteInt(appnamestr.GetString(), "FontCharSet", DEFAULT_CHARSET, dirstr.GetString());
}

void
PreferenceBase::SetGenPrefDefaultAllTabValues(HWND hwndbase, wyInt32 page)
{
	 HWND	hwnd;

    // gets the handle to the General preferences page
	VERIFY(hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page));

    if(!hwnd)
		return;
	
	SetGenPrefDefaultValues(hwnd);
	return;
}

void
PreferenceBase::SetFontPrefDefAllTabValues(HWND hwndbase, wyInt32 page)
{
	 HWND	hwnd;

    // gets the handle to the Editor preferences page
	VERIFY(hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page));

    if(!hwnd)
		return;

	SetDefaultFontDetails(hwnd);
	
	return;
}

void
PreferenceBase::SetOthersPrefDefaultAllTabValues(HWND hwndbase, wyInt32 page)
{
	 HWND	hwnd;

    // gets the handle to the Other preferences page
	VERIFY(hwnd = PropSheet_IndexToHwnd(GetParent(hwndbase), page));

    if(!hwnd)
		return;
	
	SetOthersPrefDefaultValues(hwnd);
	return;
}

//Fills Case combo
void 
PreferenceBase::FillCaseCombo(HWND hwndcombo)
{
	SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)L"Unchanged");
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)L"lowercase");
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)L"UPPERCASE");
}

void 
PreferenceBase::EnableOrDisable(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable)
{
    wyInt32 count;
    HWND	hwndc;

    for(count = 0; count < arraycount; ++count)
	{
		hwndc = GetDlgItem(hwnd, array[count]);
		if(hwndc)
        {
            if(enable == wyTrue)
                EnableWindow(hwndc, TRUE);
            else
                EnableWindow(hwndc, FALSE);
        }
	}
}

// Sets preview editor properties in Formatter tab
void
PreferenceBase::SetPreviewEditor(HWND hwnd)
{
	HWND		hwndpreview;
	
	hwndpreview = GetDlgItem(hwnd, IDC_FORMATTERPREVIEW);
	
	// attempt to set scintilla code page to support utf8 data
	SendMessage(hwndpreview, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

	SendMessage(hwndpreview, SCI_SETMARGINWIDTHN, 1, 0);

	GetAllScintillaKeyWordsAndFunctions(wyTrue, m_keywordstring);
	GetAllScintillaKeyWordsAndFunctions(wyFalse, m_functionstring);

    //set the lexer language 
	SendMessage(hwndpreview, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"MySQL");

	EditorFont::FormatEditor(hwndpreview, wyTrue, m_keywordstring, m_functionstring);

	SendMessage(hwndpreview, WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	
	/* make the scintilla preview control word wrap */
	SendMessage(hwndpreview, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);

}

//For filling IconSize ComboBox items
void 
PreferenceBase::FillSizeCombo(HWND hwndcombo)
{
	SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)_(L"Small"));
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)_(L"Normal"));
	SendMessage(hwndcombo, CB_INSERTSTRING, 0,(LPARAM)_(L"Large"));
}
