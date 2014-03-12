//This is code from HTMLLay out downloded ATL sample, Just taken main things from there as they replied my query
//http://terrainformatica.com/forums/topic.php?id=900&page&replies=1

#ifndef _htmlrender_
#define _htmlrender_

#include "Global.h"
#include "htmlayout.h"
#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;
#define OPTIMIZER_CALCULATE     _(L"Calculate Optimal Datatypes")
#define OPTIMIZER_STOP		    _(L"Stop Calculation")
#define OPTIMIZER_HIDE		    _(L"Hide Optimal Datatypes")
#define REDUNDANT_INDEX_FIND    _(L"Find Redundant Indexes")
#define REDUNDANT_INDEX_HIDE    _(L"Hide Redundant Indexes Info")

struct Module 
{
    HMODULE hModule;
    bool freeOnDestruct;
	inline Module() : hModule(pGlobals->m_hinstance), freeOnDestruct(false) {}
    inline ~Module() { if (freeOnDestruct && hModule) ::FreeLibrary(hModule); }
    HMODULE Load(LPCTSTR pszModule, DWORD flags) { freeOnDestruct = true; return hModule = ::LoadLibraryEx(pszModule, 0, flags); }
    operator HMODULE() const { return hModule; }
};

static LRESULT LoadResourceData(HWND hWnd, LPCWSTR uri )
  {
    //USES_CONVERSION;

    //ATLASSERT(::IsWindow(hWnd));
    // Check for trivial case

    if (!uri || !uri[0]) return LOAD_DISCARD;

    if (wcsncmp( uri, L"file:", 5 ) == 0 )
      return LOAD_OK;
    if (wcsncmp( uri, L"http:", 5 ) == 0 )
      return LOAD_OK;
    if (wcsncmp( uri, L"https:", 6 ) == 0 )
      return LOAD_OK;

    if (wcsncmp( uri, L"res:", 4 ) == 0 )
    {
      uri += 4;
    }
    //ATTN: you may wish to uncomment this 'else' and it will go further *only if* "res:...." url requested  
    //else 
    //  return LOAD_OK;


    // Retrieve url specification into a local storage since FindResource() expects 
    // to find its parameters on stack rather then on the heap under Win9x/Me

	wyString filepath;

	filepath.SetAs(uri);

    //TCHAR achURL[MAX_PATH]; 
	//lstrcpyn(achURL, W2CT(uri), MAX_PATH);
	//lstrcpyn(achURL, "BACK.PNG", MAX_PATH);

    Module module;

    // Separate name and handle external resource module specification

//    LPTSTR psz;//, pszName = achURL;
    /*if ((psz = _tcsrchr(pszName, '/')) != NULL) {
      LPTSTR pszModule = pszName; pszName = psz + 1; *psz = '\0';
      DWORD dwAttr = ::GetFileAttributes(pszModule);
      if (dwAttr != INVALID_FILE_ATTRIBUTES && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        module.Load(pszModule, LOAD_LIBRARY_AS_DATAFILE);
      }
    }*/

    // Separate extension if any
	wyWChar *pszExt = NULL, *tok = NULL;

    //LPTSTR pszExt = _tcsrchr(pszName, '.'); 

    pszExt = wcsrchr(filepath.GetAsWideChar(), '.'); 

	if (pszExt) *pszExt++ = '\0';

	tok = wcstok(filepath.GetAsWideChar(NULL, wyTrue), L".");

	if(!tok)
		 return LOAD_OK;
	
    // Find specified resource and leave if failed. Note that we use extension
    // as the custom resource type specification or assume standard HTML resource 
    // if no extension is specified

    HRSRC hrsrc = 0;
    bool  isHtml = false;
#ifndef _WIN32_WCE
    if( pszExt == 0 || wcsicmp(pszExt, L"HTML") == 0)
    {
		hrsrc = ::FindResource(module, tok, RT_HTML);
      isHtml = true;
    }
    else
        hrsrc = ::FindResource(module, tok, pszExt);
#else 
      hrsrc = ::FindResource(module, pszName, pszExt);
#endif

    if (!hrsrc) return LOAD_OK; // resource not found here - proceed with default loader

    // Load specified resource and check if ok

    HGLOBAL hgres = ::LoadResource(module, hrsrc);
    if (!hgres) return LOAD_DISCARD;

    // Retrieve resource data and check if ok

    PBYTE pb = (PBYTE)::LockResource(hgres); if (!pb) return LOAD_DISCARD;
    DWORD cb = ::SizeofResource(module, hrsrc); if (!cb) return LOAD_DISCARD;

    // Report data ready

    ::HTMLayoutDataReady(hWnd, uri, pb,  cb);
    
    return LOAD_OK;
  }

static
LRESULT OnAttachBehavior(LPNMHL_ATTACH_BEHAVIOR lpab )
{
    // attach custom behaviors
    htmlayout::behavior::handle(lpab);
   
    // behavior implementations are located om /include/behaviors/ folder
    // to connect them into the chain of available 
    // behaviors - just include them into the project.
    return 0;
}

static
LRESULT OnLoadData(LPNMHL_LOAD_DATA pnmld)
  {
//    ATLTRACE(_T("CHTMLayoutHost::OnLoadData: uri='%s'\n"), CString(pnmld->uri));

    // Try to load data from resource and if failed, proceed with default processing.
    // Note that this code assumes that the host and control windows are the same. If
    // you are handling HTMLayout control notification in another window, you'll have
    // to override this method and provide proper hWnd.

 //   T* pT = static_cast<T*>(this);
    
   // ATLASSERT(::IsWindow(pT->m_hWnd));

	  return LoadResourceData(pnmld->hdr.hwndFrom, pnmld->uri);
  }

//static
//LRESULT OnCreateControl(LPNMHL_CREATE_CONTROL pnmld)
//  {
////    ATLTRACE(_T("CHTMLayoutHost::OnLoadData: uri='%s'\n"), CString(pnmld->uri));
//
//    // Try to load data from resource and if failed, proceed with default processing.
//    // Note that this code assumes that the host and control windows are the same. If
//    // you are handling HTMLayout control notification in another window, you'll have
//    // to override this method and provide proper hWnd.
//
// //   T* pT = static_cast<T*>(this);
//    
//   // ATLASSERT(::IsWindow(pT->m_hWnd));
//	  //pnmld->helement->G
//	  return 0;
//  }

static
LRESULT CALLBACK HTMLayoutNotifyHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID vParam)
{
  // all HTMLayout notification are comming here.

  int k;

  k = ((NMHDR*)lParam)->code;

  switch(((NMHDR*)lParam)->code)
  {
      case HLN_CREATE_CONTROL:    break; //return OnCreateControl((LPNMHL_CREATE_CONTROL) lParam);
      case HLN_CONTROL_CREATED:   break; //return OnControlCreated((LPNMHL_CREATE_CONTROL) lParam);
      case HLN_DESTROY_CONTROL:   break; //return OnDestroyControl((LPNMHL_DESTROY_CONTROL) lParam);
      case HLN_LOAD_DATA:         //break; //return OnLoadData((LPNMHL_LOAD_DATA) lParam);
		  return OnLoadData((LPNMHL_LOAD_DATA) lParam);
      case HLN_DATA_LOADED:       break; //return OnDataLoaded((LPNMHL_DATA_LOADED)lParam);
      case HLN_DOCUMENT_COMPLETE: break; //return OnDocumentComplete();
      case HLN_ATTACH_BEHAVIOR:  
		  return OnAttachBehavior((LPNMHL_ATTACH_BEHAVIOR)lParam );
  }
  return 0;
}

//------------------------------------------------------------------------
BOOL HandleClickEventOnLink(HWND hwndLayout, HELEMENT helem, const wchar_t *url);

BOOL HandleClickButton(const wchar_t* buttontext, BEHAVIOR_EVENT_PARAMS& params);

//BOOL HandleForms(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params);

BOOL OnEditHtmlData(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params);

BOOL OnKeyDown(KEY_PARAMS *params); 

BOOL OnKeyDownWithShiftKey(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params); 

BOOL OnCharKeyDown(const char* buttontext, BEHAVIOR_EVENT_PARAMS *params); 

///This function is called whenever the table link is clicked from the DB level
/**
@param url      : IN pointer to the url string
@returns void
*/
void            HandleClickTableURL(const wchar_t *url);

///The function checks whether the given point is inside the bounding rectangle of the element identified by id
/**
@param id       : IN id of the element
@param pt       : IN point
@returns wyTrue if it is in, ele wyFalse
*/
wyBool IsPointInsideHTMLElement(HWND hwnd, wyWChar* id, POINT pt);

//void			HandleIndexFinderAndOptimizer();

static int eventcnt = 0;

//HTMLlayout Hyper link handling here
static struct DOMEventsHandlerType: htmlayout::event_handler
{    	
	DOMEventsHandlerType(): event_handler(0xFFFFFFFF) {}

    BOOL handle_key(HELEMENT he, KEY_PARAMS& params)
    {
        OnKeyDown(&params);
        return htmlayout::event_handler::handle_key(he, params);
    }

    virtual BOOL handle_event (HELEMENT he, BEHAVIOR_EVENT_PARAMS& params ) 
    {  		
		htmlayout::dom::element src = params.heTarget; 	

		wyString controlname;
		
		const wchar_t *strname = NULL;//src.get_attribute("name");
		
		//if(strname)
          //  controlname.SetAs(strname);
		
						
		HWND				hwndLayout = src.get_element_hwnd(true);
		
		switch(params.cmd)
        {
            case BUTTON_CLICK:
		    {
				strname = src.get_attribute("value");

				return HandleClickButton(strname, params);			    
		    }
            break;

            case HYPERLINK_CLICK:           // hyperlink click
		{
			const wchar_t* url = src.get_attribute("href");	

			if(hwndLayout)
				return HandleClickEventOnLink(hwndLayout, params.heTarget, url);
		}
            break;

		
		//else if(!stricmp(type, "input"))
		//{			
		//	if(name)
		//	{
		//		if(eventcnt == 1) //For avoiding the Dialog to pops up 2nd time
		//		{
		//			eventcnt = 0;
		//			return TRUE;
		//		}
		//		eventcnt++;

		//		return HandleClickButton((const char*)name, params);
		//	}
		//}

			case FORM_SUBMIT | SINKING:
			{	
				strname = src.get_attribute("name");
				if(strname)
				{
					controlname.SetAs(strname);
				}

			    //return HandleForms(controlname.GetString(), &params);			
		    }
            break;

			case EDIT_VALUE_CHANGING:
			case SELECT_SELECTION_CHANGED:
			case EDIT_VALUE_CHANGED:
			{
				strname = src.get_attribute("id");
				if(strname)
				{
					controlname.SetAs(strname);
				}

                OnEditHtmlData(controlname.GetString(), &params);	
			}
            break;

			/*case CONTEXT_MENU_REQUEST:
			case CONTEXT_MENU_SETUP:
				{
					strname = src.get_attribute("id");
					int k = 20; 
				}
				break;*/
		}
		    
		return FALSE; 
   }
 
} DOMEventsHandler;
//-----------------------------------------------------------------------------

#endif