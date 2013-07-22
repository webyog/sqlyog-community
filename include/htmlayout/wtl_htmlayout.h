//
// Windows Template Library Extension for
// Terra Informatica Lightweight Embeddable HTMLayout control
// http://htmlayout.com
//
// Written by Andrew Fedoniouk / <andrew@terrainformatica.com>
// Portions: Pete Kvitek <pete@kvitek.com>  
//
// This file is NOT part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// (C) 2003, Andrew Fedoniouk <andrew@TerraInformatica.com>
//

#ifndef __WTL_HTMLAYOUT_H__
#define __WTL_HTMLAYOUT_H__

#pragma once

#ifndef __cplusplus
  #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
  #error wtl_htmlayout.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
  #error wtl_htmlayout.h requires atlwin.h to be included first
#endif

#include <atlmisc.h>

#if _ATL_VER >= 0x0700 
#define _Module _AtlBaseModule
#endif


#include "htmlayout.h"
#include "wtl_value.h"

/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CHTMLayoutCtrlT<TBase>
// CHTMLayoutDelegate
//

namespace WTL
{

bool GetHtmlResource(LPCTSTR pszName, /*out*/PBYTE& pb, /*out*/DWORD& cb, HMODULE hModule = NULL);


/////////////////////////////////////////////////////////////////////////////
// CHTMLayoutCtrl - client side for a HTMLayout control

template <class TBase>
class CHTMLayoutCtrlT : public TBase
{
public:

  // Constructors

  CHTMLayoutCtrlT(HWND hWnd = NULL) : TBase(hWnd) { }

  CHTMLayoutCtrlT< TBase >& operator=(HWND hWnd)
  {
    m_hWnd = hWnd;
    return *this;
  }

  HWND CreateLayout(HWND hWndParent, _U_RECT rect = NULL, 
      LPCTSTR szWindowName = NULL,
      DWORD dwStyle = 0, DWORD dwExStyle = 0,
      _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
  {
    CWindow::Create(GetWndClassName(), hWndParent, rect.m_lpRect, 
      szWindowName, dwStyle | WS_CHILD, dwExStyle, MenuOrID.m_hMenu, lpCreateParam);

    return m_hWnd;
  }
  
  // Attributes

 
  static LPCTSTR GetWndClassName()
  {
    return ::HTMLayoutClassNameT();
  }

  bool LoadHtml(LPCBYTE pb, DWORD nBytes, LPCWSTR baseUrl = 0)
  {
    SetupMediaType();
    ATLASSERT(::IsWindow(m_hWnd));
    return (baseUrl? 
      ::HTMLayoutLoadHtmlEx(m_hWnd, pb, nBytes, baseUrl):
      ::HTMLayoutLoadHtml(m_hWnd, pb, nBytes)) != 0;
  }

  bool LoadHtmlResource(LPCTSTR pszName, HMODULE hModule = NULL)
  {
    SetupMediaType();
    // This code assumes that the host and control windows are the same
    ATLASSERT(::IsWindow(m_hWnd));

    LPBYTE pb;
    DWORD  cb;

    if(!GetHtmlResource(pszName, pb, cb, hModule))
      return false;

    //if(strstr((const char*)pb,"<%include ") != NULL) - engine suppoort <include src=""> internally 
    //  return LoadHtmlResourceWithIncludes(pb, cb);

    return LoadHtml(pb, cb);
  }

  bool LoadHtmlResource(DWORD resID, HMODULE hModule = NULL)
  {
    return LoadHtmlResource(MAKEINTRESOURCE(resID), hModule);
  }


  HELEMENT GetRootElement()
  {
    HELEMENT he = 0;
    ::HTMLayoutGetRootElement(m_hWnd,&he);
    return he;
  }



  static inline void AppendBytes(CSimpleValArray<BYTE>& buf, const BYTE* pb, DWORD cb)
  {
    while(cb--)
      buf.Add(*pb++);
  }

  // process <%include some.html%> 

  // <OBSOLETE> use builtin <include src> instead 
  /*
  bool LoadHtmlResourceWithIncludes(const BYTE* pb, DWORD cb)
  {
    CSimpleValArray<BYTE> buf;

    const BYTE *pend = pb + cb;
    
    while(pb < pend)
    {
      const BYTE* insPoint = (const BYTE*) strstr((const char*)pb,"<%include ");
      if(insPoint == NULL) 
      {
        // no more <%include found...
        AppendBytes( buf, pb, pend - pb );
        break;
      }
      else
      {
        AppendBytes( buf, pb, insPoint - pb );
        insPoint += 10; //sizeof("<%include ")
        const BYTE *tail = (const BYTE*) strstr( (const char*) insPoint," %>");
        ATLASSERT(tail);
        CString resName( (const char *)insPoint, tail - insPoint);
        PBYTE pbh;
        DWORD cbh;
        if(GetHtmlResource(resName, pbh, cbh))
          AppendBytes( buf, pbh, cbh );
        else
          buf.Add('?');
        pb = tail + 3; //" %>"
      }
    }
    return LoadHtml(buf.GetData(),  buf.GetSize());
  }
  */
  // </OBSOLETE>


  bool OpenFile(LPCTSTR lpszFilePath)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    SetupMediaType();
    USES_CONVERSION;
    return ::HTMLayoutLoadFile(m_hWnd,T2W(const_cast<LPTSTR>(lpszFilePath))) == TRUE;
  }

  unsigned int GetDocumentMinWidth()      
  { 
    return ::HTMLayoutGetMinWidth(m_hWnd); 
  }
  unsigned int GetDocumentMinHeight(unsigned int width) 
  { 
    return ::HTMLayoutGetMinHeight(m_hWnd, width); 
  }

  // this function will return width of the HTMLayout window needed to 
  // show document in full width - which means without horizontal scrollbar 
  int     GetWindowWidth()  
  { 
    CRect rc(0,0,0,0);
    AdjustWindowRectEx(&rc, GetWindowLong(GWL_STYLE),GetMenu() != NULL, GetWindowLong(GWL_EXSTYLE));
    return GetDocumentMinWidth() + rc.Width();
  }

  // this function will return height of HTMLayout window for proposed width
  // needed to show the document in full thus without vertical scrollbar.
  int     GetWindowHeight(int windowWidth)  
  { 
    CRect rc(0,0,0,0);
    AdjustWindowRectEx(&rc, GetWindowLong(GWL_STYLE),GetMenu() != NULL, GetWindowLong(GWL_EXSTYLE));
    int clientWidth = windowWidth - rc.Width();
    int h = GetDocumentMinHeight(clientWidth);
    h += rc.Height(); // add borders extra
    if(clientWidth < (int)GetDocumentMinWidth()) // horz scrollbar will appear, add its height
      h += GetSystemMetrics(SM_CYHSCROLL);
    return h;
  }


  //  George [12/3/2005, 19:39]
  //  a few additions for methods not present at the time of writing
  
  void SetSelectionMode()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    ::HTMLayoutSetMode(m_hWnd, 1);
  }
  void SetNavigationMode()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    ::HTMLayoutSetMode(m_hWnd, 0);
  }

  bool CopySelection()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutClipboardCopy(m_hWnd) != 0; // true is selection exist
  }

  // George [20/3/2005, 23:41]
  // An enhanced version for loading resource data
  /*
  bool LoadFromResource(LPCWSTR URI)
  {
    xool::ustring uri ( URI );
    if (!uri.like(L"res:*"))
      return false;

    // First, strip out protocol
    int iPos = uri.index_of(L':');
    if (iPos++ < 0)
      return false;

    uri = uri.replace(L'\\', L'/');
    uri = uri.substr(iPos);

    // Find out the resource file:
    iPos = uri.last_index_of('/');
    if ( (iPos++ < 0) || (iPos == uri.length()) )
      return false;

    xool::ustring resName = uri.substr(iPos);

    HMODULE hModule = NULL;

    if (iPos > 1)
    {
      uri = uri.substr(0, iPos-1);

      while ( uri.length() && uri[0] == L'/' )
        uri = uri.substr(1);

      if (uri.length() > 0)
      {
        // Replace it back since now we're dealing with real files
        xool::string filename = uri.replace(L'/', L'\\');

        // First, check if the file exists
        WIN32_FIND_DATA wfd;
        HANDLE hFile = FindFirstFile( filename, &wfd );

        // Oops, file not found. Out'a here
        if (hFile == INVALID_HANDLE_VALUE)
          return false;

        if (FindClose(hFile) == FALSE)
          ATLASSERT(FALSE);

        hModule = ::LoadLibraryEx( (LPCSTR)filename, NULL, LOAD_LIBRARY_AS_DATAFILE);
      }
    }

    // Find the resource's extension, so we know what to look for
    xool::string ext;
    xool::string name = resName;
    iPos = resName.last_index_of(L'.');
    if (iPos++ > 0)
      ext = resName.substr(iPos);

    if (hModule == NULL)
      hModule = _Module.GetResourceInstance();

    // Find the resource
    HRSRC hResInfo = 0;
    if ( ext.length() )
    {
      hResInfo = FindResource(hModule, name, ext);
    }

    if (hResInfo == NULL)
    {
      if (hModule != _Module.GetResourceInstance())
        FreeLibrary(hModule);
      return false;
    }

    // and load it
    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    if (hResData == NULL)
    {
      if (hModule != _Module.GetResourceInstance())
        FreeLibrary(hModule);
      return false;
    }

    xool::byte* pByte = (PBYTE)::LockResource(hResData);
    xool::dword dwSize = ::SizeofResource(hModule, hResInfo);

    // Load it from here...
    BOOL bRez = ::HTMLayoutDataReady(m_hWnd, URI, pByte, dwSize);

    if (hModule != _Module.GetResourceInstance())
      FreeLibrary(hModule);

    return bRez == 1 ? true : false;
  }
  */


  // Methods

  bool CALLBACK HTMLayoutDataReady(LPCWSTR uri, LPBYTE data, DWORD dataLength)
  {
    return ::HTMLayoutDataReady(m_hWnd, uri, data, dataLength) != FALSE;
  }

  LPCBYTE GetSelectedHTML(LPUINT dataLength)
  {
    return HTMLayoutGetSelectedHTML(m_hWnd,dataLength);
  }


  LRESULT CALLBACK HTMLayoutProc(UINT msg, WPARAM wParam, LPARAM lParam)
  {
    return ::HTMLayoutProc(m_hWnd, msg, wParam, lParam);
  }

  // Find HWND of child ctl by its name. 
  // nameTemplate could be literal name or template like "salary*" - to get
  // control which name looks like "salary..." e.g. "salaryHigh" and "salaryLow"
  // If there are multiple ctls which names are matching given criteria
  // you may use parameter index to select particular ctl in the name group.
  // To find max value of the index use GetDlgItemNameCount function.
  HWND    GetDlgItemByName(LPCSTR nameTemplate, int index = 0) 
  { 
    findItemHWndByName fc(index);
    
    USES_CONVERSION;

    htmlayout::dom::element root = htmlayout::dom::element::root_element(m_hWnd);
    root.select(&fc, "input,widget,textarea,select,iframe", 
        "name", A2CW(nameTemplate));
    return fc.hWndFound;  
  }
  // Get number of ctls which names are matching nameTemplate criteria
  int     GetDlgItemNameCount(LPCSTR nameTemplate) 
  { 
    findItemsCountByName fc;

    USES_CONVERSION;

    htmlayout::dom::element root = htmlayout::dom::element::root_element(m_hWnd);
    root.select(&fc, "input,widget,textarea,select,iframe", 
        "name", A2CW(nameTemplate));
    return fc.counter;  
  }
  // Get HTML name of ctl by its HWND 
  LPCWSTR GetDlgItemName(HWND hwnd) 
  { 
    findElementByHwnd fc(hwnd);
    htmlayout::dom::element root = htmlayout::dom::element::root_element(m_hWnd);
    root.select(&fc, "input,widget,textarea,select,iframe");

    if(fc.found.is_valid()) 
    {
      return fc.found.get_attribute("name");
    }
    return NULL;
  }

  // Get HTML name of ctl by its HWND 
  htmlayout::dom::element GetDlgItemElement(HWND hwnd) 
  { 
    findElementByHwnd fc(hwnd);
    htmlayout::dom::element root = htmlayout::dom::element::root_element(m_hWnd);
    root.select(&fc, "input,widget,textarea,select,iframe");
    return fc.found;
  }

  CString GetDlgItemStringAttribute(HWND hwnd, LPCSTR name) 
  { 
    htmlayout::dom::element found = GetDlgItemElement(hwnd);
    if(found.is_valid()) 
      return found.get_attribute(name);
    return CString();
  }
  int GetDlgItemIntAttribute(HWND hwnd, LPCSTR name) 
  { 
    htmlayout::dom::element found = GetDlgItemElement(hwnd);
    if(found.is_valid()) 
      return _wtoi(found.get_attribute(name));
    return 0;
  }


  // get/set control value
  // <OBSOLETE!>
  // use htmlayout::get_values & co.

  CValue  GetDlgItemValue(HWND hwndControl);
  void    SetDlgItemValue(HWND hwndControl, CValue v);

  CValue  GetDlgItemValue(LPCTSTR name);
  void    SetDlgItemValue(LPCTSTR name, CValue v);

  void    GetDlgItemValues(CSimpleMap<CString,CValue>& bag);
  void    SetDlgItemValues(const CSimpleMap<CString,CValue>& bag);
  // </OBSOLETE!>
  
  
  void    SetupMediaType()
  {
#ifndef _WIN32_WCE
    ATLASSERT(::IsWindow(m_hWnd));
    SIZED_STRUCT(HIGHCONTRAST, hc);
    ::SystemParametersInfo(SPI_GETHIGHCONTRAST, 0, &hc, 0);
    LPCWSTR mediaType = hc.dwFlags & HCF_HIGHCONTRASTON?L"contrast-screen":L"screen";
    ::HTMLayoutSetMediaType(m_hWnd, mediaType);
#endif
  }

private:

  // dom callback helpers
  struct  findItemHWndByName: htmlayout::dom::callback 
  {
    HWND hWndFound;
    int  nIndexRequested;
    int  nIndex;
    findItemHWndByName(int index = 0):hWndFound(0),nIndexRequested(index),nIndex(0) {}
    inline bool on_element(HELEMENT he) 
    { 
      if(nIndexRequested == nIndex)
      {
        htmlayout::dom::element el = he;
        hWndFound = el.get_element_hwnd(false);
        return true; /*stop enumeration*/ 
      }
      ++nIndex;
      return false;
    }
  };
  struct  findItemsCountByName: htmlayout::dom::callback 
  {
    int  counter;
    findItemsCountByName():counter(0) {}
    inline bool on_element(HELEMENT he) { ++counter; return false; }
  };

  struct  findElementByHwnd: htmlayout::dom::callback 
  {
    HWND      hwnd;
    htmlayout::dom::element  found;
    findElementByHwnd(HWND hWnd):hwnd(hWnd), found(0) {}
    inline bool on_element(HELEMENT he) 
    { 
      htmlayout::dom::element el = he;
      if(hwnd == el.get_element_hwnd(false))
      {
        found = el;
        return true; // stop, found
      }
      return false; 
    }
  };

};

typedef CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE> CHTMLayoutWinTraits;
typedef CHTMLayoutCtrlT<CWindow> CHTMLayoutCtrl;

#ifndef RT_HTML
  #define RT_HTML         MAKEINTRESOURCE(23)
#endif

inline bool GetHtmlResource(LPCTSTR pszName, /*out*/PBYTE& pb, /*out*/DWORD& cb, HMODULE hModule)
{
  ATLASSERT(pszName != NULL);

  // Find specified resource and check if ok

  if(!hModule)
    hModule = _Module.GetResourceInstance();
  
  HRSRC hrsrc = ::FindResource(hModule, pszName, MAKEINTRESOURCE(RT_HTML));

  if(!hrsrc) 
    return false;

  // Load specified resource and check if ok
  
  HGLOBAL hgres = ::LoadResource(hModule, hrsrc);
  if(!hgres) return false;

  // Retrieve resource data and check if ok

  pb = (PBYTE)::LockResource(hgres); if (!pb) return false;
  cb = ::SizeofResource(hModule, hrsrc); if (!cb) return false;

  return true;
}

} //namespace WTL

//
// "Delegate" handling of painting,resizing and other stuff to HTMLayout winproc,
// Any CWindowImpl derived container can be converted into HTMLayout by 
// including CHAIN_TO_HTMLAYOUT() into message map
// Example: to assign HTML layout functionality to some Dialog it is enough 
//          to declare it as:
//
//    class CAboutDlg : 
//      public CDialogImpl<CAboutDlg, CHTMLayoutCtrl>,
//      public CHTMLayoutHost<CAboutDlg>, ...
//
//    ....
//
//    BEGIN_MSG_MAP(CMainDlg)
//       CHAIN_TO_HTMLAYOUT()
//       MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
//
// This macro must be first very first one in message map
#define CHAIN_TO_HTMLAYOUT() \
  { \
    BOOL bHandled = FALSE; \
    lResult = ::HTMLayoutProcND(hWnd,uMsg, wParam, lParam, &bHandled); \
    if(bHandled) return TRUE; \
} \

#include "wtl_htmlayoutvalues.h"

// GEORGE
#ifndef HTMENGINE_STATIC_LIB

// Link against HTMLayout library
#pragma comment(lib, "HTMLayout.lib")

#endif

#ifndef RT_HTML
#define RT_HTML         MAKEINTRESOURCE(23)
#endif

#endif // __WTL_HTMLayout_H__
