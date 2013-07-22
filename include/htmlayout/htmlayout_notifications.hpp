/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * HTMLayout Implementation of standard notifications.
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2004, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief Standard implementation of HTMLAYOUT_NOTIFY handler.
 **/

#ifndef __htmlayout_notifications_hpp__
#define __htmlayout_notifications_hpp__

#include "htmlayout_dom.h"
#include "htmlayout_aux.h"
#include "htmlayout_behavior.hpp"
#include <assert.h>

#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) //'strcpy' was declared deprecated
#pragma warning(disable:4100) //unreferenced formal parameter 

#pragma once

typedef void* HTMPRINT;

/**HTMLayout namespace.*/
namespace htmlayout
{

  inline bool load_resource_data(LPCWSTR uri, PBYTE& pb, DWORD& cb );
  inline bool load_html_resource(UINT resId, PBYTE& pb, DWORD& cb );


/** \struct notification_handler
 *  \brief tandard implementation of HTMLAYOUT_NOTIFY handler.
 *  Supposed to be used as a C++ mixin, see: <a href="http://en.wikipedia.org/wiki/Curiously_Recurring_Template_Pattern">CRTP</a>
 **/
  template <typename BASE>
    struct notification_handler
  {

      void setup_callback(HWND hwnd)
      {
        HTMLayoutSetCallback(hwnd,callback, static_cast< BASE* >( this ) );
      }

      void setup_callback(HTMPRINT printex)
      {
        HTMPrintSetCallback(printex,callback, static_cast< BASE* >( this ) );
      }


      // HTMLayout callback
      static LRESULT CALLBACK callback(UINT uMsg, WPARAM wParam, LPARAM lParam, LPVOID vParam)
      {
          assert(vParam);
          BASE* self = static_cast<BASE*>(vParam);
          return self->handle_notification(uMsg, wParam, lParam);
      }

      // notifiaction cracker
      LRESULT handle_notification(UINT uMsg, WPARAM wParam, LPARAM lParam)
      {
        assert(uMsg == WM_NOTIFY);

        // Crack and call appropriate method
    
        // here are all notifiactions
        switch(((NMHDR*)lParam)->code) 
        {
          case HLN_CREATE_CONTROL:    return on_create_control((LPNMHL_CREATE_CONTROL) lParam);
          case HLN_CONTROL_CREATED:   return on_control_created((LPNMHL_CREATE_CONTROL) lParam);
          case HLN_DESTROY_CONTROL:   return on_destroy_control((LPNMHL_DESTROY_CONTROL) lParam);
          case HLN_LOAD_DATA:         return on_load_data((LPNMHL_LOAD_DATA) lParam);
          case HLN_DATA_LOADED:       return on_data_loaded((LPNMHL_DATA_LOADED)lParam);
          case HLN_DOCUMENT_COMPLETE: return on_document_complete();
          case HLN_DOCUMENT_LOADED:   return on_document_loaded();
          case HLN_ATTACH_BEHAVIOR:   return on_attach_behavior((LPNMHL_ATTACH_BEHAVIOR)lParam );
          case HLN_DIALOG_CREATED:    return on_dialog_created((NMHDR*)lParam);
          case HLN_DIALOG_CLOSE_RQ:   return on_dialog_close((LPNMHL_DIALOG_CLOSE_RQ)lParam);
        }
        return 0;
      }

      // Overridables 
      
      

      virtual LRESULT on_create_control(LPNMHL_CREATE_CONTROL pnmcc) {  return 0;  }
      virtual LRESULT on_control_created(LPNMHL_CREATE_CONTROL pnmcc) {  return 0;  }
      virtual LRESULT on_destroy_control(LPNMHL_DESTROY_CONTROL pnmhl) { return 0; }
      
      virtual LRESULT on_load_data(LPNMHL_LOAD_DATA pnmld)
      {
        PBYTE pb; DWORD cb;
        if(load_resource_data(pnmld->uri, pb, cb))
          ::HTMLayoutDataReady(pnmld->hdr.hwndFrom, pnmld->uri, pb,  cb);
        return LOAD_OK;
      }

      virtual LRESULT on_data_loaded(LPNMHL_DATA_LOADED pnmld)  { return 0; }
      virtual LRESULT on_document_complete() { return 0; }
      virtual LRESULT on_document_loaded() { return 0; }

      virtual LRESULT on_attach_behavior( LPNMHL_ATTACH_BEHAVIOR lpab )
      {
        htmlayout::behavior::handle(lpab);
        return 0;
      }

      virtual LRESULT on_dialog_created(NMHDR*) { return 0;  }
      virtual LRESULT on_dialog_close( LPNMHL_DIALOG_CLOSE_RQ pnmld) { return 0; }


  };


  inline bool load_html_resource(UINT resId, PBYTE& pb, DWORD& cb )
  {
    HRSRC hrsrc = ::FindResource(0, MAKEINTRESOURCE(resId), MAKEINTRESOURCE(23));
    if (!hrsrc) return FALSE; // resource not found here - proceed with default loader
    // Load specified resource and check if ok
    HGLOBAL hgres = ::LoadResource(0, hrsrc);
    if (!hgres) return FALSE;
    // Retrieve resource data and check if ok
    pb = (PBYTE)::LockResource(hgres); if (!pb) return FALSE;
    cb = ::SizeofResource(0, hrsrc); if (!cb) return FALSE;
    return TRUE;
  }

  inline bool load_resource_data(LPCWSTR uri, PBYTE& pb, DWORD& cb )
  {

    if (!uri || !uri[0]) return LOAD_DISCARD;
    // Retrieve url specification into a local storage since FindResource() expects 
    // to find its parameters on stack rather then on the heap under Win9x/Me
    WCHAR achURL[MAX_PATH]; wcsncpy(achURL, uri, MAX_PATH);
  
    LPWSTR pszName = achURL;

    // Separate extension if any
    LPWSTR pszExt = wcsrchr(pszName, '.'); if (pszExt) *pszExt++ = '\0';

    // Find specified resource and leave if failed. Note that we use extension
    // as the custom resource type specification or assume standard HTML resource 
    // if no extension is specified

    HRSRC hrsrc = 0;
    bool  isHtml = false;
    if( pszExt == 0 || wcsicmp(pszExt,L"HTML") == 0)
    {
      hrsrc = ::FindResourceW(0, pszName, MAKEINTRESOURCEW(23));
      isHtml = true;
    }
    else
      hrsrc = ::FindResourceW(0, pszName, pszExt);

    if (!hrsrc) return false; // resource not found here - proceed with default loader

    // Load specified resource and check if ok

    HGLOBAL hgres = ::LoadResource(0, hrsrc);
    if (!hgres) return false;

    // Retrieve resource data and check if ok

    pb = (PBYTE)::LockResource(hgres); if (!pb) return FALSE;
    cb = ::SizeofResource(0, hrsrc); if (!cb) return FALSE;

    // Report data ready
    
    return true;
  }




}

#endif
