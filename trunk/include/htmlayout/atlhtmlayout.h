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


#ifndef __ATLHTMLAYOUT_H__
#define __ATLHTMLAYOUT_H__

#pragma once

#ifndef __cplusplus
#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
#error atlhtmlayout.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
#error atlhtmlayout.h requires atlwin.h to be included first
#endif

#include "htmlayout.h"

//////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CHTMLayoutCtrlT<TBase> - CHTMLayoutCtrl

namespace WTL
{

template<class TBase>
class CHTMLayoutCtrlT: public TBase
{
public:
// Constructors
  CHTMLayoutCtrlT(HWND hWnd = NULL) : TBase(hWnd)
  { }

  CHTMLayoutCtrlT< TBase >& operator = (HWND hWnd)
  {
    m_hWnd = hWnd;
    return *this;
  }

  HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
        DWORD dwStyle = 0, DWORD dwExStyle = 0, ATL::_U_MENUorID MenuOrId = 0U, LPVOID lpCreateParam = NULL)
  {
    return TBase::Create(GetWndClassName(), hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, MenuOrId.m_hMenu, lpCreateParam);
  }

// Attributes
  static LPCTSTR GetWndClassName()
  {
    return ::HTMLayoutClassNameT();
  }

  BOOL LoadHtmlMemory(LPVOID Data, UINT uDataLength, LPCWSTR lpBaseURL = NULL)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    if (lpBaseURL)
      return ::HTMLayoutLoadHtmlEx(m_hWnd, reinterpret_cast<LPCBYTE>(Data), uDataLength, lpBaseURL);
    else
      return ::HTMLayoutLoadHtml(m_hWnd, reinterpret_cast<LPCBYTE>(Data), uDataLength);
  }

  BOOL LoadHtmlFile(LPCTSTR lpFileName)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    USES_CONVERSION;
    return ::HTMLayoutLoadFile(m_hWnd, T2CW(lpFileName));
  }

  BOOL LoadHtmlResource(LPCTSTR lpResourceName, HMODULE hModule = NULL)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    HRSRC hRsrc;
    HGLOBAL hGRes;
    LPVOID  lpData;
    UINT  uSize;
    if (!hModule)
      hModule = _Module.GetResourceInstance();

    hRsrc = ::FindResource(hModule, lpResourceName, RT_HTML);
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
            return ::HTMLayoutLoadHtml(m_hWnd, reinterpret_cast<LPBYTE>(lpData), uSize);
          }
        }
      }
    }
    return FALSE;
  }

  BOOL LoadHtmlResource(DWORD dwResourceId, HMODULE hModule = NULL)
  {
    return LoadHtmlResource(MAKEINTRESOURCE(dwResourceId), hModule);
  }

  UINT GetDocumentMinWidth()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutGetMinWidth(m_hWnd);
  }

  UINT GetDocumentMinHeight(UINT uWidth)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutGetMinHeight(m_hWnd, uWidth);
  }

  VOID SetSelectionMode()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    ::HTMLayoutSetMode(m_hWnd, /*HTMLayoutModes::*/HLM_SHOW_SELECTION);
  }

  VOID SetNavigationMode()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    ::HTMLayoutSetMode(m_hWnd, /*HTMLayoutModes::*/HLM_LAYOUT_ONLY);
  }

  BOOL SelectionExist()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutSelectionExist(m_hWnd);
  }

  LPCBYTE GetSelectedHtml(UINT& selectedSize)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutGetSelectedHTML(m_hWnd, &selectedSize);
  }

  BOOL SelectionToClipboard()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutClipboardCopy(m_hWnd);
  }

  HELEMENT GetRootElement()
  {
    ATLASSERT(::IsWindow(m_hWnd));
    HELEMENT hRootElement = 0;
    ::HTMLayoutGetRootElement(m_hWnd, &hRootElement);
    return hRootElement;
  }

  LRESULT Proc(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutProc(m_hWnd, uMsg, wParam, lParam);
  }

  LRESULT ProcND(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutProcND(m_hWnd, uMsg, wParam, lParam);
  }

  BOOL DataReady(LPCTSTR lpURI, LPVOID lpData, DWORD dwSize)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    USES_CONVERSION;
    return ::HTMLayoutDataReady(m_hWnd, T2CW(lpURI), reinterpret_cast<LPBYTE>(lpData), dwSize);
  }

  BOOL DataReadyAsync(LPCTSTR lpURI, LPVOID lpData, DWORD dwSize, HTMLayoutResourceType rType)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    USES_CONVERSION;
    return ::HTMLayoutDataReadyAsync(m_hWnd, T2CW(lpURI), reinterpret_cast<LPBYTE>(lpData), dwSize, rType);
  }

  static BOOL SetMasterCSS(LPVOID lpData, UINT uSize)
  {
    return ::HTMLayoutSetMasterCSS(reinterpret_cast<LPCBYTE>(lpData), uSize);
  }

  BOOL SetCSS(LPVOID lpData, UINT uSize)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutSetCSS(m_hWnd, reinterpret_cast<LPCBYTE>(lpData), uSize);
  }

  BOOL SetMediaType(LPCTSTR lpMediaType)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    USES_CONVERSION;
    return ::HTMLayoutSetMediaType(m_hWnd, T2CW(lpMediaType));
  }

  BOOL SetHttpHeaders(LPCTSTR lpHeaders, UINT uSize)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    USES_CONVERSION;
    return ::HTMLayoutSetHttpHeaders(m_hWnd, T2CA(lpHeaders), uSize);
  }

  BOOL SetOption(HTMLAYOUT_OPTIONS Option, UINT uValue)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutSetOption(m_hWnd, Option, uValue);
  }

  BOOL Render(HBITMAP hBitmap, RECT Rect)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutRender(m_hWnd, hBitmap, Rect);
  }

  VOID SetCallback(LPHTMLAYOUT_NOTIFY lpNotify, LPVOID lpUserData)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    ::HTMLayoutSetCallback(m_hWnd, lpNotify, lpUserData);
  }

  UINT EnumResources(HTMLAYOUT_CALLBACK_RES *lpCallback)
  {
    ATLASSERT(::IsWindow(m_hWnd));
    return ::HTMLayoutEnumResources(m_hWnd, lpCallback);
  }
protected:
private:
};

typedef CHTMLayoutCtrlT<ATL::CWindow> CHTMLayoutCtrl;

#define CHAIN_TO_HTMLAYOUT() \
  { \
    BOOL bHandled = FALSE; \
    lResult = ::HTMLayoutProcND(hWnd,uMsg, wParam, lParam, &bHandled); \
    if(bHandled) return TRUE; \
  } \


}

#endif // __ATLHTMLAYOUT_H__
