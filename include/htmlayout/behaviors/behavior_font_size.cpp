#include "behavior_aux.h"

#pragma warning(disable:4786) //identifier was truncated...

/*
BEHAVIOR: font-size
    
  Increases/decreases font size on CTRL+MOUSE_WHEEL

TYPICAL USE CASE:
   < html style="behavior:font-size" >
   </html>
   
DEMO of the behavior:
  sdk/html_samples/cssmap.htm 
   
*/

namespace htmlayout 
{
  struct font_size: public behavior
  {
      // ctor
      font_size(): behavior(HANDLE_MOUSE, "font-size") { }
    
      virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
      {
        if( event_type == (MOUSE_WHEEL | SINKING) && keyboardStates == CONTROL_KEY_PRESSED)
        {
          dom::element el = he;
          int zoom = aux::wtoi(el.get_attribute("zoom"),4);
          int dir = (int)mouseButtons;
          int new_zoom = aux::limit(zoom + dir,1,7);
          if( new_zoom != zoom)
          {
            el.set_attribute("zoom",aux::itow(new_zoom));
            //el.update(true);
          }
          return true;
        }
        return false;
      }
  };

  // instantiating and attaching it to the global list
  font_size font_size_instance;

} // htmlayout namespace