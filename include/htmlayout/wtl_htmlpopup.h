//
// Windows Template Library Extension for
// Terra Informatica Lightweight Embeddable HTMLayout control
// http://htmlayout.com
//
// Written by Andrew Fedoniouk / <andrew@terrainformatica.com>
//
// This file is NOT part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// (C) 2003, 2004, Andrew Fedoniouk <andrew@TerraInformatica.com>
//


#ifndef __WTL_HTMLPOPUP_H__
#define __WTL_HTMLPOPUP_H__

#pragma once

#ifndef __cplusplus
  #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
  #error wtl_htmlpopup.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
  #error wtl_htmlpopup.h requires atlwin.h to be included first
#endif

#include <atlmisc.h>

#include "wtl_htmlayout.h"
#include "wtl_htmlayouthost.h"


#define DECLARE_WND_CLASS_EX_XP(WndClassName, style, style_XP, bkgnd) \
static CWndClassInfo& GetWndClassInfo() \
{ \
  static CWndClassInfo wc = \
  { \
      { sizeof(WNDCLASSEX), IsWindowsXP()?style_XP:style, StartWindowProc, \
      0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL }, \
    NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
  }; \
  return wc; \
}


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CHTMLPopup - popup html window,
//        Could be used as e.g. HTML tooltip or
//        dropdown window
//

namespace WTL
{

class CHTMLPopup : 
    public CWindowImpl<CHTMLPopup>,
    public CHTMLayoutHost<CHTMLPopup>
  {
    volatile DWORD m_signature;
  public:

  DECLARE_WND_CLASS_EX_XP(TEXT("HTMLayoutPopup"),
    CS_SAVEBITS | CS_CLASSDC,
    CS_SAVEBITS | CS_CLASSDC | CS_DROPSHADOW, COLOR_WINDOW)

    BOOL  m_autoDelete;
    
    // Constructors

    CHTMLPopup(BOOL autoDelete = FALSE): m_autoDelete(autoDelete) { }

    void Show(HWND hOwner, RECT rcOwnerArea, _U_STRINGorID HTMLresIDorName)
    {
      PBYTE pb;
      DWORD cb;
      if(GetHtmlResource(HTMLresIDorName.m_lpstr, pb, cb, NULL))
      {
        Show(hOwner, rcOwnerArea, pb, cb);
        return;
      }
      ATLASSERT(FALSE);
    }

    void Show(HWND hOwner, RECT rcOwnerArea, LPCBYTE html, DWORD htmlSize)
    {
        ATLASSERT(m_hWnd == 0);

        /*CWindow::Create((TEXT("HTMLayoutPopup"), hOwner, rcDefault, 
           NULL, WS_POPUP | WS_CLIPCHILDREN, 0, NULL, NULL);
           */
        Create(hOwner, rcDefault, NULL, WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
        
        SetCallback(); // we will receive notifications
                       // in this message map
        CHTMLayoutCtrl self = m_hWnd;

        if(!self.LoadHtml(html, htmlSize))
        {
          const char *message = "Got a <b>problem</b>!";
          self.LoadHtml((LPCBYTE)message, strlen(message));
        }

        // we expect that html has dimensions defined
        int width = self.GetWindowWidth();
        int height = self.GetWindowHeight(width);

        CWindow wOwner(hOwner);
        RECT rc = rcOwnerArea;
        wOwner.MapWindowPoints(m_hWnd,&rc);

        //TODO! check if window is inside display area
        SetWindowPos(NULL, 
          rc.left,
          rc.bottom + 1,
          width,
          height,
          SWP_SHOWWINDOW);
                
    }

    void Hide() { SendMessage(WM_CLOSE,0,0); }

    BEGIN_MSG_MAP(CHTMLPopup)
      CHAIN_TO_HTMLAYOUT() // this must be first item!
      MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
    END_MSG_MAP()
    
    LRESULT OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
      if( LOWORD( wParam ) == WA_INACTIVE )
        Hide(); // this window was deactivated somehow, we are done!
      else 
        bHandled = FALSE;
      return 0;
    }
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
      DestroyWindow();
      return 0;
    }

    virtual void OnFinalMessage(HWND /*hWnd*/)
    {
      if(m_autoDelete)
      {
        ATLASSERT( m_signature == 0xAFEDDADA );
        // this instance was created on stack! 
        // but it should be created on heap through 
        // new CHTMLPopup(TRUE);
        if( m_signature == 0xAFEDDADA )
          delete this;
      }
    }

    void *operator new( size_t stAllocateBlock ) 
    {
      void *p = ::operator new(stAllocateBlock);
      if(p) static_cast<CHTMLPopup*>(p)->m_signature = 0xAFEDDADA;
      return p;
    }
    void operator delete( void *p ) { ::operator delete(p); }


        
  };


}

#endif // __WTL_HTMLayout_H__
