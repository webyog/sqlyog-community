//
// Windows Template Library Extension for
// Terra Informatica Lightweight Embeddable HTMLayout control
// http://terra-informatica.org/htmlayout
//
// Written by Andrew Fedoniouk and Pete Kvitek

//
// This file is NOT part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// (C) 2003, Pete Kvitek <pete@kvitek.com>
//       and Andrew Fedoniouk <andrew@TerraInformatica.com>
//

#ifndef __WTL_HTMLAYOUTHOST_H__
#define __WTL_HTMLAYOUTHOST_H__

#pragma once

#ifndef __cplusplus
  #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
  #error wtl_htmlayouthost.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
  #error wtl_htmlayouthost.h requires atlwin.h to be included first
#endif

#include <atlmisc.h>
#include <atlctrls.h>

#ifdef _WIN32_WCE
#include <commctrl.h>
#define stricmp _stricmp
#else
#include <richedit.h>
#endif

#include "htmlayout.h"
#include "htmlayout_behavior.h"
#include "htmlayout_dom.hpp"
#include "htmlayout_behavior.hpp"
#include "behaviors/notifications.h"
//#include "xool/xool.h"

/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CHTMLayoutHost<T>
//

namespace WTL
{

// Command name handler. Used primarily for handling button clicks 
// Mimics COMMAND_ID_HANDLER but uses html defined control name instead of ID 

#define HTML_WIDGET_HANDLER(element_name, func) \
  if(uMsg == WM_COMMAND) \
  { \
    LPCTSTR ctlName = GetDlgItemName((HWND)lParam); \
    if(ctlName && _tcsicmp(ctlName,element_name) == 0) \
    { \
       bHandled = TRUE; \
       lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
       if(bHandled) \
         return TRUE; \
    } \
  }

// handles clicks on elements having behavior 'command'

#define HTML_COMMAND_CLICK_HANDLER(lpwsElementId, func) \
  if(uMsg == WM_BEHAVIOR_NOTIFY && HLN_COMMAND_CLICK == ((LPNMHDR)lParam)->code  ) \
  { \
    LPCWSTR srcElementId = ((NMHL_COMMAND_CLICK*)lParam)->szElementID; \
    if(wcscmp(srcElementId,lpwsElementId) == 0) \
    { \
       bHandled = TRUE; \
       HELEMENT he = ((NMHL_COMMAND_CLICK*)lParam)->he; \
       lResult = func(lpwsElementId, he, bHandled); \
       if(bHandled) \
         return TRUE; \
    } \
  }

// George
#define HTML_COMMAND_CLICK_HANDLER_EX(func) \
  if (uMsg == WM_BEHAVIOR_NOTIFY && HLN_COMMAND_CLICK == ((LPNMHDR)lParam)->code) \
  { \
    bHandled = TRUE;  \
    lResult = func((NMHL_COMMAND_CLICK*)lParam, bHandled);  \
    if (bHandled) \
      return TRUE;  \
  }

  inline bool IsWindowsXP()
  {
    static int tristate = 0; 
    if( tristate == 0 )
    {
      OSVERSIONINFO ovi = { sizeof(OSVERSIONINFO) };
      ::GetVersionEx(&ovi);
      tristate = ((ovi.dwMajorVersion == 5 && ovi.dwMinorVersion >= 1) || (ovi.dwMajorVersion > 5))? 1:-1;
    }
    return tristate == 1;
  }



// HYPERLINK_HANDLER 
// Handles clicks on hyperlinks in html document
// To be able to catch this notifications class should provide  
// following implementation of 
/*
  LRESULT OnHyperlink(NMHL_HYPERLINK* pnmhl, BOOL& bHandled);
*/

#define HTML_HYPERLINK_HANDLER(func) \
  if(uMsg == WM_BEHAVIOR_NOTIFY && HLN_HYPERLINK == ((LPNMHDR)lParam)->code  ) \
  { \
       bHandled = TRUE; \
       lResult = func((NMHL_HYPERLINK*)lParam, bHandled); \
       if(bHandled) \
         return TRUE; \
  }

inline int GetAttrInt(HELEMENT he, LPCSTR attrName, int defaultValue = 0)
{
  htmlayout::dom::element el = he;
  const wchar_t* pv = el.get_attribute(attrName);
  if(pv && wcslen(pv) > 0)
    return _wtoi(pv);
  return defaultValue;
}

inline CString GetElementType(HELEMENT he)
{
  htmlayout::dom::element el = he;
  CString s = el.get_element_type();
  return s;
}

inline CString GetAttr(HELEMENT he, LPCSTR attrName)
{
  htmlayout::dom::element el = he;
  CString s = el.get_attribute(attrName);
  return s;
}

inline bool HasAttr(HELEMENT he, LPCSTR attrName)
{
  htmlayout::dom::element el = he;
  for(unsigned int i = 0; i < el.get_attribute_count(); ++i)
  {
    if(stricmp(el.get_attribute_name(i),attrName) == 0)
      return true;
  }
  return false;
}



/////////////////////////////////////////////////////////////////////////////
// CHTMLayoutHost - host side implementation for a HTMLayout control

template <class T>
class CHTMLayoutHost
{
public:

  // HTMLayout callback
  static LRESULT CALLBACK callback(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID vParam)
  {
      ATLASSERT(vParam);
      CHTMLayoutHost<T>* pThis = (CHTMLayoutHost<T>*)vParam;
      return pThis->OnHtmlNotify(uMsg, wParam, lParam);
  }

  void SetCallback()
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    ::HTMLayoutSetCallback(pT->m_hWnd,callback, (CHTMLayoutHost<T>*)this);
  }

  void SetEventHandler(htmlayout::event_handler* peh)
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    HTMLayoutWindowAttachEventHandler(pT->m_hWnd, htmlayout::event_handler::element_proc,peh,peh->subscribed_to); 
  }

  // Overridables

  virtual LRESULT OnHtmlNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    ATLASSERT(uMsg == WM_NOTIFY);

    // Crack and call appropriate method
    
    // here are all notifiactions
    switch(((NMHDR*)lParam)->code) 
    {
      case HLN_CREATE_CONTROL:    return OnCreateControl((LPNMHL_CREATE_CONTROL) lParam);
      case HLN_CONTROL_CREATED:   return OnControlCreated((LPNMHL_CREATE_CONTROL) lParam);
      case HLN_DESTROY_CONTROL:   return OnDestroyControl((LPNMHL_DESTROY_CONTROL) lParam);
      case HLN_LOAD_DATA:         return OnLoadData((LPNMHL_LOAD_DATA) lParam);
      case HLN_DATA_LOADED:       return OnDataLoaded((LPNMHL_DATA_LOADED)lParam);
      case HLN_DOCUMENT_COMPLETE: return OnDocumentComplete();
      case HLN_ATTACH_BEHAVIOR:   return OnAttachBehavior((LPNMHL_ATTACH_BEHAVIOR)lParam );
      case HLN_BEHAVIOR_CHANGED:
      case HLN_DIALOG_CREATED:
      case HLN_DIALOG_CLOSE_RQ:
      case HLN_DOCUMENT_LOADED:   return 0; // not used in this wrapper.

      // generic common control notifications:
      /* not used in the wrapper, so they will go to OnHtmlGenericNotifications
      case NM_CLICK:
      case NM_DBLCLK:
      case NM_RETURN:
      case NM_RCLICK:
      case NM_RDBLCLK:
      case NM_SETFOCUS:
      case NM_KILLFOCUS:
      case NM_NCHITTEST:
      case NM_KEYDOWN:
      case NM_RELEASEDCAPTURE:
      case NM_SETCURSOR:
      case NM_CHAR:
        etc.
      //default:
        //ATLASSERT(FALSE); */
    }
    return OnHtmlGenericNotifications(uMsg,wParam,lParam);
  }


  virtual LRESULT OnHtmlGenericNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    // all generic notifications 
    // are coming to the parent of HTMLayout
    T* pT = static_cast<T*>(this);

    ATLASSERT(::IsWindow(pT->m_hWnd));
    // Pass it to the parent window if any
    HWND hWndParent = pT->GetParent();
    if (!hWndParent) return 0;

    return ::SendMessage(hWndParent, uMsg, wParam, lParam);
  }

 
  virtual LRESULT OnCreateControl(LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnCreateControl: type='%s' \n"), GetElementType(pnmcc->helement) );

    // Try to create control and if failed, proceed with default processing.
    // Note that this code assumes that the host and control windows are the same. If
    // you are handling HTMLayout control notification in another window, you'll have
    // to override this method and provide proper hWnd.

    T* pT = static_cast<T*>(this);
    
    ATLASSERT(::IsWindow(pT->m_hWnd));

    return CreateControl(pT->m_hWnd, pnmcc);
  }

  virtual LRESULT OnControlCreated(LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnControlCreated: type='%s' \n"), GetElementType(pnmcc->helement) );
    return 0;
  }

  virtual LRESULT OnDestroyControl(LPNMHL_DESTROY_CONTROL pnmhl)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDestroyControl: HWND=%x\n"), pnmhl->inoutControlHwnd);
    
    //  use this if you don't want this child to be destroyed:
    //  pnmhl->inoutControlHwnd = 0;

    //  If you will not change pnmhl->inoutControlHwnd field then HTMLayout 
    //  will call ::DestroyWindow by itself.

    return 0;
  }


  virtual LRESULT OnLoadData(LPNMHL_LOAD_DATA pnmld)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnLoadData: uri='%s'\n"), CString(pnmld->uri));

    // Try to load data from resource and if failed, proceed with default processing.
    // Note that this code assumes that the host and control windows are the same. If
    // you are handling HTMLayout control notification in another window, you'll have
    // to override this method and provide proper hWnd.

    T* pT = static_cast<T*>(this);
    
    ATLASSERT(::IsWindow(pT->m_hWnd));

    return LoadResourceData(pT->m_hWnd, pnmld->uri);
  }

  virtual LRESULT OnDataLoaded(LPNMHL_DATA_LOADED pnmld)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDataLoaded: uri='%s'\n"), CString(pnmld->uri));
    return 0;
  }

  virtual LRESULT OnDocumentComplete()
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDocumentComplete\n"));
    return 0;
  }

  virtual LRESULT OnAttachBehavior( LPNMHL_ATTACH_BEHAVIOR lpab )
  {
    htmlayout::behavior::handle(lpab);
    return 0;
  }

  // Define module manager that will free loaded module upon exit
struct Module 
  {
    HMODULE hModule;
    bool freeOnDestruct;
    inline Module() : hModule(_Module.GetResourceInstance()), freeOnDestruct(false) {}
    inline ~Module() { if (freeOnDestruct && hModule) ::FreeLibrary(hModule); }
    HMODULE Load(LPCTSTR pszModule, DWORD flags) { freeOnDestruct = true; return hModule = ::LoadLibraryEx(pszModule, 0, flags); }
    operator HMODULE() const { return hModule; }
  };

  // Load Data helper routines

#ifndef INVALID_FILE_ATTRIBUTES //wince has not it
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif


  static LRESULT LoadResourceData(HWND hWnd, LPCSTR uri )
  {
     USES_CONVERSION;
     return LoadResourceData(hWnd,A2CW(uri));
  }

  static LRESULT LoadResourceData(HWND hWnd, LPCWSTR uri )
  {
    USES_CONVERSION;

    ATLASSERT(::IsWindow(hWnd));
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

    TCHAR achURL[MAX_PATH]; lstrcpyn(achURL, W2CT(uri), MAX_PATH);

    Module module;

    // Separate name and handle external resource module specification

    LPTSTR psz, pszName = achURL;
    if ((psz = _tcsrchr(pszName, '/')) != NULL) {
      LPTSTR pszModule = pszName; pszName = psz + 1; *psz = '\0';
      DWORD dwAttr = ::GetFileAttributes(pszModule);
      if (dwAttr != INVALID_FILE_ATTRIBUTES && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        module.Load(pszModule, LOAD_LIBRARY_AS_DATAFILE);
      }
    }

    // Separate extension if any

    LPTSTR pszExt = _tcsrchr(pszName, '.'); if (pszExt) *pszExt++ = '\0';

    // Find specified resource and leave if failed. Note that we use extension
    // as the custom resource type specification or assume standard HTML resource 
    // if no extension is specified

    HRSRC hrsrc = 0;
    bool  isHtml = false;
#ifndef _WIN32_WCE
    if( pszExt == 0 || _tcsicmp(pszExt,TEXT("HTML")) == 0)
    {
      hrsrc = ::FindResource(module, pszName, RT_HTML);
      isHtml = true;
    }
    else
      hrsrc = ::FindResource(module, pszName, pszExt);
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

  LRESULT LoadResourceData(LPNMHL_LOAD_DATA pnmld)
  {
    // This code assumes that the host and control windows are the same

    T* pT = static_cast<T*>(this);
    
    ATLASSERT(::IsWindow(pT->m_hWnd));

    return LoadResourceData(pT->m_hWnd, pnmld->uri);
  }

  virtual LRESULT CreateControl(HWND hWnd, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hWnd));
    ATLASSERT(pnmcc != NULL);

    CString type = GetAttr(pnmcc->helement, "type");

    // Create control of the specified type
#ifndef _WIN32_WCE    
    //if (type == _T("richedit")) return CreateRichEdit(pnmcc->inHwndParent, pnmcc);
#endif
    if (type == _T("sys-datetime")) return CreateDateTime(pnmcc->inHwndParent, pnmcc);
    if (type == _T("sys-calendar")) return CreateCalendar(pnmcc->inHwndParent, pnmcc);
    //if (type == _T("listview")) return CreateListView(pnmcc->inHwndParent, pnmcc);
    //if (type == _T("treeview")) return CreateTreeView(pnmcc->inHwndParent, pnmcc);
    CString elementType = GetElementType(pnmcc->helement);
    
    //if (elementType == _T("iframe")) 
    //  return CreateHTMLayoutFrame(pnmcc->inHwndParent, pnmcc);

    return 0;
    
  }

  // Style table declarations

  #define STYLETABENTRY(prefix, style) { prefix##style, #style }
  #define STYLETABLEEND { 0, NULL }

  typedef struct _STYLETABLE {
    DWORD dwStyle;
    LPCSTR  pszStyle;
  } STYLETABLE;

  static DWORD GetCtlStyle(LPNMHL_CREATE_CONTROL pnmcc, const STYLETABLE* pStyles, DWORD dwDefStyle = 0)
  {
    ATLASSERT(pnmcc != NULL);

    // Scan specified style table collecting styles
    DWORD dwStyle = 0;
    for (; pStyles->pszStyle != NULL; pStyles++) {
      if (HasAttr(pnmcc->helement,pStyles->pszStyle)) {
        dwStyle|= pStyles->dwStyle;
      }
    }
    
    // Return collected styles or default style if none specified
    
    return dwStyle | dwDefStyle;
  }

  // Generic window style handling

  static DWORD GetCtlStyle(LPNMHL_CREATE_CONTROL pnmcc, DWORD dwDefStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
  {
    // Window style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(WS_, OVERLAPPED),
      STYLETABENTRY(WS_, POPUP),
      STYLETABENTRY(WS_, CHILD),
      STYLETABENTRY(WS_, VISIBLE),
      STYLETABENTRY(WS_, DISABLED),
      STYLETABENTRY(WS_, CLIPSIBLINGS),
      STYLETABENTRY(WS_, CLIPCHILDREN),
    #ifndef _WIN32_WCE
      STYLETABENTRY(WS_, MINIMIZE),
      STYLETABENTRY(WS_, MAXIMIZE),
    #endif
      STYLETABENTRY(WS_, CAPTION),
      STYLETABENTRY(WS_, BORDER),
      STYLETABENTRY(WS_, DLGFRAME),
      STYLETABENTRY(WS_, VSCROLL),
      STYLETABENTRY(WS_, HSCROLL),
      STYLETABENTRY(WS_, SYSMENU),
      STYLETABENTRY(WS_, THICKFRAME),
      STYLETABENTRY(WS_, GROUP),
      STYLETABENTRY(WS_, TABSTOP),
      STYLETABENTRY(WS_, MINIMIZEBOX),
      STYLETABENTRY(WS_, MAXIMIZEBOX),
      STYLETABLEEND
    };

    return GetCtlStyle(pnmcc, aStyles, dwDefStyle);
  }

  static DWORD GetCtlExStyle(LPNMHL_CREATE_CONTROL pnmcc, DWORD dwDefExStyle = 0)
  {
    // Extended window style table
#ifndef _WIN32_WCE
    static STYLETABLE aStyles[] = {
      STYLETABENTRY(WS_EX_, DLGMODALFRAME),
      STYLETABENTRY(WS_EX_, NOPARENTNOTIFY),
      STYLETABENTRY(WS_EX_, TOPMOST),
      STYLETABENTRY(WS_EX_, ACCEPTFILES),
      STYLETABENTRY(WS_EX_, TRANSPARENT),
    #if (WINVER >= 0x0400)
      STYLETABENTRY(WS_EX_, MDICHILD),
      STYLETABENTRY(WS_EX_, TOOLWINDOW),
      STYLETABENTRY(WS_EX_, WINDOWEDGE),
      STYLETABENTRY(WS_EX_, CLIENTEDGE),
      STYLETABENTRY(WS_EX_, CONTEXTHELP),
    #endif
    #if (WINVER >= 0x0400)
      STYLETABENTRY(WS_EX_, RIGHT),
      STYLETABENTRY(WS_EX_, LEFT),
      STYLETABENTRY(WS_EX_, RTLREADING),
      STYLETABENTRY(WS_EX_, LTRREADING),
      STYLETABENTRY(WS_EX_, LEFTSCROLLBAR),
      STYLETABENTRY(WS_EX_, RIGHTSCROLLBAR),
      STYLETABENTRY(WS_EX_, CONTROLPARENT),
      STYLETABENTRY(WS_EX_, STATICEDGE),
      STYLETABENTRY(WS_EX_, APPWINDOW),
    #endif
    #if (_WIN32_WINNT >= 0x0500)
      STYLETABENTRY(WS_EX_, LAYERED),
    #endif
    #if (WINVER >= 0x0500)
      STYLETABENTRY(WS_EX_, NOINHERITLAYOUT),
      STYLETABENTRY(WS_EX_, LAYOUTRTL),
    #endif
    #if (_WIN32_WINNT >= 0x0500)
      STYLETABENTRY(WS_EX_, NOACTIVATE),
    #endif
    #if (_WIN32_WINNT >= 0x0501)
      STYLETABENTRY(WS_EX_, COMPOSITED),
    #endif
      STYLETABLEEND
    };
    return GetCtlStyle(pnmcc, aStyles, dwDefExStyle);
#else
    return dwDefExStyle;
#endif
  }

  // Control factory routines
#ifndef _WIN32_WCE
  virtual LRESULT CreateRichEdit(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);
    
    // Control style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(ES_, LEFT),
      STYLETABENTRY(ES_, CENTER),
      STYLETABENTRY(ES_, RIGHT),
      STYLETABENTRY(ES_, MULTILINE),
      STYLETABENTRY(ES_, PASSWORD),
      STYLETABENTRY(ES_, AUTOVSCROLL),
      STYLETABENTRY(ES_, AUTOHSCROLL),
      STYLETABENTRY(ES_, NOHIDESEL),
      STYLETABENTRY(ES_, READONLY),
      STYLETABENTRY(ES_, WANTRETURN),
    #if(WINVER >= 0x0400)
      STYLETABENTRY(ES_, NUMBER),
    #endif

      STYLETABENTRY(ES_, SAVESEL),
      STYLETABENTRY(ES_, SUNKEN),
      STYLETABENTRY(ES_, DISABLENOSCROLL),
      STYLETABENTRY(ES_, SELECTIONBAR),
      STYLETABENTRY(ES_, NOOLEDRAGDROP),
      STYLETABENTRY(ES_, VERTICAL),
      STYLETABENTRY(ES_, NOIME),
      STYLETABENTRY(ES_, SELFIME),
      STYLETABLEEND
    };

    // Default richedit style

    const DWORD dwDefStyle = ES_MULTILINE 
                 | ES_WANTRETURN 
//                 | ES_AUTOVSCROLL 
//                 | ES_AUTOHSCROLL
                 ;

    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc) | GetCtlStyle(pnmcc, aStyles, dwDefStyle);
    DWORD dwExStyle = GetCtlExStyle(pnmcc);

    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CRichEditCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), pnmcc);

    if (!pnmcc->outControlHwnd) 
    {
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;
      return 0;
    }

    // Handle bottomless RichEdit controls

    if (HasAttr(pnmcc->helement,"autoheight")) {
      ::SendMessage(pnmcc->outControlHwnd, EM_SETEVENTMASK, 0, ENM_REQUESTRESIZE);
    }
    
    return 0;
  }
#endif

  virtual LRESULT CreateHTMLayoutFrame(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);
  
    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc);
    DWORD dwExStyle = GetCtlExStyle(pnmcc);

    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CHTMLayoutCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), pnmcc);

    if (!pnmcc->outControlHwnd) 
    {
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;
      return 0;
    }

    CHTMLayoutCtrl ctl = pnmcc->outControlHwnd;

    ::HTMLayoutSetCallback(ctl.m_hWnd, callback, (CHTMLayoutHost<T>*)this);

    // set inital text with 1 pix margins 
    const char inittext[] = 
      "<html><body leftmargin=1 topmargin=1 rightmargin=1 bottommargin=1>&nbsp;</body></html>";
    ctl.LoadHtml((LPCBYTE)inittext,sizeof(inittext));

    htmlayout::dom::element el = pnmcc->helement;
    wchar_t src[2048];
    const wchar_t* t = el.get_attribute("src");
    if( t )
    {
      USES_CONVERSION;
      wcsncpy(src,t,2047); src[2047] = 0;
      el.combine_url(src,2048);
      ctl.OpenFile(W2T(src));
    }
    return 0;
  }


  virtual LRESULT CreateDateTime(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);

    // Control style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(DTS_, UPDOWN),
      STYLETABENTRY(DTS_, SHOWNONE),
      STYLETABENTRY(DTS_, SHORTDATEFORMAT),
      STYLETABENTRY(DTS_, LONGDATEFORMAT),
    #if (_WIN32_IE >= 0x500)
      STYLETABENTRY(DTS_, SHORTDATECENTURYFORMAT),
    #endif
      STYLETABENTRY(DTS_, TIMEFORMAT),
      STYLETABENTRY(DTS_, APPCANPARSE),
      STYLETABENTRY(DTS_, RIGHTALIGN),
      STYLETABLEEND
    };

    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc) | GetCtlStyle(pnmcc, aStyles);
    DWORD dwExStyle = GetCtlExStyle(pnmcc);

    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CDateTimePickerCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), 
      pnmcc);

    if (!pnmcc->outControlHwnd) 
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;
    
    return 0;
  }

  virtual LRESULT CreateCalendar(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);
    
    // Control style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(MCS_, DAYSTATE),
      STYLETABENTRY(MCS_, MULTISELECT),
      STYLETABENTRY(MCS_, WEEKNUMBERS),
    #if (_WIN32_IE >= 0x0400)
      STYLETABENTRY(MCS_, NOTODAYCIRCLE),
      STYLETABENTRY(MCS_, NOTODAY),
    #else
      STYLETABENTRY(MCS_, NOTODAY),
    #endif
      STYLETABLEEND
    };

    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc) | GetCtlStyle(pnmcc, aStyles);
    DWORD dwExStyle = GetCtlExStyle(pnmcc);

    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CMonthCalendarCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), 
      pnmcc);

    if (!pnmcc->outControlHwnd) 
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;
    
    return 0;
  }

  virtual LRESULT CreateListView(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);
    
    // Control style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(LVS_, ICON),
      STYLETABENTRY(LVS_, REPORT),
      STYLETABENTRY(LVS_, SMALLICON),
      STYLETABENTRY(LVS_, LIST),
      STYLETABENTRY(LVS_, SINGLESEL),
      STYLETABENTRY(LVS_, SHOWSELALWAYS),
      STYLETABENTRY(LVS_, SORTASCENDING),
      STYLETABENTRY(LVS_, SORTDESCENDING),
      STYLETABENTRY(LVS_, SHAREIMAGELISTS),
      STYLETABENTRY(LVS_, NOLABELWRAP),
      STYLETABENTRY(LVS_, AUTOARRANGE),
      STYLETABENTRY(LVS_, EDITLABELS),
    #if (_WIN32_IE >= 0x0300)
      STYLETABENTRY(LVS_, OWNERDATA),
    #endif
      STYLETABENTRY(LVS_, NOSCROLL),
      STYLETABENTRY(LVS_, ALIGNTOP),
      STYLETABENTRY(LVS_, ALIGNLEFT),
      STYLETABENTRY(LVS_, OWNERDRAWFIXED),
      STYLETABENTRY(LVS_, NOCOLUMNHEADER),
      STYLETABENTRY(LVS_, NOSORTHEADER),
      STYLETABLEEND
    };

    // Extended control style table

    static STYLETABLE aExStyles[] = {
      STYLETABENTRY(LVS_EX_, GRIDLINES),
      STYLETABENTRY(LVS_EX_, SUBITEMIMAGES),
      STYLETABENTRY(LVS_EX_, CHECKBOXES),
      STYLETABENTRY(LVS_EX_, TRACKSELECT),
      STYLETABENTRY(LVS_EX_, HEADERDRAGDROP),
      STYLETABENTRY(LVS_EX_, FULLROWSELECT),
      STYLETABENTRY(LVS_EX_, ONECLICKACTIVATE),
   #ifndef _WIN32_WCE
      STYLETABENTRY(LVS_EX_, TWOCLICKACTIVATE),
    #if (_WIN32_IE >= 0x0400)
      STYLETABENTRY(LVS_EX_, FLATSB),
      STYLETABENTRY(LVS_EX_, REGIONAL),
      STYLETABENTRY(LVS_EX_, INFOTIP),
      STYLETABENTRY(LVS_EX_, UNDERLINEHOT),
      STYLETABENTRY(LVS_EX_, UNDERLINECOLD),
      STYLETABENTRY(LVS_EX_, MULTIWORKAREAS),
    #endif
    #if (_WIN32_IE >= 0x0500)
      STYLETABENTRY(LVS_EX_, LABELTIP),
      STYLETABENTRY(LVS_EX_, BORDERSELECT),
    #endif
    #if (_WIN32_WINNT >= 0x501)
      STYLETABENTRY(LVS_EX_, DOUBLEBUFFER),
      STYLETABENTRY(LVS_EX_, HIDELABELS),
      STYLETABENTRY(LVS_EX_, SINGLEROW),
      STYLETABENTRY(LVS_EX_, SNAPTOGRID),
      STYLETABENTRY(LVS_EX_, SIMPLESELECT),
    #endif
   #endif
      STYLETABLEEND
    };
    
    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc) | GetCtlStyle(pnmcc, aStyles);
    DWORD dwExStyle = GetCtlExStyle(pnmcc) | GetCtlStyle(pnmcc, aExStyles);

#ifdef FLICKER_FREE_COMCTL // force double buffering for it
    if( IsWindowsXP() ) 
        dwExStyle |= WS_EX_COMPOSITED;
#endif

    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CListViewCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), 
      pnmcc);

    if (!pnmcc->outControlHwnd) 
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;

    return 0;
  }

  virtual LRESULT CreateTreeView(HWND hwndParent, LPNMHL_CREATE_CONTROL pnmcc)
  {
    ATLASSERT(::IsWindow(hwndParent));
    ATLASSERT(pnmcc != NULL);
    
    // Control style table

    static STYLETABLE aStyles[] = {
      STYLETABENTRY(TVS_, HASBUTTONS),
      STYLETABENTRY(TVS_, HASLINES),
      STYLETABENTRY(TVS_, LINESATROOT),
      STYLETABENTRY(TVS_, EDITLABELS),
      STYLETABENTRY(TVS_, DISABLEDRAGDROP),
      STYLETABENTRY(TVS_, SHOWSELALWAYS),
    #if (_WIN32_IE >= 0x0300)
      STYLETABENTRY(TVS_, RTLREADING),
      STYLETABENTRY(TVS_, NOTOOLTIPS),
      STYLETABENTRY(TVS_, CHECKBOXES),
      STYLETABENTRY(TVS_, TRACKSELECT),
    #if (_WIN32_IE >= 0x0400)
      STYLETABENTRY(TVS_, SINGLEEXPAND),
      #ifndef _WIN32_WCE
        STYLETABENTRY(TVS_, INFOTIP),
        STYLETABENTRY(TVS_, FULLROWSELECT),
        STYLETABENTRY(TVS_, NOSCROLL),
        STYLETABENTRY(TVS_, NONEVENHEIGHT),
      #endif
    #endif
    #if (_WIN32_IE >= 0x500)
      STYLETABENTRY(TVS_, NOHSCROLL),
    #endif
    #endif
      STYLETABLEEND
    };



    // Make up the control window style

    DWORD dwStyle = GetCtlStyle(pnmcc) | GetCtlStyle(pnmcc, aStyles);
    DWORD dwExStyle = GetCtlExStyle(pnmcc);

#ifdef FLICKER_FREE_COMCTL // force double buffering for it
    if( IsWindowsXP() ) 
        dwExStyle |= WS_EX_COMPOSITED;
#endif
    // Create control and check if ok

    pnmcc->outControlHwnd = ::CreateWindowEx(
      dwExStyle, CTreeViewCtrl::GetWndClassName(), NULL, dwStyle, 
      0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)(GetAttrInt(pnmcc->helement,"id")), _Module.GetModuleInstance(), 
      pnmcc);

    /*CTreeViewCtrl ctl = pnmcc->outControlHwnd;
    for( int i = 0; i < 30; ++i )
    {
      char buf[64];
      sprintf(buf,"item %d",i);
      ctl.InsertItem(buf,0,0);
    }*/

    if (!pnmcc->outControlHwnd) 
      pnmcc->outControlHwnd = HWND_DISCARD_CREATION;
    
    return 0;
  }

  // Cleanup local defines

  #undef STYLETABENTRY
  #undef STYLETABLEEND

};

}; //namespace WTL

// George - 11.Mar.2005
#ifdef _ATL
#if _ATL_VER >= 7
#undef _Module
#endif
#endif

#endif // __ATLHTMENGINEHOST_H__
