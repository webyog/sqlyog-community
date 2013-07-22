#include "behavior_aux.h"

namespace htmlayout 
{
 
/** behavior:sizer, 
 * allows to resize elements 
 **/
    
struct sizer: public event_handler
{
    bool tracking;
    SIZE delta;
    // ctor
    sizer(const char* name = "sizer"): event_handler(HANDLE_MOUSE), tracking(false) {}
    
    virtual void attached  (HELEMENT he ) 
    { 
      tracking = false;
    } 
    virtual void detached  (HELEMENT he ) 
    { 
      delete this;
    } 

    // either CSS custom attribute -resize: horizontal | vertical | both
    // or element attribute resize = horizontal | vertical | both
    bool is_resize_vertical(dom::element& self)
    {
      const wchar_t* r = self.attribute("-resize", (const wchar_t*)0);
      return !aux::wcseq(r, L"horizontal");
    }
    bool is_resize_horizontal(dom::element& self)
    {
      const wchar_t* r = self.attribute("-resize", (const wchar_t*)0);
      return !aux::wcseq(r, L"vertical");
    }

    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params ) 
    {
      dom::element self = he;
      if( !tracking )
      {
          if(params.cmd == (MOUSE_DOWN | SINKING))
          {
            tracking = tracking;
          }

          if( params.is_on_icon && (params.cmd == (MOUSE_DOWN | SINKING)) && (params.button_state == MAIN_MOUSE_BUTTON))
            // icon - foreground-image/foreground-repeat:no-repeat is serving role of gripper
          {
            tracking = true;
            RECT rc = self.get_location(SELF_RELATIVE | CONTENT_BOX);
            delta.cx = rc.right - params.pos.x;
            delta.cy = rc.bottom - params.pos.y;
            self.set_capture();
            return TRUE; // handled
          }
          return FALSE; // not handled 
      }

      // tracking mode
      if( params.cmd == (MOUSE_MOVE | SINKING) )
      {
        int w = max(0, params.pos.x + delta.cx); 
        int h = max(0, params.pos.y + delta.cy);
        if(is_resize_horizontal(self)) 
          self.set_style_attribute("width",aux::itow(w));
        if(is_resize_vertical(self)) 
          self.set_style_attribute("height",aux::itow(h));
        
        /*if(self.get_state(STATE_POPUP))
          self.update( RESET_STYLE_THIS | MEASURE_DEEP | REDRAW_NOW);
        else
        {
          dom::element parent = self.parent(); // as dimensions of this element were changed we need to remeasure parent element. 
          parent.update( MEASURE_DEEP | REDRAW_NOW );
        }*/
        return TRUE; // handled
      }

      else if( params.cmd == (MOUSE_UP | SINKING) )
      {
        self.release_capture();
        tracking = false;
        return TRUE; // handled
      }
      return FALSE; 
    }

};

struct sizer_factory: public behavior
{
    sizer_factory(): behavior(HANDLE_MOUSE, "sizer") {}

    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT /*he*/ ) 
    { 
      return new sizer(); 
    }
};

// instantiating and attaching it to the global list
sizer_factory sizer_factory_instance;

}