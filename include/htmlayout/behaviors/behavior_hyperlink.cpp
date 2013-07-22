#include "behavior_aux.h"

namespace htmlayout 
{

/*
BEHAVIOR: hyperlink
    goal: Implementation of hyperlink notifications.
    All <a href="something"> elements have "style.behavior=hyperlink" set by default
SAMPLE:
   TBD

*/

struct hyperlink: public behavior
{

    // ctor
    hyperlink(): behavior(HANDLE_MOUSE | HANDLE_FOCUS | HANDLE_KEY | HANDLE_METHOD_CALL, 
      "hyperlink") {}

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      switch( event_type )
      {
        case MOUSE_ENTER:
          {
            dom::element el = he;
            notify(el,NMHL_HYPERLINK::ENTER);
            return false;
          }
          break;
        case MOUSE_LEAVE:
          {
            dom::element el = he;
            notify(el,NMHL_HYPERLINK::LEAVE);
            return false;
          } 
          break;
        case MOUSE_DOWN: 
          if( mouseButtons == MAIN_MOUSE_BUTTON )
          {
            dom::element el = he;
            el.set_state(STATE_CURRENT); // we are reusing STATE_CURRENT to detect click in the element
            return true;
          }
          break;
        case MOUSE_UP:
          {
            dom::element el = he;
            bool was_current = el.get_state(STATE_CURRENT);
            if( was_current )
            {
              el.set_state(0,STATE_CURRENT); // clearing CURRENT state
              el.post_event(HYPERLINK_CLICK,BY_MOUSE_CLICK,el);
              notify(el,NMHL_HYPERLINK::CLICK);
              return true;
            }
          }
          break;
      }

      return false;
    }

    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if( event_type == KEY_UP )
      {
        if(code != ' ' && code != VK_RETURN)
          return FALSE;
        dom::element el = he;
        el.post_event(HYPERLINK_CLICK,BY_KEY_CLICK,el); // send bubbling event.
        notify(el,NMHL_HYPERLINK::CLICK); // send standard notifiaction too. 
        return TRUE;
      }
      return FALSE; 
    }

    virtual BOOL on_focus  (HELEMENT he, HELEMENT target, UINT event_type ) 
    { 
      return TRUE; 
    }

    virtual BOOL on_method_call (HELEMENT he, UINT methodID, METHOD_PARAMS* params ) 
    { 
      if( methodID == DO_CLICK )
      {
         dom::element el = he;
         el.set_state(STATE_FOCUS);
         //notify(el, NMHL_HYPERLINK::CLICK);
         return TRUE;
      }
      return FALSE; /*not handled*/ 
    }


    void notify(dom::element& el, NMHL_HYPERLINK::type code)
    {
      // send notification
        NMHL_HYPERLINK nm;
        memset(&nm,0,sizeof(nm));

        HWND hwnd = el.get_element_hwnd(true);

        nm.hdr.code = HLN_HYPERLINK;
        nm.hdr.hwndFrom = hwnd;
        nm.hdr.idFrom = GetDlgCtrlID(hwnd);

        nm.action = code;
        nm.he = el;

        dom::element root = el.root();

        const wchar_t *pHREF = el.get_attribute("href");
        if(pHREF)
        {
          if(code == NMHL_HYPERLINK::CLICK && pHREF[0] == '#') // anchor name, this is a local hyperlink
          {
            if( pHREF+1 == 0 ) // href='#' case
              return;
            
            dom::element anchor_el = root.find_first("[id='%S'],[name='%S']",pHREF+1,pHREF+1);
              //find_element_by_name(el.root_element(hwnd), pHREF + 1);
            if(anchor_el.is_valid()) // found
            {
              anchor_el.scroll_to_view(true /* scroll it to top of the view */);
              return; // shall host be notified about this?
            }
          }
          wcsncpy(nm.szHREF,pHREF,MAX_URL_LENGTH);
          el.combine_url(nm.szHREF,MAX_URL_LENGTH);
        }
        const wchar_t *pszTarget = el.get_attribute("target");
        if(pszTarget)
        {
          if(code == NMHL_HYPERLINK::CLICK && try_to_load( root, nm.szHREF, pszTarget ))
            return;

          wcsncpy(nm.szTarget,pszTarget,MAX_URL_LENGTH);
        }

        ::SendMessage(hwnd,WM_BEHAVIOR_NOTIFY,HLN_HYPERLINK,LPARAM(&nm));

    }

    bool try_to_load( dom::element& root, const wchar_t* url, const wchar_t* target )
    {
      if(target[0] == '_' ) 
        return false; // do ShellExecute here to open default browser if needed.
      dom::element target_el = root.find_first("#%S",target);                              
      if(!target_el.is_valid())
        return false;

      target_el.load_html(url);
      return true;
    }



   
};

// instantiating and attaching it to the global list
hyperlink hyperlink_instance;


} // htmlayout namespace
