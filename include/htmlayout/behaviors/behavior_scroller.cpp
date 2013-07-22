#include "behavior_aux.h"

namespace htmlayout 
{
 
/** behavior:scroller, 
 * another way of scrolling  
 **/
    
struct scroller: public behavior
{
    // ctor
    scroller(const char* name = "scroller"): behavior(HANDLE_MOUSE, name) {}
    
    virtual void attached  (HELEMENT he ) 
    { 
    } 

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      if( (event_type != (MOUSE_MOVE | SINKING)) || (mouseButtons != 0))
        return false; // only mouse move in unpressed state.

      POINT scroll_pos;
      RECT  view_rect; SIZE content_size;

      dom::element el = he;

      el.get_scroll_info(scroll_pos, view_rect, content_size);

      if( pt.x < view_rect.left || pt.x > view_rect.right || 
          pt.y < view_rect.top || pt.y > view_rect.bottom )
        return false;

      int vh = view_rect.bottom - view_rect.top;
      if( vh >= content_size.cy  )
        return false;

      int dh = content_size.cy - vh;

      int py = ( pt.y * dh) / vh;
      if( py != scroll_pos.y )
      {
        scroll_pos.y = py;
        el.set_scroll_pos(scroll_pos, false);
      }
      return false; 
    }


};

scroller          scroller_instance;


}