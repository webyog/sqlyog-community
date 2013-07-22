/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * HTML dialog. 
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2009, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief Implementation of HTML based dialog
 **/

#ifndef __htmlayout_dialog_hpp__
#define __htmlayout_dialog_hpp__

#include "htmlayout_dom.hpp"
#include "htmlayout_behavior.hpp"
#include "htmlayout_controls.hpp"
#include "htmlayout_notifications.hpp"
#include "htmlayout_queue.h"

#include <shellapi.h>
#include <assert.h>

#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) //'strcpy' was declared deprecated
#pragma warning(disable:4100) //unreferenced formal parameter 

#pragma once

/**HTMLayout namespace.*/
namespace htmlayout
{

  class dialog: public event_handler,
                public notification_handler<dialog>
  {
  public:
     HWND           hwnd;
     HWND           parent;
     POINT          position; 
     INT            alignment;
     UINT           style;
     UINT           style_ex;
     named_values*  pvalues;
     LPCBYTE        html;
     UINT           html_length;
     json::string   base_url;
     int            return_id;
  
     dialog(HWND hWndParent, UINT styles = 0): pvalues(0),
        event_handler( HANDLE_BEHAVIOR_EVENT | HANDLE_INITIALIZATION),
        return_id(IDCANCEL)
      {  
        parent = hWndParent;
        position.x = 0;
        position.y = 0;
        alignment = 1; // center of desktop
        style = WS_DLGFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | styles;
        style_ex = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
      }

    // SHOW function family - show modal dalog and return one of 
    // IDOK, IDCANCEL, etc. codes. 
    // Dialog buttons shall be defined as <button name="OK">, <button name="CANCEL">, etc.

    // show HTML dialog from url
    unsigned int show( LPCWSTR url  );
    
    // show HTML dialog from resource given by resource id
    unsigned int show( UINT html_res_id );
    
    // show HTML dialog from html in memory buffer
    unsigned int show( LPCBYTE html, UINT html_length );


    // INPUT function family - show modal dalog and return one of button codes,
    // values collection is used for initializing and returning values of inputs.

    // show HTML input dialog from url
    unsigned int input( LPCWSTR url, named_values& values  )
    {
      pvalues = &values;
      return show(url);
    }
    
    // show HTML input dialog from resource given by resource id
    unsigned int input( UINT html_res_id, named_values& values )
    {
      pvalues = &values;
      return show(html_res_id);
    }
    
    // show HTML input dialog from html in memory buffer
    unsigned int input( LPCBYTE html, UINT html_length, named_values& values )
    {
      pvalues = &values;
      return show(html, html_length);
    }

  protected:
    virtual BOOL handle_event ( HELEMENT he, BEHAVIOR_EVENT_PARAMS& params ); 
    virtual BOOL handle_key ( HELEMENT he, KEY_PARAMS& params );
    virtual HWND create_window();

    static dialog* self(HWND hwndDlg)
    {
      #if defined(UNDER_CE)
        return static_cast<dialog*>( (void*)GetWindowLong(hwndDlg, DWL_USER) );
      #else
        return static_cast<dialog*>( (void*)GetWindowLongPtr(hwndDlg, DWLP_USER) );
      #endif
    }
    static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR on_WM_INITDIALOG( HWND hwndDlg, WPARAM wParam, LPARAM lParam );
    static INT_PTR on_WM_DESTROY( HWND hwndDlg, WPARAM wParam, LPARAM lParam );
    static INT_PTR on_WM_USER( HWND hwndDlg, WPARAM wParam, LPARAM lParam );
    static INT_PTR on_WM_CLOSE( HWND hwndDlg, WPARAM wParam, LPARAM lParam );
    static void do_modal_loop(HWND hwnd);
  };

  
// implementations 

    // show HTML dialog from url
    inline unsigned int dialog::show( LPCWSTR url ) 
    { 
      return show( (LPCBYTE) url, 0); 
    }

    // show HTML dialog from resource given by resource id
    inline unsigned int dialog::show( UINT html_res_id )
    {

      PBYTE   html;
      DWORD   html_length;
      if(!load_html_resource(html_res_id, html, html_length ))
      {
        assert(false); // resource not found!
        return 0;
      }
      return show(html, html_length);
    }
    
    // show HTML dialog from html in memory buffer
    inline unsigned int dialog::show( LPCBYTE html, UINT html_length )
    {
      this->html = html;
      this->html_length = html_length;
      HWND hwnd = create_window();
      if(pvalues)
      {
        dom::element root = dom::element::root_element(hwnd);
        set_values(root,*pvalues);
      }
      ShowWindow(hwnd,SW_SHOW);
      do_modal_loop(hwnd); 
      return return_id;
    }


    // handle button events.
    // here it does special treatment of "dialog buttons"

    inline BOOL dialog::handle_event (HELEMENT he, BEHAVIOR_EVENT_PARAMS& params ) 
      { 
        dom::element src = params.heTarget;
        if( params.cmd == BUTTON_CLICK )
        {
          const wchar_t* bname = src.get_attribute("name");
          if(!bname)
            return FALSE;

          bool positive_answer = false;
    
          return_id = 0;
          if( aux::wcseqi(bname,L"OK") )            { return_id =IDOK    ; positive_answer = true; }
          else if( aux::wcseqi(bname,L"CANCEL") )     return_id =IDCANCEL;
          else if( aux::wcseqi(bname,L"ABORT") )      return_id =IDABORT ;
          else if( aux::wcseqi(bname,L"RETRY") )    { return_id =IDRETRY ; positive_answer = true; }
          else if( aux::wcseqi(bname,L"IGNORE") )     return_id =IDIGNORE;
          else if( aux::wcseqi(bname,L"YES") )      { return_id =IDYES   ; positive_answer = true; }
          else if( aux::wcseqi(bname,L"NO") )         return_id =IDNO    ;
          else if( aux::wcseqi(bname,L"CLOSE") )      return_id =IDCLOSE ;
          else if( aux::wcseqi(bname,L"HELP") )       return_id =IDHELP  ; // ?
          else 
            return event_handler::handle_event (he, params) ;

          HWND hwndDlg = src.get_element_hwnd(true);

          if(positive_answer && pvalues)
          {
            dom::element root = src.root();
            pvalues->clear();
            get_values(root,*pvalues);
          }
          ::PostMessage(hwndDlg, WM_CLOSE, 0,0 );
          return TRUE;
        }

#if !defined( _WIN32_WCE ) 
        else if (params.cmd == (HYPERLINK_CLICK | SINKING) )
        {
          HWND hwndLayout = src.get_element_hwnd(true);
          const wchar_t* url = src.get_attribute("href");
          if( url )
          {
            #if !defined(UNICODE) 
              ::ShellExecuteA(hwndLayout,"open", aux::w2a(url), NULL,NULL,SW_SHOWNORMAL);
            #else
              ::ShellExecuteW(hwndLayout,L"open", url, NULL,NULL,SW_SHOWNORMAL);
            #endif
            return TRUE;
          }
        }
#endif
        return FALSE;
      }

    inline BOOL dialog::handle_key (HELEMENT he, KEY_PARAMS& params ) 
    {
       if(params.cmd != KEY_DOWN)
         return FALSE;

       dom::element root = dom::element::root_element(hwnd);

       switch(params.key_code)
       {
         case VK_RETURN:
           {
             dom::element def = root.find_first("[role='default-button']");
             if( def.is_valid() )
             {
               METHOD_PARAMS params; params.methodID = DO_CLICK;
               return def.call_behavior_method(&params)? TRUE:FALSE;
             }
             
           } break;
         case VK_ESCAPE:
           {
             dom::element def = root.find_first("[role='cancel-button']");
             if( def.is_valid() )
             {
               METHOD_PARAMS params; params.methodID = DO_CLICK;
               return def.call_behavior_method(&params)? TRUE:FALSE;
             }
           } break;
       }
       return FALSE;
    }

    inline HWND dialog::create_window()
    {
      struct zDLGTEMPLATE: DLGTEMPLATE 
      {
        WORD strings[3];
      };

      zDLGTEMPLATE dt; 
      memset(&dt,0,sizeof(dt));

      dt.style            = style;
      dt.dwExtendedStyle  = style_ex;
      dt.cdit = 0;

      HINSTANCE hinstance = 
      #if defined(UNDER_CE)
        (HINSTANCE)::GetModuleHandle(NULL); 
      #else
        (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
      #endif

      hwnd = CreateDialogIndirectParam(
        hinstance,                        // handle to module 
        &dt,                              // dialog box template
        parent,                           // handle to owner window
        &DialogProc,                      // dialog box procedure
        LPARAM(this)                      // initialization value
      );

      return hwnd;
    }


    inline void dialog::do_modal_loop(HWND hwnd)
    {
#if defined(UNDER_CE)
       HWND frm = GetWindow(hwnd,GW_OWNER);
#else
       HWND frm = GetAncestor(hwnd,GA_ROOTOWNER);
#endif
       EnableWindow(frm,FALSE);
       MSG msg;
       while(::IsWindow(hwnd) && GetMessage(&msg,NULL,0,0))
       {
         queue::execute();
         TranslateMessage(&msg);
         DispatchMessage(&msg);
       }
       EnableWindow(frm,TRUE);
       SetForegroundWindow(frm);
    } 

    inline INT_PTR CALLBACK dialog::DialogProc
    (
      HWND hwndDlg,   // handle to dialog box
      UINT uMsg,      // message
      WPARAM wParam,  // first message parameter
      LPARAM lParam ) // second message parameter
    {
      BOOL handled = false;
      LRESULT lr = HTMLayoutProcND(hwndDlg,uMsg,wParam,lParam, &handled);
      if( handled )
        return lr;

      switch(uMsg)
      {
        case WM_INITDIALOG:   return on_WM_INITDIALOG( hwndDlg, wParam, lParam ); 
        case WM_CLOSE:        return on_WM_CLOSE( hwndDlg, wParam, lParam ); 
        case WM_DESTROY:      return on_WM_DESTROY( hwndDlg, wParam, lParam );
        case WM_USER:         return on_WM_USER( hwndDlg, wParam, lParam );
      }
      return FALSE;
    }

    inline INT_PTR dialog::on_WM_INITDIALOG( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
    {
      dialog* ctl = static_cast<dialog*>((void*)lParam );
      #if defined(UNDER_CE)
        SetWindowLong(hwndDlg, DWL_USER, LONG(ctl));
      #else
        SetWindowLongPtr(hwndDlg, DWLP_USER, LONG_PTR(ctl));
      #endif
      ctl->hwnd = hwndDlg;

      ctl->setup_callback(hwndDlg);
      attach_event_handler(hwndDlg,ctl);

      if(!ctl->html) 
        return FALSE;
      if(ctl->html_length == 0) // this is a file name
        HTMLayoutLoadFile(hwndDlg,(LPCWSTR)ctl->html);
      else
        HTMLayoutLoadHtmlEx(hwndDlg, ctl->html, ctl->html_length,ctl->base_url);

      dom::element root = dom::element::root_element(hwndDlg);
      if(!root.is_valid())
        return FALSE;

      // set dialog caption
      dom::element title = root.find_first("title"); 
      if( title.is_valid() )
		  ::SetWindowText(hwndDlg,w2t(title.text().c_str()));

      SIZE sz;
      sz.cx = HTMLayoutGetMinWidth(hwndDlg);
      sz.cy = HTMLayoutGetMinHeight(hwndDlg,sz.cx);

      RECT rc; rc.left = ctl->position.x;
               rc.top = ctl->position.y;
               rc.right = rc.left + sz.cx;
               rc.bottom = rc.top + sz.cy;

      AdjustWindowRectEx( 
        &rc, 
        GetWindowLong(hwndDlg,GWL_STYLE),
        FALSE,
        GetWindowLong(hwndDlg,GWL_EXSTYLE));

      sz.cx = rc.right - rc.left;
      sz.cy = rc.bottom - rc.top;
      
      if( ctl->alignment == 1)
      {
        RECT prc;
        ::GetClientRect(::GetDesktopWindow(),&prc);
        rc.left = (prc.right - prc.left - sz.cx) / 2 + prc.left;
        rc.top = (prc.bottom - prc.top - sz.cy) / 2 + prc.top;
        rc.right = rc.left + sz.cx;
        rc.bottom = rc.top + sz.cy;

      }
      else if( ctl->alignment == 2 )
      {
        RECT prc;
        ::GetWindowRect(ctl->parent,&prc);
        rc.left = (prc.right - prc.left - sz.cx) / 2 + prc.left;
        rc.top = (prc.bottom - prc.top - sz.cy) / 2 + prc.top;
        rc.right = rc.left + sz.cx;
        rc.bottom = rc.top + sz.cy;
      }
      UINT MOVE_FLAGS = (SWP_NOZORDER);
      ::SetWindowPos(hwndDlg, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, MOVE_FLAGS);
      ::PostMessage(hwndDlg, WM_USER, 0,0);
      
      return 0;
    }

    inline INT_PTR dialog::on_WM_DESTROY( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
    {
      dialog* ctl = self(hwndDlg);
      if( ctl)
        detach_event_handler(hwndDlg, ctl);
      return 0;
    } 

    inline INT_PTR dialog::on_WM_USER( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
    {
      dom::element root = dom::element::root_element(hwndDlg);
      if( !root.is_valid() )
        return 0;
      dom::element def = root.find_first("[role='default-button']");
      if( def.is_valid() )
        def.set_state(STATE_FOCUS);
      else 
        ::SetFocus(hwndDlg);
      return 0;
    } 

    inline INT_PTR dialog::on_WM_CLOSE( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
    {
       HWND parent = ::GetParent(hwndDlg);
       ::EnableWindow(parent,TRUE);
       ::SetForegroundWindow(parent);
       ::DestroyWindow(hwndDlg);
       return 0;
    }


}

#endif