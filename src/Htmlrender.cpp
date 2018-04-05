#include "Htmlrender.h"
#ifndef COMMUNITY
#include "SchemaOptimizer.h"
#include "RedundantIndexFinder.h"
#include "FormView.h"
#include "DatabaseSearch.h"
#endif
#include "MDIWindow.h"
#include "FKDropDown.h"
#include "CalendarCtrl.h"

#define PROMPT_MESSAGE _(L"Can not find selected table in Object Browser.\r\n\r\nRemove Filter applied (if any) in Object Browser and Refresh Object Browser.")


BOOL 
HandleClickEventOnLink(HWND hwndLayout, HELEMENT helem, const wchar_t *url)
{
	wyInt32 tabicon;	
	MDIWindow *wnd = GetActiveWin();
	if(url)
	{
		tabicon = wnd->m_pctabmodule->GetActiveTabImage();

		if(tabicon == IDI_DATASEARCH)
		{
#ifndef COMMUNITY			
			TabDbSearch *tabsearch;
			tabsearch = dynamic_cast<TabDbSearch*>(wnd->m_pctabmodule->GetActiveTabType());
			
            if(!tabsearch)
            {
				return FALSE;
            }

			tabsearch->OnHyperLink(helem, url);
#endif
			return true;		
		}

		if(!wcsicmp(url, L"optimizer_help "))
        {
			ShowHelp("http://sqlyogkb.webyog.com/article/81-schema-optimiser");
        }
        else if(!wcsicmp(url, L"redundantindexes_help "))
        {
            ShowHelp("http://sqlyogkb.webyog.com/article/82-redundant-index-analyzer");
        }
		else if(!wcsicmp(url, L"pref_others ")) 
		{
			PreferenceBase	*pref = CreatePreferences();
			
            if(pref)
			{
				if(pGlobals->m_entlicense.CompareI("Professional"))
				{
					pref->Create(4);
				}
				else
				{
					pref->Create(3);
				}

				delete pref;
			}
		}
		else if(!wcsicmp(url, L"http://www.webyog.com/product/sqlyogpricing/?ref=community.queryprofiler"))
        {
			::ShellExecuteW(hwndLayout,L"open", url, NULL,NULL,SW_SHOWNORMAL);
        }
        else
        {
            HandleClickTableURL(url);
            PostMessage(hwndLayout, UM_FOCUS, 0, 0);
        }
	}

	return true;		
}

BOOL
HandleClickButton(const wchar_t* buttontext, BEHAVIOR_EVENT_PARAMS& params)
{
    MDIWindow*              wnd = GetActiveWin();
	wyInt32		            tabicon = 0;
    htmlayout::dom::element src = params.heTarget; 	
    htmlayout::dom::element temp;
#ifndef COMMUNITY
    TabObject*              ptabinfo = NULL;
    SchemaOptimizer*        sopt = NULL;
    RedundantIndexFinder*   redindfinder = NULL;
	TabDbSearch*            tabsearch;    
    TabEditor*              ptabeditor = wnd->m_pctabmodule->GetActiveTabEditor();
#endif

    tabicon = wnd->m_pctabmodule->GetActiveTabImage();

    if(!wnd || !wnd->m_pctabmodule || !buttontext || !wcslen(buttontext))
    {
		return FALSE;
    }

#ifndef COMMUNITY	
	if(tabicon == IDI_DATASEARCH)
	{
		if(!(tabsearch = (TabDbSearch*)wnd->m_pctabmodule->GetActiveTabType()))
        {
            return FALSE;
        }
		
        tabsearch->OnSearchButton();
        return FALSE;
	}
	else if(tabicon == IDI_TABLEINDEX)
	{
		ptabinfo = (TabObject*)wnd->m_pctabmodule->GetActiveTabType();
		
        if(!ptabinfo || !ptabinfo->m_pobjinfo->m_schemaoptimize || !ptabinfo->m_pobjinfo->m_redindexfinder)
        {
			return FALSE;
        }

        sopt = ptabinfo->m_pobjinfo->m_schemaoptimize;
        redindfinder = ptabinfo->m_pobjinfo->m_redindexfinder;
	}
    else if(ptabeditor && ptabeditor->m_pctabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX)
    {
        sopt = ptabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->m_schemaoptimize;
        redindfinder = ptabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->m_redindexfinder;
    }
    else
    {
        return FALSE;
    }
    
    if(!sopt->m_tabinfo || !redindfinder->m_tabinfo)
	{
		return FALSE;
	}
#endif

    if(wcsicmp(buttontext, OPTIMIZER_CALCULATE) == 0 || 
       wcsicmp(buttontext, REDUNDANT_INDEX_FIND) == 0)
	{
#ifndef COMMUNITY
        if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && 
           pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
        {
            if(wcsicmp(buttontext, OPTIMIZER_CALCULATE) == 0)
            {
	    	    sopt->ProcessTableAnalyse();		
            }
            else
            {
                if(redindfinder->ProcessFind() == wyTrue)
                {
                    //scroll the element into view
                    temp = htmlayout::dom::element::root_element(ptabinfo->m_pobjinfo->m_hwndhtmleditor);
                    temp = temp.get_element_by_id(L"indexinfo");
                    temp.scroll_to_view(true, false);
                }                
            }
        }
#else		
		pGlobals->m_pcmainwin->m_connection->GetSQLyogUltimateDialog();
		return TRUE;
#endif
	}
#ifndef COMMUNITY		
	else if(wcsicmp(buttontext, OPTIMIZER_STOP) == 0)
	{		
		sopt->StopExecution(wnd);

		//Resetting the Button text
		if(sopt->m_ismysqlerror == wyTrue)
		{
			src.set_text(_(L"Calculate Optimal Datatypes"));
		}
	}
	else if(wcsicmp(buttontext, OPTIMIZER_HIDE) == 0)
	{		
        if(ptabinfo && ptabinfo->m_pobjinfo)
        {
		    ptabinfo->m_pobjinfo->m_istableanalyse = wyFalse;
		    ptabinfo->m_pobjinfo->m_istohideoptimizecolumn = wyTrue;
		    ptabinfo->OnSelectInfoTab(sopt->m_con);
		    ptabinfo->m_pobjinfo->m_istohideoptimizecolumn = wyFalse;
        }
        else if(ptabeditor && ptabeditor->m_pctabmgmt->m_pqueryobj)
        {
            ptabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->m_istableanalyse = wyFalse;
		    ptabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->m_istohideoptimizecolumn = wyTrue;
            ptabeditor->m_pctabmgmt->m_pqueryobj->Refresh(wyTrue);
		    ptabeditor->m_pctabmgmt->m_pqueryobj->m_pobjectinfo->m_istohideoptimizecolumn = wyFalse;
        }
	}
    else if(wcsicmp(buttontext, REDUNDANT_INDEX_HIDE)== 0)
    {
        //hide the redundant indexes info
        redindfinder->HideColumns();
        //scroll the element into view
        temp = htmlayout::dom::element::root_element(ptabinfo->m_pobjinfo->m_hwndhtmleditor);
        temp = temp.get_element_by_id(L"indexinfo");
        temp.scroll_to_view(true, false);
    }
#endif
    return FALSE;
}

//this function is called whenever the table link is clicked from the DB level
void HandleClickTableURL(const wchar_t *url)
{
    MDIWindow   *wnd = NULL;
    HTREEITEM   hitem = NULL;
    wyWChar     buff[MAX_PATH] = {0};
    TVITEM      tvitem;

    wnd = GetActiveWin();

    if(!wnd || !wnd->m_pcqueryobject)
    {
		return;
    }

    //switch the current selection image
    switch(wnd->m_pcqueryobject->GetSelectionImage())
    {
        case NDATABASE:
            //get the selection
            hitem = TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd);

            if(hitem)
            {
                //expand and get the first child
                TreeView_Expand(wnd->m_pcqueryobject->m_hwnd, hitem, TVE_EXPAND);
                hitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, hitem);
            }
            break;

        case NTABLES:
            //get the selection
            hitem = TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd);
    }

    if(!hitem)
    {
        return;
    }

    //expand the Tables folder
    TreeView_Expand(wnd->m_pcqueryobject->m_hwnd, hitem, TVE_EXPAND);
    
    //traverse through the tables
    for(hitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, hitem); hitem; 
        hitem = TreeView_GetNextSibling(wnd->m_pcqueryobject->m_hwnd, hitem))
    {
        //get the item
        tvitem.hItem = hitem;
        tvitem.mask = TVIF_TEXT;
        buff[0] = 0;
        tvitem.pszText = buff;
        tvitem.cchTextMax = MAX_PATH - 1;
        TreeView_GetItem(wnd->m_pcqueryobject->m_hwnd, &tvitem);

        //compare the item with the url
        if(!wcscmp(url, tvitem.pszText))
        {
            //set the selection; this will generate  TVN_SELCHANGING and TVN_SELCHANGED messages
            TreeView_SelectItem(wnd->m_pcqueryobject->m_hwnd, hitem);
            break;
        }
    }

    if(!hitem)
    {
        MDIWindow *wnd = GetActiveWin();
        yog_message(wnd->m_hwnd, PROMPT_MESSAGE, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
    }

    return;
}

//the function checks whether the given point is inside the bounding rectangle of the element identified by id
wyBool IsPointInsideHTMLElement(HWND hwnd, wyWChar* id, POINT pt)
{
    htmlayout::dom::element temp;

    temp = htmlayout::dom::element::root_element(hwnd);
    temp = temp.get_element_by_id(id);

    if(temp.is_inside(pt) == true)
    {
        return wyTrue;
    }

    return wyFalse;
}

BOOL HandleForms(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params)
{	
    return TRUE;
}

BOOL
OnEditHtmlData(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params)   
{
    return TRUE;
}

//When a "Key_Down" on Html page
BOOL
OnKeyDown(KEY_PARAMS *params)   
{		
#ifndef COMMUNITY
	MDIWindow*      wnd;
	wyInt32			tabicon;
	TabDbSearch*    tabsearch = NULL;

    wnd = GetActiveWin();
    tabicon = wnd->m_pctabmodule->GetActiveTabImage();

	//Handle Database search tab, when press on Enter key
	if(tabicon == IDI_DATASEARCH)
	{	
        if(!(tabsearch = (TabDbSearch*)wnd->m_pctabmodule->GetActiveTabType()))
        {
            return FALSE;
        }

        if(params->cmd == KEY_DOWN && params->key_code == VK_RETURN)
		{
            if(tabsearch->OnEnterKey(params->target))
            {
                return TRUE;					
            }
		}	
	}
#endif
    return FALSE;
}

//When a "Key_Down + Shift" on Html page
BOOL
OnKeyDownWithShiftKey(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params)   
{
    return FALSE;    
}

BOOL 
OnCharKeyDown(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params)
{
    return FALSE;
}