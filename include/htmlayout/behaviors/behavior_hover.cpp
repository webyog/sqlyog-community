#include "behavior_aux.h"

/*
BEHAVIOR: hover-click
    
  calls DO_CLICK method after 'delay=milliseconds' 

TYPICAL USE CASE:
   < div style="behavior:popup-menu hover-click" >
        ....
        <menu>
          <li>...
          <li>...
          <li>...
          <li>...       
        </menu>
   </div>
*/

bool IsInActiveWindow(HWND hwnd);

namespace htmlayout 
{

  struct hover_click: public behavior
  {
      // ctor
      hover_click(): behavior(HANDLE_MOUSE | HANDLE_TIMER, "hover-click") { }
    
      virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
      {
        switch( event_type )
        {
          case MOUSE_MOVE: 
            {
              dom::element el = he;
              if( el.get_state(STATE_OWNS_POPUP) )
                return false;

              if( !IsInActiveWindow(el.get_element_hwnd(true)) )
                return false;

              UINT delay = el.get_attribute_int("delay", ::GetDoubleClickTime() + 10);
              el.start_timer(delay);
            }
            break;
          case MOUSE_LEAVE: 
            {
              dom::element el = he;
              el.stop_timer();            
              HTMLayoutHidePopup(el.find_first(":popup"));
            }
            break;
        }
        return false;
      }

      virtual BOOL on_timer  (HELEMENT he ) 
      { 
         dom::element el = he;
         METHOD_PARAMS prm; prm.methodID = DO_CLICK;
         el.call_behavior_method(&prm);
         return false; // do not need timer anymore.
      }
  
  };

  // instantiating and attaching it to the global list
  hover_click hover_click_instance;

} // htmlayout namespace

bool IsInActiveWindow(HWND hwnd)
{
  GUITHREADINFO guit; guit.cbSize = sizeof(guit);
  if(GetGUIThreadInfo(NULL, &guit))
  {
    while( ::IsWindow(hwnd) )
    {
      if( hwnd == guit.hwndActive ) return true;
      hwnd = ::GetParent(hwnd);
    }
  }
  return false;
}


