#include "behavior_aux.h"

namespace htmlayout 
{

/*
OUTDATED! use behavior 'button' instead.
It is here only for the demo purposes.

BEHAVIOR: command
    goal: Implementation of command button.
          command button is any HTML element having 
          attribute 'id' set and behavior:command (this one)
TYPICAL USE CASE:
    <div style="behavior:command" id="my-button">My button text</div>
SAMPLE:
*/

struct command: public behavior
{

    // ctor
    command(): behavior(HANDLE_MOUSE, "command") {}

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      switch( event_type )
      {
        case MOUSE_DOWN: 
          {
            dom::element el = he;
            el.set_capture();
            el.set_state(STATE_CURRENT); // we are reusing STATE_CURRENT to detect click in the element
            return true;
          }
          break;
        case MOUSE_UP:
          {
            dom::element el = he;
            el.release_capture();

            bool was_current = el.get_state(STATE_CURRENT);
            if( was_current )
              el.set_state(0,STATE_CURRENT); // clearing CURRENT state

            RECT rc = el.get_location(SELF_RELATIVE | BORDER_BOX);
            if( pt.x < rc.left || pt.y < rc.top || pt.x > rc.right || pt.y > rc.bottom )
              return true; // MOUSE_UP is outside

            const wchar_t *pID = el.get_attribute("id");
            if(pID && pID[0] && was_current)
            {
              // send command
                NMHL_COMMAND_CLICK nm;
                memset(&nm,0,sizeof(nm));

                HWND hwnd = el.get_element_hwnd(true);

                nm.hdr.code = HLN_COMMAND_CLICK;
                nm.hdr.hwndFrom = hwnd;
                nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                wcsncpy(nm.szElementID,pID,MAX_URL_LENGTH);
                nm.he = he;
                //must be ::GetParent(hwnd) in fact
                ::SendMessage(hwnd,WM_BEHAVIOR_NOTIFY,HLN_COMMAND_CLICK,LPARAM(&nm));
            }
            return true;
          }
      }

      return false;
    }
    
   
};

// instantiating and attaching it to the global list
command command_instance;

} // htmlayout namespace

