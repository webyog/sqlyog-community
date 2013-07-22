// Windows Template Library Extension
// for Terra Informatica Lightweight Embeddable HTMLayout control
// Copyright (C) 2006, Andrew Fedoniouk <andrew@TerraInformatica.com>
//
// Written by Alexander Murashko (LeonCrew)
//
// This file is NOT part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// Copyright (C) 2006, Alexander Murashko <leoncrew@tut.by>


#ifndef __ATLHTMLAYOUTHOST_H__
#define __ATLHTMLAYOUTHOST_H__

#pragma once

#ifndef __cplusplus
#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
#error atlhtmlayouthost.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
#error atlhtmlayouthost.h requires atlwin.h to be included first
#endif

#include "htmlayout.h"
#include <strsafe.h>
//#include <string.h>

//////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CHTMLayoutHost<T>

namespace WTL
{

template<class T>
class CHTMLayoutHost
{
public:
  VOID SetCallback()
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    ::HTMLayoutSetCallback(pT->m_hWnd, CallbackHost, (CHTMLayoutHost<T>*)this);
  }

  /*
  ** Attaches event handler to the window.
  ** Such event handler is will survive document reloads
  ** event_handler handles HTML bubbling events.
  */
  VOID AttachEventHandler(htmlayout::event_handler *lpEventHandler,  UINT subscription = HANDLE_ALL)
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    ::HTMLayoutWindowAttachEventHandler(pT->m_hWnd, &htmlayout::event_handler::element_proc, lpEventHandler, subscription);
  }

  VOID DetachEventHandler(htmlayout::event_handler *lpEventHandler, UINT subscription = HANDLE_ALL)
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    ::HTMLayoutWindowDetachEventHandler(pT->m_hWnd, &htmlayout::event_handler::element_proc, lpEventHandler);
  }

  virtual LRESULT OnHtmlNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    ATLASSERT(uMsg == WM_NOTIFY);

    switch(((NMHDR*)lParam)->code)
    {
    case HLN_CREATE_CONTROL:
      return OnCreateControl((LPNMHL_CREATE_CONTROL)lParam);
    case HLN_CONTROL_CREATED:
      return OnControlCreated((LPNMHL_CREATE_CONTROL)lParam);
    case HLN_DESTROY_CONTROL:
      return OnDestroyControl((LPNMHL_DESTROY_CONTROL)lParam);
    case HLN_LOAD_DATA:
      return OnLoadData((LPNMHL_LOAD_DATA)lParam);
    case HLN_DATA_LOADED:
      return OnDataLoaded((LPNMHL_DATA_LOADED)lParam);
    case HLN_DOCUMENT_COMPLETE:
      return OnDocumentComplete();
    case HLN_ATTACH_BEHAVIOR:
      return OnAttachBehavior((LPNMHL_ATTACH_BEHAVIOR)lParam);
    case HLN_DIALOG_CREATED:
      return OnDialogCreated((NMHDR*)lParam);
    case HLN_DIALOG_CLOSE_RQ:
      return OnDialogCloseRQ((LPNMHL_DIALOG_CLOSE_RQ)lParam);
    }

    return OnHtmlGenericNotifications(uMsg, wParam, lParam);
  }

  virtual LRESULT OnHtmlGenericNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    HWND hWndParent = pT->GetParent();
    if (::IsWindow(hWndParent))
      return ::SendMessage(hWndParent, uMsg, wParam, lParam);
    return 0;
  }

  virtual LRESULT OnCreateControl(LPNMHL_CREATE_CONTROL pnmcc)
  {
    htmlayout::dom::element el = pnmcc->helement;
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnCreateControl: type = \"%s\"\n"), A2CT(el.get_element_type()));
    return 0;
  }

  virtual LRESULT OnControlCreated(LPNMHL_CREATE_CONTROL pnmcc)
  {
    htmlayout::dom::element el = pnmcc->helement;
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnControlCreated: type = \"%s\"\n"), A2CT(el.get_element_type()));
    return 0;
  }

  virtual LRESULT OnDestroyControl(LPNMHL_DESTROY_CONTROL pnmdc)
  {
    htmlayout::dom::element el = pnmdc->helement;
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnDestroyControl: type = \"%s\", HWND = 0x%X\n"), A2CT(el.get_element_type()), pnmdc->inoutControlHwnd);
    return 0;
  }

  virtual LRESULT OnLoadData(LPNMHL_LOAD_DATA pnmld)
  {
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnLoadData: uri = \"%s\"\n"), W2CT(pnmld->uri));
    T* pT = static_cast<T*>(this);
    ATLASSERT(::IsWindow(pT->m_hWnd));
    return LoadDataDefault(pnmld);
  }

  virtual LRESULT OnDataLoaded(LPNMHL_DATA_LOADED pnmdl)
  {
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnDataLoaded: uri = \"%s\" (%d bytes)\n"), W2CT(pnmdl->uri), pnmdl->dataSize);
    return 0;
  }

  virtual LRESULT OnDocumentComplete()
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDocumentComplete\n"));
    return 0;
  }

  virtual LRESULT OnAttachBehavior(LPNMHL_ATTACH_BEHAVIOR pnmab)
  {
    USES_CONVERSION;
    ATLTRACE(_T("CHTMLayoutHost::OnAttachBehavior: behavior = \"%s\""), A2CT(pnmab->behaviorName));
    if (!htmlayout::behavior::handle(pnmab))
    {
      ATLTRACE(_T(" - not found\n"));
    }
    else
    {
      ATLTRACE(_T("\n"));
    }
    return 0;
  }

  virtual LRESULT OnDialogCreated(NMHDR* pnmdlg)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDialogCreated: HWND = 0x%X, Id = %d, code = %d\n"), pnmdlg->hwndFrom, pnmdlg->idFrom, pnmdlg->code);
    return 0;
  }

  virtual LRESULT OnDialogCloseRQ(LPNMHL_DIALOG_CLOSE_RQ pnmdcrq)
  {
    ATLTRACE(_T("CHTMLayoutHost::OnDialogCloseRQ: HWND = 0x%X, Id = %d, code = %d\n"), pnmdcrq->hdr.hwndFrom, pnmdcrq->hdr.idFrom, pnmdcrq->hdr.code);
    return 0;
  }

  /*
  ** Load from resources: res:MyModule.dll/SomeFile.dat or res:SomeFile.dat (current module)
  **      in MyModule.rc:  SomeFile  DAT  "SomeFile.dat"
  ** SomeFile.htm or SomeFile.html or SomeFile load from RC_HTML resource type
  */

  static LRESULT LoadDataDefault(LPNMHL_LOAD_DATA pnmld)
  {
    USES_CONVERSION;
    size_t  uriLength;
    BOOL  bReturn;
    ATLASSERT(::IsWindow(pnmld->hdr.hwndFrom));
    
    if (::StringCchLengthW(pnmld->uri, STRSAFE_MAX_CCH, &uriLength) != S_OK) return LOAD_DISCARD;

    if (!::wcsncmp(pnmld->uri, L"res:", 4))
    {
      HMODULE hModule;
      HRSRC hRsrc;
      HGLOBAL hGRes;
      LPVOID  lpData;
      UINT  uSize;

      WCHAR lpModuleName[MAX_PATH];
      LPWSTR  lpResourceName;
      LPWSTR  lpResourceType;
      size_t  i, resLength;

      bReturn = FALSE;
      lpModuleName[0] = 0;
      lpResourceName = NULL;
      lpResourceType = NULL;

      for (i = uriLength - 1; i >= 4; i--)
      {
        if (pnmld->uri[i] == L'/' || pnmld->uri[i] == L'\\')
        {
          ::memcpy(lpModuleName, &pnmld->uri[4], sizeof(WCHAR) * (uriLength - 4 + 1));
          lpModuleName[i - 4] = 0;
          lpResourceName = &lpModuleName[i - 3];
          resLength = uriLength - i - 1;
          break;
        }
      }

      if (i < 4)
      {
        ::StringCchCopyW(&lpModuleName[1], sizeof(lpModuleName) / sizeof(lpModuleName[0]) - 1, &pnmld->uri[4]);
        lpResourceName = &lpModuleName[1];
        ::StringCchLengthW(lpResourceName, sizeof(lpModuleName) / sizeof(lpModuleName[0]) - 1, &resLength);
      }

      if (lpResourceName[0])
      {
        for (i = resLength - 1; i > 0; i --)
        {
          if (lpResourceName[i] == L'.')
          {
            lpResourceName[i] = 0;
            lpResourceType = &lpResourceName[i + 1];
            break;
          }
        }

        if (lpModuleName[0])
          hModule = ::LoadLibraryEx(W2CT(lpModuleName), NULL, LOAD_LIBRARY_AS_DATAFILE);
        else
          hModule = _Module.GetResourceInstance();

        if (hModule)
        {
          if (!lpResourceType || !::wcscmp(lpResourceType, L"htm") || !::wcscmp(lpResourceType, L"html"))
            hRsrc = ::FindResource(hModule, W2CT(lpResourceName), RT_HTML);
          else
            hRsrc = ::FindResource(hModule, W2CT(lpResourceName), W2CT(lpResourceType));
          if (hRsrc)
          {
            hGRes = ::LoadResource(hModule, hRsrc);
            if (hGRes)
            {
              uSize = ::SizeofResource(hModule, hRsrc);
              if (uSize)
              {
                lpData = ::LockResource(hGRes);
                if (lpData)
                {
                  bReturn = ::HTMLayoutDataReady(pnmld->hdr.hwndFrom, pnmld->uri, reinterpret_cast<LPBYTE>(lpData), uSize);
                }
              }
            }
          }
          if (lpModuleName[0])
            ::FreeLibrary(hModule);
        }
      }
    }
    else
      bReturn = TRUE;

    return bReturn ? LOAD_OK : LOAD_DISCARD;
  }

protected:
  static LRESULT CALLBACK CallbackHost(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID lpData)
  {
    ATLASSERT(lpData);
    CHTMLayoutHost<T>* pThis = static_cast< CHTMLayoutHost<T>* >(lpData);
    return pThis->OnHtmlNotify(uMsg, wParam, lParam);
  }
private:
};

}

#endif // __ATLHTMLAYOUTHOST_H__
